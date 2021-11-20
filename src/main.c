#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <pty.h>
#include <utmp.h>
#include <X11/Xlib.h>
#include <assert.h>
#include "colour.h"
#include "string.h"

int main(void) {
	int master, slave;
	struct winsize win;
	pid_t process;

	win.ws_col = 50;
	win.ws_row = 50;

	if (openpty(&master, &slave, NULL, NULL, &win) == -1) {
		perror("failed to open terminal");
		return 1;
	}

	login_tty(slave);

	process = forkpty(&master, NULL, NULL, &win);
	if (process == -1) {
		perror("fork");
		return 1;
	}
	else if (process == 0) { // child process
		char *env[] = { "TERM=vt100", NULL };

		close(master);
		setsid();
		ioctl(slave, TIOCSCTTY, NULL);

		// allow shell to talk to terminal
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		close(slave);

		//char* shell = getenv("SHELL");
		char* shell = "/bin/sh";

		execle(shell, shell, NULL, env);
	}
	else if (process > 0) { // parent
		bool     run = true;
		Display* display;
		Window   window;
		colourv  colours;
		GC       gc;
		XEvent   event;
		string   text;
		char     in;
		char     out;

		text.buf  = (char*) malloc(1);
		text.size = 1;

		*text.buf = '\0';

		display = XOpenDisplay(NULL);
		colours.white = WhitePixel(display, DefaultScreen(display));
		colours.black = BlackPixel(display, DefaultScreen(display));

		window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, win.ws_col * 16, win.ws_row * 8, 0, colours.black, colours.black);
		XSelectInput(display, window, StructureNotifyMask);
		XMapWindow(display, window); // show the window onscreen

		gc = XCreateGC(display, window, 0, NULL);

		while (true) { // wait for window to appear
			XNextEvent(display, &event);
			if (event.type == MapNotify)
				break;
		}
		XStoreName(display, window, "yterm");
		XSetForeground(display, gc, colours.white);

		XSelectInput(display, window, ClientMessage);
		Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", 1);
		XSetWMProtocols(display, window, &wm_delete, 1);
		XSelectInput(display, window, KeyPressMask);

		while (run) {
			// render
			XDrawString(display, window, gc, 0, 16, text.buf, strlen(text.buf));
			XFlush(display);

			// handle events
			XNextEvent(display, &event);
			switch (event.type) {
				case ClientMessage: {
					if (strcmp(XGetAtomName(display, event.xclient.message_type ), "WM_PROTOCOLS") == 0) {
						run = false;
					}
					break;
				}
				case KeyPress: {
					if (event.xkey.window == window) {
						write(slave, &event.xkey.keycode, 1);
						printf("sent key %i '%c'\n", event.xkey.keycode, event.xkey.keycode);
					}
					break;
				}
			}

			// terminal io
			if (read(slave, &in, 1) <= 0) {
				perror("nothing to read");
				return 0;
			}
			printf("new input: %d '%c'\n", in, in);
			++ text.size;
			text.buf = (char*) realloc(text.buf, text.size);
			strncat(text.buf, &in, 1);
		}

		XDestroyWindow(display, window);
		XCloseDisplay(display);
	}
}