/*
 * screen.c - Double-buffered terminal screen rendering
 *
 * This file is part of vframetest.
 * Copyright (c) 2023-2025 Tuxera Inc.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "screen.h"

/* Box drawing characters (single-width for simplicity) */
#define BOX_H '-'
#define BOX_V '|'
#define BOX_TL '+'
#define BOX_TR '+'
#define BOX_BL '+'
#define BOX_BR '+'

/* ─────────────────────────────────────────────────────────────────────────────
 * Built-in themes
 * ───────────────────────────────────────────────────────────────────────────── */

const screen_theme_t THEME_DEFAULT = { .border_fg = COLOR_CYAN,
				       .border_bg = COLOR_DEFAULT,
				       .title_fg = COLOR_BRIGHT_WHITE,
				       .title_bg = COLOR_DEFAULT,
				       .text_fg = COLOR_DEFAULT,
				       .text_bg = COLOR_DEFAULT,
				       .highlight_fg = COLOR_BRIGHT_CYAN,
				       .highlight_bg = COLOR_DEFAULT,
				       .selected_fg = COLOR_BLACK,
				       .selected_bg = COLOR_CYAN,
				       .success_fg = COLOR_BRIGHT_GREEN,
				       .error_fg = COLOR_BRIGHT_RED,
				       .warning_fg = COLOR_BRIGHT_YELLOW,
				       .info_fg = COLOR_BRIGHT_BLUE,
				       .value_fg = COLOR_BRIGHT_CYAN,
				       .progress_fg = COLOR_BRIGHT_GREEN,
				       .progress_bg = COLOR_BRIGHT_BLACK,
				       .tab_active_fg = COLOR_BRIGHT_WHITE,
				       .tab_active_bg = COLOR_BLUE,
				       .tab_inactive_fg = COLOR_WHITE,
				       .status_fg = COLOR_BRIGHT_YELLOW,
				       .status_bg = COLOR_DEFAULT };

const screen_theme_t THEME_DARK = { .border_fg = COLOR_BLUE,
				    .border_bg = COLOR_DEFAULT,
				    .title_fg = COLOR_BRIGHT_WHITE,
				    .title_bg = COLOR_DEFAULT,
				    .text_fg = COLOR_WHITE,
				    .text_bg = COLOR_DEFAULT,
				    .highlight_fg = COLOR_BRIGHT_BLUE,
				    .highlight_bg = COLOR_DEFAULT,
				    .selected_fg = COLOR_BLACK,
				    .selected_bg = COLOR_BLUE,
				    .success_fg = COLOR_GREEN,
				    .error_fg = COLOR_RED,
				    .warning_fg = COLOR_YELLOW,
				    .info_fg = COLOR_BLUE,
				    .value_fg = COLOR_CYAN,
				    .progress_fg = COLOR_GREEN,
				    .progress_bg = COLOR_BRIGHT_BLACK,
				    .tab_active_fg = COLOR_BLACK,
				    .tab_active_bg = COLOR_WHITE,
				    .tab_inactive_fg = COLOR_BRIGHT_BLACK,
				    .status_fg = COLOR_YELLOW,
				    .status_bg = COLOR_DEFAULT };

const screen_theme_t THEME_LIGHT = { .border_fg = COLOR_BLUE,
				     .border_bg = COLOR_DEFAULT,
				     .title_fg = COLOR_BLACK,
				     .title_bg = COLOR_DEFAULT,
				     .text_fg = COLOR_BLACK,
				     .text_bg = COLOR_DEFAULT,
				     .highlight_fg = COLOR_BLUE,
				     .highlight_bg = COLOR_DEFAULT,
				     .selected_fg = COLOR_WHITE,
				     .selected_bg = COLOR_BLUE,
				     .success_fg = COLOR_GREEN,
				     .error_fg = COLOR_RED,
				     .warning_fg = COLOR_YELLOW,
				     .info_fg = COLOR_BLUE,
				     .value_fg = COLOR_MAGENTA,
				     .progress_fg = COLOR_GREEN,
				     .progress_bg = COLOR_WHITE,
				     .tab_active_fg = COLOR_WHITE,
				     .tab_active_bg = COLOR_BLUE,
				     .tab_inactive_fg = COLOR_BRIGHT_BLACK,
				     .status_fg = COLOR_BLUE,
				     .status_bg = COLOR_DEFAULT };

