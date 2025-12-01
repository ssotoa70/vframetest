/*
 * tty.c - Terminal control layer for interactive TTY mode
 *
 * This file is part of vframetest.
 *
 * Copyright (c) 2023-2025 Tuxera Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Enable POSIX signal handling (sigaction, sigemptyset, SA_RESTART) */
#if defined(__linux__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "tty.h"

/* ANSI escape sequences */
#define ESC "\033"
#define CSI ESC "["

/* Alternate screen buffer */
#define ENTER_ALT_SCREEN CSI "?1049h"
#define EXIT_ALT_SCREEN CSI "?1049l"

/* Cursor control */
#define HIDE_CURSOR CSI "?25l"
#define SHOW_CURSOR CSI "?25h"
#define CLEAR_SCREEN CSI "2J"
#define MOVE_HOME CSI "H"
#define CLEAR_TO_EOL CSI "K"

/* Static state */
static struct termios orig_termios;
static int tty_initialized = 0;
static volatile sig_atomic_t got_sigwinch = 0;
static volatile sig_atomic_t got_sigint = 0;
static volatile sig_atomic_t got_sigterm = 0;

/* Resize callback */
static tty_resize_callback_t resize_callback = NULL;
static void *resize_callback_ctx = NULL;

/* Signal handlers */
static void handle_sigwinch(int sig)
{
	(void)sig;
	got_sigwinch = 1;

	if (resize_callback) {
		tty_size_t size = tty_get_size();
		resize_callback(size.width, size.height, resize_callback_ctx);
	}
}

static void handle_sigint(int sig)
{
	(void)sig;
	got_sigint = 1;
}

static void handle_sigterm(int sig)
{
	(void)sig;
	got_sigterm = 1;
}

int tty_is_supported(void)
{
	if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
		return 0;

	const char *term = getenv("TERM");
	if (!term || strcmp(term, "dumb") == 0)
		return 0;

	return 1;
}

int tty_init(void)
{
	struct termios raw;
	struct sigaction sa;

	if (tty_initialized)
		return 0;

	if (!tty_is_supported())
		return -1;

	/* Save original terminal attributes */
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
		return -1;

	/* Set up raw mode */
	raw = orig_termios;

	/* Input modes: no break, no CR to NL, no parity check, no strip char,
	 * no start/stop output control */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	/* Output modes: disable post processing */
	raw.c_oflag &= ~(OPOST);

	/* Control modes: set 8 bit chars */
	raw.c_cflag |= (CS8);

	/* Local modes: echo off, canonical off, no extended functions,
	 * no signal chars (^Z, ^C) */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	/* Control chars: set return condition - min chars and timer */
	raw.c_cc[VMIN] = 0; /* Return immediately with what's available */
	raw.c_cc[VTIME] = 0; /* No timeout */

	/* Apply raw mode */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		return -1;

	/* Set up signal handlers using sigaction */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigwinch;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGWINCH, &sa, NULL);

	sa.sa_handler = handle_sigint;
	sa.sa_flags = 0; /* Don't restart - we want to break out of reads */
	sigaction(SIGINT, &sa, NULL);

	sa.sa_handler = handle_sigterm;
	sigaction(SIGTERM, &sa, NULL);

	/* Enter alternate screen buffer and hide cursor */
	printf("%s%s%s", ENTER_ALT_SCREEN, CLEAR_SCREEN MOVE_HOME, HIDE_CURSOR);
	fflush(stdout);

	tty_initialized = 1;
	return 0;
}

void tty_cleanup(void)
{
	if (!tty_initialized)
		return;

	/* Show cursor, exit alternate screen, reset attributes */
	printf("%s%s", SHOW_CURSOR, EXIT_ALT_SCREEN);
	fflush(stdout);

	/* Restore original terminal attributes */
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);

	/* Restore default signal handlers */
	signal(SIGWINCH, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	tty_initialized = 0;
}

int tty_is_initialized(void)
{
	return tty_initialized;
}

tty_size_t tty_get_size(void)
{
	tty_size_t size = { 80, 24 }; /* Default fallback */
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
		if (ws.ws_col > 0)
			size.width = ws.ws_col;
		if (ws.ws_row > 0)
			size.height = ws.ws_row;
	}

	return size;
}

void tty_set_resize_callback(tty_resize_callback_t callback, void *ctx)
{
	resize_callback = callback;
	resize_callback_ctx = ctx;
}

