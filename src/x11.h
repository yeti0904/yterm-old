#pragma once
#include <sys/types.h>
#include <termios.h>
#include "structures.h"

void* x11_term(struct PTY* pty, struct winsize* win);