const screen_theme_t THEME_MONO = { .border_fg = COLOR_DEFAULT,
				    .border_bg = COLOR_DEFAULT,
				    .title_fg = COLOR_DEFAULT,
				    .title_bg = COLOR_DEFAULT,
				    .text_fg = COLOR_DEFAULT,
				    .text_bg = COLOR_DEFAULT,
				    .highlight_fg = COLOR_DEFAULT,
				    .highlight_bg = COLOR_DEFAULT,
				    .selected_fg = COLOR_BLACK,
				    .selected_bg = COLOR_WHITE,
				    .success_fg = COLOR_DEFAULT,
				    .error_fg = COLOR_DEFAULT,
				    .warning_fg = COLOR_DEFAULT,
				    .info_fg = COLOR_DEFAULT,
				    .value_fg = COLOR_DEFAULT,
				    .progress_fg = COLOR_DEFAULT,
				    .progress_bg = COLOR_DEFAULT,
				    .tab_active_fg = COLOR_BLACK,
				    .tab_active_bg = COLOR_WHITE,
				    .tab_inactive_fg = COLOR_DEFAULT,
				    .status_fg = COLOR_DEFAULT,
				    .status_bg = COLOR_DEFAULT };

/* ─────────────────────────────────────────────────────────────────────────────
 * ANSI color code mapping
 * ───────────────────────────────────────────────────────────────────────────── */

static int fg_code(screen_color_t c)
{
	switch (c) {
	case COLOR_DEFAULT:
		return 39;
	case COLOR_BLACK:
		return 30;
	case COLOR_RED:
		return 31;
	case COLOR_GREEN:
		return 32;
	case COLOR_YELLOW:
		return 33;
	case COLOR_BLUE:
		return 34;
	case COLOR_MAGENTA:
		return 35;
	case COLOR_CYAN:
		return 36;
	case COLOR_WHITE:
		return 37;
	case COLOR_BRIGHT_BLACK:
		return 90;
	case COLOR_BRIGHT_RED:
		return 91;
	case COLOR_BRIGHT_GREEN:
		return 92;
	case COLOR_BRIGHT_YELLOW:
		return 93;
	case COLOR_BRIGHT_BLUE:
		return 94;
	case COLOR_BRIGHT_MAGENTA:
		return 95;
	case COLOR_BRIGHT_CYAN:
		return 96;
	case COLOR_BRIGHT_WHITE:
		return 97;
	default:
		return 39;
	}
}