/* Parse escape sequences into key codes */
static int parse_escape_sequence(void)
{
	char seq[8];
	int i = 0;

	/* Read the escape sequence with timeout */
	struct timeval tv = { 0,
			      50000 }; /* 50ms timeout for escape sequences */
	fd_set fds;

	while (i < 7) {
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
		if (ret <= 0)
			break;

		if (read(STDIN_FILENO, &seq[i], 1) != 1)
			break;
		i++;

		/* Quick check for complete sequences */
		if (i >= 2) {
			/* CSI sequences: ESC [ ... */
			if (seq[0] == '[') {
				/* Arrow keys: ESC [ A/B/C/D */
				if (seq[1] >= 'A' && seq[1] <= 'D') {
					switch (seq[1]) {
					case 'A':
						return TTY_KEY_UP;
					case 'B':
						return TTY_KEY_DOWN;
					case 'C':
						return TTY_KEY_RIGHT;
					case 'D':
						return TTY_KEY_LEFT;
					}
				}
				/* Home/End: ESC [ H / ESC [ F */
				if (seq[1] == 'H')
					return TTY_KEY_HOME;
				if (seq[1] == 'F')
					return TTY_KEY_END;

				/* Extended sequences: ESC [ N ~ */
				if (i >= 3 && seq[2] == '~') {
					switch (seq[1]) {
					case '1':
						return TTY_KEY_HOME;
					case '2':
						return TTY_KEY_INSERT;
					case '3':
						return TTY_KEY_DELETE;
					case '4':
						return TTY_KEY_END;
					case '5':
						return TTY_KEY_PAGEUP;
					case '6':
						return TTY_KEY_PAGEDOWN;
					}
				}

				/* Function keys: ESC [ NN ~ */
				if (i >= 4 && seq[3] == '~') {
					int num = (seq[1] - '0') * 10 +
						  (seq[2] - '0');
					switch (num) {
					case 11:
						return TTY_KEY_F1;
					case 12:
						return TTY_KEY_F2;
					case 13:
						return TTY_KEY_F3;
					case 14:
						return TTY_KEY_F4;
					case 15:
						return TTY_KEY_F5;
					case 17:
						return TTY_KEY_F6;
					case 18:
						return TTY_KEY_F7;
					case 19:
						return TTY_KEY_F8;
					case 20:
						return TTY_KEY_F9;
					case 21:
						return TTY_KEY_F10;
					case 23:
						return TTY_KEY_F11;
					case 24:
						return TTY_KEY_F12;
					}
				}
			}
			/* SS3 sequences: ESC O ... (alternate arrow keys) */
			else if (seq[0] == 'O') {
				switch (seq[1]) {
				case 'A':
					return TTY_KEY_UP;
				case 'B':
					return TTY_KEY_DOWN;
				case 'C':
					return TTY_KEY_RIGHT;
				case 'D':
					return TTY_KEY_LEFT;
				case 'H':
					return TTY_KEY_HOME;
				case 'F':
					return TTY_KEY_END;
				case 'P':
					return TTY_KEY_F1;
				case 'Q':
					return TTY_KEY_F2;
				case 'R':
					return TTY_KEY_F3;
				case 'S':
					return TTY_KEY_F4;
				}
			}
		}

		tv.tv_sec = 0;
		tv.tv_usec = 10000; /* 10ms for subsequent chars */
	}

	/* Unknown escape sequence, return ESC */
	return TTY_KEY_ESCAPE;
}

int tty_read_key(int timeout_ms)
{
	unsigned char c;
	fd_set fds;
	struct timeval tv, *tvp = NULL;
	int ret;

	if (!tty_initialized)
		return TTY_KEY_ERROR;

	/* Set up select timeout */
	if (timeout_ms >= 0) {
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		tvp = &tv;
	}

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, tvp);

	if (ret == -1) {
		if (errno == EINTR) {
			/* Interrupted by signal */
			if (got_sigint || got_sigterm)
				return 'q'; /* Treat as quit */
			return TTY_KEY_NONE;
		}
		return TTY_KEY_ERROR;
	}

	if (ret == 0)
		return TTY_KEY_NONE; /* Timeout */

	/* Read single byte */
	if (read(STDIN_FILENO, &c, 1) != 1)
		return TTY_KEY_ERROR;

	/* Handle special characters */
	switch (c) {
	case 0x1b: /* Escape */
		return parse_escape_sequence();

	case 0x0d: /* Enter (CR) */
	case 0x0a: /* Enter (LF) */
		return TTY_KEY_ENTER;

	case 0x09: /* Tab */
		return TTY_KEY_TAB;

	case 0x7f: /* Backspace (DEL) */
	case 0x08: /* Backspace (BS) */
		return TTY_KEY_BACKSPACE;

	case 0x03: /* Ctrl-C */
		return TTY_KEY_CTRL_C;

	case 0x04: /* Ctrl-D */
		return TTY_KEY_CTRL_D;

	case 0x1a: /* Ctrl-Z */
		return TTY_KEY_CTRL_Z;

	default:
		/* Regular character */
		return (int)c;
	}
}

int tty_key_available(void)
{
	fd_set fds;
	struct timeval tv = { 0, 0 };

	if (!tty_initialized)
		return 0;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

void tty_move_cursor(int row, int col)
{
	printf(CSI "%d;%dH", row, col);
}

void tty_clear_screen(void)
{
	printf(CLEAR_SCREEN MOVE_HOME);
}

void tty_clear_to_eol(void)
{
	printf(CLEAR_TO_EOL);
}

void tty_show_cursor(void)
{
	printf(SHOW_CURSOR);
}

void tty_hide_cursor(void)
{
	printf(HIDE_CURSOR);
}

void tty_flush(void)
{
	fflush(stdout);
}

int tty_should_quit(void)
{
	return got_sigint || got_sigterm;
}

int tty_was_resized(void)
{
	if (got_sigwinch) {
		got_sigwinch = 0;
		return 1;
	}
	return 0;
}
