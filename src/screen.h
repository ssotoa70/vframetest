/*
 * screen.h - Double-buffered terminal screen rendering
 *
 * This file is part of vframetest.
 * Copyright (c) 2023-2025 Tuxera Inc.
 */

#ifndef FRAMETEST_SCREEN_H
#define FRAMETEST_SCREEN_H

#include <stddef.h>

#define SCREEN_MAX_WIDTH 256
#define SCREEN_MAX_HEIGHT 64

/* Color definitions */
typedef enum {
	COLOR_DEFAULT = 0,
	COLOR_BLACK,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_WHITE,
	COLOR_BRIGHT_BLACK,
	COLOR_BRIGHT_RED,
	COLOR_BRIGHT_GREEN,
	COLOR_BRIGHT_YELLOW,
	COLOR_BRIGHT_BLUE,
	COLOR_BRIGHT_MAGENTA,
	COLOR_BRIGHT_CYAN,
	COLOR_BRIGHT_WHITE
} screen_color_t;

/* Text attributes */
typedef enum {
	ATTR_NONE = 0,
	ATTR_BOLD = (1 << 0),
	ATTR_DIM = (1 << 1),
	ATTR_UNDERLINE = (1 << 2),
	ATTR_REVERSE = (1 << 3)
} screen_attr_t;

/* Color attribute packed into single byte: fg(4 bits) | bg(4 bits) */
typedef unsigned char color_attr_t;

#define MAKE_COLOR(fg, bg) (((fg) & 0x0F) | (((bg) & 0x0F) << 4))
#define GET_FG(c) ((c) & 0x0F)
#define GET_BG(c) (((c) >> 4) & 0x0F)

/* Theme color roles */
typedef struct {
	screen_color_t border_fg;
	screen_color_t border_bg;
	screen_color_t title_fg;
	screen_color_t title_bg;
	screen_color_t text_fg;
	screen_color_t text_bg;
	screen_color_t highlight_fg;
	screen_color_t highlight_bg;
	screen_color_t selected_fg;
	screen_color_t selected_bg;
	screen_color_t success_fg;
	screen_color_t error_fg;
	screen_color_t warning_fg;
	screen_color_t info_fg;
	screen_color_t value_fg;
	screen_color_t progress_fg;
	screen_color_t progress_bg;
	screen_color_t tab_active_fg;
	screen_color_t tab_active_bg;
	screen_color_t tab_inactive_fg;
	screen_color_t status_fg;
	screen_color_t status_bg;
} screen_theme_t;

/* Screen buffer - stores characters and color attributes */
typedef struct {
	char cells[SCREEN_MAX_HEIGHT][SCREEN_MAX_WIDTH];
	color_attr_t colors[SCREEN_MAX_HEIGHT][SCREEN_MAX_WIDTH];
	unsigned char attrs[SCREEN_MAX_HEIGHT][SCREEN_MAX_WIDTH];
	int width;
	int height;
	int cursor_row;
	int cursor_col;
	screen_color_t current_fg;
	screen_color_t current_bg;
	screen_attr_t current_attr;
	const screen_theme_t *theme;
} screen_t;

/* Built-in themes */
extern const screen_theme_t THEME_DEFAULT;
extern const screen_theme_t THEME_DARK;
extern const screen_theme_t THEME_LIGHT;
extern const screen_theme_t THEME_MONO;

/* Initialize screen buffer */
void screen_init(screen_t *scr, int width, int height);

/* Set theme */
void screen_set_theme(screen_t *scr, const screen_theme_t *theme);

/* Clear screen buffer */
void screen_clear(screen_t *scr);

/* Set cursor position (0-indexed) */
void screen_move(screen_t *scr, int row, int col);

/* Set current colors */
void screen_set_color(screen_t *scr, screen_color_t fg, screen_color_t bg);
void screen_set_fg(screen_t *scr, screen_color_t fg);
void screen_set_bg(screen_t *scr, screen_color_t bg);
void screen_set_attr(screen_t *scr, screen_attr_t attr);
void screen_reset_color(screen_t *scr);

/* Print string at cursor, advance cursor */
void screen_print(screen_t *scr, const char *str);

/* Print formatted string */
void screen_printf(screen_t *scr, const char *fmt, ...);

/* Print single character */
void screen_putc(screen_t *scr, char c);

/* Fill rest of current line with spaces */
void screen_fill_line(screen_t *scr);

/* Draw horizontal line */
void screen_hline(screen_t *scr, int row, int col, int len, char c);

/* Draw vertical line */
void screen_vline(screen_t *scr, int row, int col, int len, char c);

/* Draw box */
void screen_box(screen_t *scr, int row, int col, int width, int height);

/* Flush buffer to terminal */
void screen_render(screen_t *scr);

#endif