static int bg_code(screen_color_t c)
{
	switch (c) {
	case COLOR_DEFAULT:
		return 49;
	case COLOR_BLACK:
		return 40;
	case COLOR_RED:
		return 41;
	case COLOR_GREEN:
		return 42;
	case COLOR_YELLOW:
		return 43;
	case COLOR_BLUE:
		return 44;
	case COLOR_MAGENTA:
		return 45;
	case COLOR_CYAN:
		return 46;
	case COLOR_WHITE:
		return 47;
	case COLOR_BRIGHT_BLACK:
		return 100;
	case COLOR_BRIGHT_RED:
		return 101;
	case COLOR_BRIGHT_GREEN:
		return 102;
	case COLOR_BRIGHT_YELLOW:
		return 103;
	case COLOR_BRIGHT_BLUE:
		return 104;
	case COLOR_BRIGHT_MAGENTA:
		return 105;
	case COLOR_BRIGHT_CYAN:
		return 106;
	case COLOR_BRIGHT_WHITE:
		return 107;
	default:
		return 49;
	}
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Screen functions
 * ───────────────────────────────────────────────────────────────────────────── */

void screen_init(screen_t *scr, int width, int height)
{
	if (width > SCREEN_MAX_WIDTH)
		width = SCREEN_MAX_WIDTH;
	if (height > SCREEN_MAX_HEIGHT)
		height = SCREEN_MAX_HEIGHT;

	scr->width = width;
	scr->height = height;
	scr->cursor_row = 0;
	scr->cursor_col = 0;
	scr->current_fg = COLOR_DEFAULT;
	scr->current_bg = COLOR_DEFAULT;
	scr->current_attr = ATTR_NONE;
	scr->theme = &THEME_DEFAULT;

	screen_clear(scr);
}

void screen_set_theme(screen_t *scr, const screen_theme_t *theme)
{
	if (scr && theme)
		scr->theme = theme;
}

void screen_clear(screen_t *scr)
{
	for (int r = 0; r < scr->height; r++) {
		memset(scr->cells[r], ' ', scr->width);
		scr->cells[r][scr->width] = '\0';
		for (int c = 0; c < scr->width; c++) {
			scr->colors[r][c] =
				MAKE_COLOR(COLOR_DEFAULT, COLOR_DEFAULT);
			scr->attrs[r][c] = ATTR_NONE;
		}
	}
	scr->cursor_row = 0;
	scr->cursor_col = 0;
	scr->current_fg = COLOR_DEFAULT;
	scr->current_bg = COLOR_DEFAULT;
	scr->current_attr = ATTR_NONE;
}

void screen_move(screen_t *scr, int row, int col)
{
	if (row >= 0 && row < scr->height)
		scr->cursor_row = row;
	if (col >= 0 && col < scr->width)
		scr->cursor_col = col;
}

void screen_set_color(screen_t *scr, screen_color_t fg, screen_color_t bg)
{
	scr->current_fg = fg;
	scr->current_bg = bg;
}

void screen_set_fg(screen_t *scr, screen_color_t fg)
{
	scr->current_fg = fg;
}

void screen_set_bg(screen_t *scr, screen_color_t bg)
{
	scr->current_bg = bg;
}

void screen_set_attr(screen_t *scr, screen_attr_t attr)
{
	scr->current_attr = attr;
}

void screen_reset_color(screen_t *scr)
{
	scr->current_fg = COLOR_DEFAULT;
	scr->current_bg = COLOR_DEFAULT;
	scr->current_attr = ATTR_NONE;
}

void screen_putc(screen_t *scr, char c)
{
	if (c == '\n') {
		scr->cursor_row++;
		scr->cursor_col = 0;
		return;
	}

	if (scr->cursor_row < scr->height && scr->cursor_col < scr->width) {
		scr->cells[scr->cursor_row][scr->cursor_col] = c;
		scr->colors[scr->cursor_row][scr->cursor_col] =
			MAKE_COLOR(scr->current_fg, scr->current_bg);
		scr->attrs[scr->cursor_row][scr->cursor_col] =
			scr->current_attr;
		scr->cursor_col++;
	}
}

void screen_print(screen_t *scr, const char *str)
{
	while (*str) {
		screen_putc(scr, *str);
		str++;
	}
}

void screen_printf(screen_t *scr, const char *fmt, ...)
{
	char buf[512];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	screen_print(scr, buf);
}

void screen_fill_line(screen_t *scr)
{
	while (scr->cursor_col < scr->width) {
		scr->cells[scr->cursor_row][scr->cursor_col] = ' ';
		scr->colors[scr->cursor_row][scr->cursor_col] =
			MAKE_COLOR(scr->current_fg, scr->current_bg);
		scr->attrs[scr->cursor_row][scr->cursor_col] =
			scr->current_attr;
		scr->cursor_col++;
	}
}

void screen_hline(screen_t *scr, int row, int col, int len, char c)
{
	screen_move(scr, row, col);
	for (int i = 0; i < len && scr->cursor_col < scr->width; i++) {
		screen_putc(scr, c);
	}
}

void screen_vline(screen_t *scr, int row, int col, int len, char c)
{
	for (int i = 0; i < len && row + i < scr->height; i++) {
		scr->cells[row + i][col] = c;
		scr->colors[row + i][col] =
			MAKE_COLOR(scr->current_fg, scr->current_bg);
		scr->attrs[row + i][col] = scr->current_attr;
	}
}

void screen_box(screen_t *scr, int row, int col, int width, int height)
{
	/* Corners */
	if (row < scr->height && col < scr->width) {
		scr->cells[row][col] = BOX_TL;
		scr->colors[row][col] =
			MAKE_COLOR(scr->current_fg, scr->current_bg);
	}
	if (row < scr->height && col + width - 1 < scr->width) {
		scr->cells[row][col + width - 1] = BOX_TR;
		scr->colors[row][col + width - 1] =
			MAKE_COLOR(scr->current_fg, scr->current_bg);
	}
	if (row + height - 1 < scr->height && col < scr->width) {
		scr->cells[row + height - 1][col] = BOX_BL;
		scr->colors[row + height - 1][col] =
			MAKE_COLOR(scr->current_fg, scr->current_bg);
	}
	if (row + height - 1 < scr->height && col + width - 1 < scr->width) {
		scr->cells[row + height - 1][col + width - 1] = BOX_BR;
		scr->colors[row + height - 1][col + width - 1] =
			MAKE_COLOR(scr->current_fg, scr->current_bg);
	}

	/* Top and bottom edges */
	for (int c = col + 1; c < col + width - 1 && c < scr->width; c++) {
		if (row < scr->height) {
			scr->cells[row][c] = BOX_H;
			scr->colors[row][c] =
				MAKE_COLOR(scr->current_fg, scr->current_bg);
		}
		if (row + height - 1 < scr->height) {
			scr->cells[row + height - 1][c] = BOX_H;
			scr->colors[row + height - 1][c] =
				MAKE_COLOR(scr->current_fg, scr->current_bg);
		}
	}

	/* Left and right edges */
	for (int r = row + 1; r < row + height - 1 && r < scr->height; r++) {
		if (col < scr->width) {
			scr->cells[r][col] = BOX_V;
			scr->colors[r][col] =
				MAKE_COLOR(scr->current_fg, scr->current_bg);
		}
		if (col + width - 1 < scr->width) {
			scr->cells[r][col + width - 1] = BOX_V;
			scr->colors[r][col + width - 1] =
				MAKE_COLOR(scr->current_fg, scr->current_bg);
		}
	}
}

void screen_render(screen_t *scr)
{
	/* Build output buffer with absolute positioning and colors */
	char out[SCREEN_MAX_HEIGHT * (SCREEN_MAX_WIDTH * 20 + 32)];
	int pos = 0;

	/* Hide cursor and move home */
	pos += snprintf(out + pos, sizeof(out) - pos, "\033[?25l\033[H");

	screen_color_t last_fg = COLOR_DEFAULT;
	screen_color_t last_bg = COLOR_DEFAULT;
	screen_attr_t last_attr = ATTR_NONE;

	for (int r = 0; r < scr->height; r++) {
		/* Move to row, column 1 */
		pos += snprintf(out + pos, sizeof(out) - pos, "\033[%d;1H",
				r + 1);

		for (int c = 0; c < scr->width; c++) {
			screen_color_t fg = GET_FG(scr->colors[r][c]);
			screen_color_t bg = GET_BG(scr->colors[r][c]);
			screen_attr_t attr = scr->attrs[r][c];

			/* Output color change if needed */
			if (fg != last_fg || bg != last_bg ||
			    attr != last_attr) {
				pos += snprintf(out + pos, sizeof(out) - pos,
						"\033[0");

				if (attr & ATTR_BOLD)
					pos += snprintf(out + pos,
							sizeof(out) - pos,
							";1");
				if (attr & ATTR_DIM)
					pos += snprintf(out + pos,
							sizeof(out) - pos,
							";2");
				if (attr & ATTR_UNDERLINE)
					pos += snprintf(out + pos,
							sizeof(out) - pos,
							";4");
				if (attr & ATTR_REVERSE)
					pos += snprintf(out + pos,
							sizeof(out) - pos,
							";7");

				pos += snprintf(out + pos, sizeof(out) - pos,
						";%d;%dm", fg_code(fg),
						bg_code(bg));

				last_fg = fg;
				last_bg = bg;
				last_attr = attr;
			}

			out[pos++] = scr->cells[r][c];
		}

		/* Clear to end of line */
		pos += snprintf(out + pos, sizeof(out) - pos, "\033[K");
	}

	/* Reset colors and show cursor */
	pos += snprintf(out + pos, sizeof(out) - pos, "\033[0m\033[?25h");

	/* Write all at once - ignore return (best effort output) */
	if (write(STDOUT_FILENO, out, pos) <
	    0) { /* suppress warn_unused_result */
	}
}
