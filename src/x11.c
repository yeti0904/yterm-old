#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <X11/Xlib.h>
#include <string.h>
#include "colour.h"
#include "string.h"
#include "structures.h"

void* x11_term(struct PTY* pty, struct winsize* win) {
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

	window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, win->ws_col * 16, win->ws_row * 8, 0, colours.black, colours.black);
	XSelectInput(display, window, StructureNotifyMask | KeyPressMask | ClientMessage);
	XMapWindow(display, window); // show the window onscreen

	gc = XCreateGC(display, window, 0, NULL);

	while (true) { // wait for window to appear
		XNextEvent(display, &event);
		if (event.type == MapNotify)
			break;
	}
	XStoreName(display, window, "yterm");
	XSetForeground(display, gc, colours.white);

	Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", 1);
	XSetWMProtocols(display, window, &wm_delete, 1);

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
					write(pty->slave, &event.xkey.keycode, 1);
					printf("sent key %i '%c'\n", event.xkey.keycode, event.xkey.keycode);
				}
				break;
			}
		}

		// terminal io
		if (read(pty->slave, &in, 1) <= 0) {
			perror("nothing to read");
			return NULL;
		}
		printf("new input: %d '%c'\n", in, in);
		++ text.size;
		text.buf = (char*) realloc(text.buf, text.size);
		strncat(text.buf, &in, 1);
	}

	XDestroyWindow(display, window);
	XCloseDisplay(display);
	return NULL;
}