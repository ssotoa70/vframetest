/*
 * tui_input.c - Keyboard input handling for interactive TUI
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

#include "tui_input.h"
#include "tty.h"

/* Handle global keys (work in all views) */
static int handle_global_key(tui_app_state_t *state, int key)
{
	switch (key) {
	case 'q':
	case 'Q':
	case TTY_KEY_CTRL_C:
	case TTY_KEY_CTRL_D:
		tui_state_request_quit(state);
		return 1;

	case 'p':
	case 'P':
		/* Only allow pause during test execution */
		if (state->run_state == TUI_STATE_RUNNING ||
		    state->run_state == TUI_STATE_PAUSED) {
			tui_state_toggle_pause(state);
		}
		return 1;

	case '1':
		tui_state_set_view(state, TUI_VIEW_DASHBOARD);
		return 1;

	case '2':
		tui_state_set_view(state, TUI_VIEW_HISTORY);
		return 1;

	case '3':
		tui_state_set_view(state, TUI_VIEW_LATENCY);
		return 1;

	case '4':
		tui_state_set_view(state, TUI_VIEW_CONFIG);
		return 1;

	case '?':
	case 'h':
	case 'H':
		tui_state_toggle_help(state);
		return 1;

	case TTY_KEY_ESCAPE:
		/* ESC closes help overlay, or does nothing */
		if (state->show_help) {
			state->show_help = 0;
			state->needs_redraw = 1;
			return 1;
		}
		return 0;

	default:
		return 0;
	}
}

/* Handle dashboard view keys */
static int handle_dashboard_key(tui_app_state_t *state, int key)
{
	(void)state;
	(void)key;
	/* Dashboard has no view-specific keys currently */
	return 0;
}

/* Handle history view keys */
static int handle_history_key(tui_app_state_t *state, int key)
{
	switch (key) {
	case TTY_KEY_UP:
	case 'k':
		tui_history_scroll_up(state, 1);
		return 1;

	case TTY_KEY_DOWN:
	case 'j':
		tui_history_scroll_down(state, 1);
		return 1;

	case TTY_KEY_PAGEUP:
		tui_history_page_up(state);
		return 1;

	case TTY_KEY_PAGEDOWN:
		tui_history_page_down(state);
		return 1;

	case 'g':
		tui_history_scroll_top(state);
		return 1;

	case 'G':
		tui_history_scroll_bottom(state);
		return 1;

	case TTY_KEY_HOME:
		tui_history_scroll_top(state);
		return 1;

	case TTY_KEY_END:
		tui_history_scroll_bottom(state);
		return 1;

	case 'f':
	case 'F':
		tui_history_toggle_filter(state);
		return 1;

	case TTY_KEY_ENTER:
		tui_history_toggle_details(state);
		return 1;

	default:
		return 0;
	}
}

/* Handle latency view keys */
static int handle_latency_key(tui_app_state_t *state, int key)
{
	switch (key) {
	case TTY_KEY_LEFT:
		if (state->latency_pan_offset > 0) {
			state->latency_pan_offset--;
			state->needs_redraw = 1;
		}
		return 1;

	case TTY_KEY_RIGHT:
		state->latency_pan_offset++;
		state->needs_redraw = 1;
		return 1;

	case 'z':
	case 'Z':
		/* Cycle through zoom levels */
		state->latency_zoom = (state->latency_zoom + 1) % 4;
		state->needs_redraw = 1;
		return 1;

	case 'r':
	case 'R':
		/* Reset pan */
		state->latency_pan_offset = 0;
		state->needs_redraw = 1;
		return 1;

	default:
		return 0;
	}
}

/* Handle config view keys */
static int handle_config_key(tui_app_state_t *state, int key)
{
	/* If editing text, handle text input */
	if (state->editing_text) {
		switch (key) {
		case TTY_KEY_ENTER:
			tui_config_end_edit(state, 1); /* Save */
			return 1;

		case TTY_KEY_ESCAPE:
			tui_config_end_edit(state, 0); /* Cancel */
			return 1;

		case TTY_KEY_BACKSPACE:
			tui_config_edit_backspace(state);
			return 1;

		default:
			/* Printable character */
			if (key >= 32 && key < 127) {
				tui_config_edit_char(state, (char)key);
				return 1;
			}
			return 0;
		}
	}

	/* Normal config navigation */
	switch (key) {
	case TTY_KEY_UP:
	case 'k':
		tui_config_prev_field(state);
		return 1;

	case TTY_KEY_DOWN:
	case 'j':
	case TTY_KEY_TAB:
		tui_config_next_field(state);
		return 1;

	case TTY_KEY_LEFT:
		tui_config_prev_option(state);
		return 1;

	case TTY_KEY_RIGHT:
		tui_config_next_option(state);
		return 1;

	case '+':
	case '=':
		tui_config_increment(state);
		return 1;

	case '-':
	case '_':
		tui_config_decrement(state);
		return 1;

	case TTY_KEY_ENTER:
		if (state->selected_field == TUI_CONFIG_PATH) {
			tui_config_start_edit(state);
		} else if (state->selected_field == TUI_CONFIG_START_BUTTON) {
			tui_config_start_test(state);
		}
		return 1;

	case 's':
	case 'S':
		/* Start test shortcut */
		tui_config_start_test(state);
		return 1;

	default:
		return 0;
	}
}

int tui_input_handle_key(tui_app_state_t *state, int key)
{
	if (!state || key == TTY_KEY_NONE || key == TTY_KEY_ERROR)
		return 0;

	/* Help overlay intercepts most keys */
	if (state->show_help) {
		/* Any key closes help */
		state->show_help = 0;
		state->needs_redraw = 1;
		return 1;
	}

	/* When editing text, skip global keys (except Ctrl+C/Ctrl+D for emergency quit) */
	if (state->editing_text) {
		if (key == TTY_KEY_CTRL_C || key == TTY_KEY_CTRL_D) {
			tui_state_request_quit(state);
			return 1;
		}
		/* Route directly to config handler for text input */
		return handle_config_key(state, key);
	}

	/* Try global keys first */
	if (handle_global_key(state, key))
		return 1;

	/* Then view-specific keys */
	switch (state->current_view) {
	case TUI_VIEW_DASHBOARD:
		return handle_dashboard_key(state, key);

	case TUI_VIEW_HISTORY:
		return handle_history_key(state, key);

	case TUI_VIEW_LATENCY:
		return handle_latency_key(state, key);

	case TUI_VIEW_CONFIG:
		return handle_config_key(state, key);

	default:
		return 0;
	}
}

int tui_input_process(tui_app_state_t *state, int timeout_ms)
{
	if (!state)
		return 1;

	/* Check for terminal signals */
	if (tty_should_quit()) {
		tui_state_request_quit(state);
		return 1;
	}

	/* Check for resize */
	if (tty_was_resized()) {
		tty_size_t size = tty_get_size();
		tui_state_set_size(state, size.width, size.height);
	}

	/* Read and process key */
	int key = tty_read_key(timeout_ms);

	if (key != TTY_KEY_NONE && key != TTY_KEY_ERROR) {
		tui_input_handle_key(state, key);
	}

	return tui_should_quit(state);
}
