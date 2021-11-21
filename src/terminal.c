#include <unistd.h>
#include <sys/ioctl.h>
#include "structures.h"

void terminal(struct PTY *pty) {
	char *env[] = { "TERM=vt100", NULL };

	close(pty->master);
	setsid();
	ioctl(pty->slave, TIOCSCTTY, NULL);

	// allow shell to talk to terminal
	dup2(0, pty->slave);
	dup2(1, pty->slave);
	dup2(2, pty->slave);
	close(pty->slave);

	//char* shell = getenv("SHELL");
	char* shell = "/bin/sh";

	execle(shell, shell, NULL, env);
}