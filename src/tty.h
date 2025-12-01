/*
 * tty.h - Terminal control layer for interactive TTY mode
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

#ifndef FRAMETEST_TTY_H
#define FRAMETEST_TTY_H

#include <stddef.h>

/* Key codes for special keys */
typedef enum tty_key_t {
	TTY_KEY_NONE = 0,

	/* Printable ASCII (32-126) are returned as-is */

	/* Special keys (values > 256 to avoid ASCII conflicts) */
	TTY_KEY_UP = 256,
	TTY_KEY_DOWN,
	TTY_KEY_LEFT,
	TTY_KEY_RIGHT,
	TTY_KEY_HOME,
	TTY_KEY_END,
	TTY_KEY_PAGEUP,
	TTY_KEY_PAGEDOWN,
	TTY_KEY_INSERT,
	TTY_KEY_DELETE,
	TTY_KEY_ESCAPE,
	TTY_KEY_ENTER,
	TTY_KEY_TAB,
	TTY_KEY_BACKSPACE,

	/* Function keys */
	TTY_KEY_F1,
	TTY_KEY_F2,
	TTY_KEY_F3,
	TTY_KEY_F4,
	TTY_KEY_F5,
	TTY_KEY_F6,
	TTY_KEY_F7,
	TTY_KEY_F8,
	TTY_KEY_F9,
	TTY_KEY_F10,
	TTY_KEY_F11,
	TTY_KEY_F12,

	/* Control combinations */
	TTY_KEY_CTRL_C,
	TTY_KEY_CTRL_D,
	TTY_KEY_CTRL_Z,

	/* Error/special states */
	TTY_KEY_ERROR = -1,
	TTY_KEY_EOF = -2
} tty_key_t;

/* Terminal size */
typedef struct tty_size_t {
	int width;
	int height;
} tty_size_t;

/* Callback for terminal resize events */
typedef void (*tty_resize_callback_t)(int width, int height, void *ctx);

/*
 * Initialize terminal for interactive mode
 * - Enters raw mode (no line buffering, no echo)
 * - Switches to alternate screen buffer
 * - Hides cursor
 * - Sets up signal handlers (SIGWINCH, SIGINT, SIGTERM)
 *
 * Returns 0 on success, -1 on failure
 */
int tty_init(void);

/*
 * Restore terminal to original state
 * - Exits raw mode
 * - Returns to main screen buffer
 * - Shows cursor
 * - Restores original terminal attributes
 *
 * Safe to call multiple times or if tty_init() wasn't called
 */
void tty_cleanup(void);

/*
 * Check if terminal is initialized in raw mode
 */
int tty_is_initialized(void);

/*
 * Check if stdout is a terminal that supports interactive mode
 */
int tty_is_supported(void);

/*
 * Get current terminal size
 */
tty_size_t tty_get_size(void);

/*
 * Set callback for terminal resize events
 * The callback will be called from signal handler context
 */
void tty_set_resize_callback(tty_resize_callback_t callback, void *ctx);

/*
 * Read a key from stdin (non-blocking)
 * Returns the key code, TTY_KEY_NONE if no key available,
 * or TTY_KEY_ERROR on error
 *
 * timeout_ms: -1 = block forever, 0 = non-blocking, >0 = timeout in ms
 */
int tty_read_key(int timeout_ms);

/*
 * Check if a key is available without consuming it
 */
int tty_key_available(void);

/*
 * Move cursor to position (1-based coordinates)
 */
void tty_move_cursor(int row, int col);

/*
 * Clear the screen
 */
void tty_clear_screen(void);

/*
 * Clear from cursor to end of line
 */
void tty_clear_to_eol(void);

/*
 * Show/hide cursor
 */
void tty_show_cursor(void);
void tty_hide_cursor(void);

/*
 * Flush output buffer
 */
void tty_flush(void);

/*
 * Check if we should quit (SIGINT/SIGTERM received)
 */
int tty_should_quit(void);

/*
 * Check if terminal was resized (and clear the flag)
 */
int tty_was_resized(void);

#endif /* FRAMETEST_TTY_H */
