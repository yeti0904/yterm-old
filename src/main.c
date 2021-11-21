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
#include "structures.h"
#include "x11.h"
#include "terminal.h"

int main(void) {
	struct PTY pty;
	struct winsize win;
	pid_t process;

	win.ws_col = 50;
	win.ws_row = 50;

	if (openpty(&pty.master, &pty.slave, NULL, NULL, &win) == -1) {
		perror("failed to open terminal");
		return 1;
	}

	login_tty(pty.slave);

	process = forkpty(&pty.master, NULL, NULL, &win);
	if (process == -1) {
		perror("fork");
		return 1;
	}
	else if (process == 0) { // child process
		terminal(&pty);
	}
	else if (process > 0) { // parent
		x11_term(&pty, &win);
	}
}