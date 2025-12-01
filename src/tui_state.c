/*
 * tui_state.c - Application state machine for interactive TUI
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

#include <stdlib.h>
#include <string.h>
#include "tui_state.h"
#include "tty.h"

/* String constants for display */
static const char *profile_names[] = { "SD", "HD", "FULLHD", "2K",
				       "4K", "8K", "Custom" };

static const char *test_type_names[] = { "Write", "Read", "Empty",
					 "Streaming" };

static const char *access_order_names[] = { "Normal", "Reverse", "Random" };

const char *tui_profile_name(tui_profile_t profile)
{
	if (profile < TUI_PROFILE_COUNT)
		return profile_names[profile];
	return "Unknown";
}

const char *tui_test_type_name(tui_test_type_t type)
{
	if (type < TUI_TEST_TYPE_COUNT)
		return test_type_names[type];
	return "Unknown";
}

const char *tui_access_order_name(tui_access_order_t order)
{
	if (order < TUI_ACCESS_ORDER_COUNT)
		return access_order_names[order];
	return "Unknown";
}

void tui_config_init(tui_test_config_t *config)
{
	if (!config)
		return;

	memset(config, 0, sizeof(*config));
	strncpy(config->path, "/tmp/vframetest", sizeof(config->path) - 1);
	config->test_type = TUI_TEST_WRITE;
	config->profile = TUI_PROFILE_FULLHD;
	config->custom_width = 1920;
	config->custom_height = 1080;
	config->custom_bpp = 10;
	config->threads = 1;
	config->frames = 1800;
	config->fps = 0; /* Unlimited */
	config->access_order = TUI_ACCESS_NORMAL;
	config->header_size = 65536;
	config->auto_cleanup = 1; /* Auto-cleanup by default */
	config->open_dashboard = 0; /* Don't open dashboard by default */
}

int tui_state_init(tui_app_state_t *state, size_t history_capacity)
{
	if (!state)
		return -1;

	memset(state, 0, sizeof(*state));

	/* Set defaults - start in config view in IDLE state */
	state->current_view = TUI_VIEW_CONFIG;
	state->run_state = TUI_STATE_IDLE;
	state->show_help = 0;
	state->needs_redraw = 1;
	state->selected_field = TUI_CONFIG_PATH;
	state->editing_text = 0;
	state->start_test_requested = 0;

	/* Initialize test config with defaults */
	tui_config_init(&state->config);

	/* Get initial terminal size */
	tty_size_t size = tty_get_size();
	state->term_width = size.width;
	state->term_height = size.height;

	/* Initialize history */
	if (history_capacity == 0)
		history_capacity = TUI_HISTORY_DEFAULT;
	if (history_capacity < TUI_HISTORY_MIN)
		history_capacity = TUI_HISTORY_MIN;
	if (history_capacity > TUI_HISTORY_MAX)
		history_capacity = TUI_HISTORY_MAX;

	state->history.frames =
		calloc(history_capacity, sizeof(tui_frame_record_t));
	if (!state->history.frames)
		return -1;

	state->history.capacity = history_capacity;
	state->history.head = 0;
	state->history.count = 0;
	if (pthread_mutex_init(&state->history.mutex, NULL) != 0) {
		free(state->history.frames);
		state->history.frames = NULL;
		return -1;
	}

	/* Initialize pause synchronization */
	state->paused = 0;
	if (pthread_mutex_init(&state->pause_mutex, NULL) != 0) {
		pthread_mutex_destroy(&state->history.mutex);
		free(state->history.frames);
		state->history.frames = NULL;
		return -1;
	}
	if (pthread_cond_init(&state->pause_cond, NULL) != 0) {
		pthread_mutex_destroy(&state->pause_mutex);
		pthread_mutex_destroy(&state->history.mutex);
		free(state->history.frames);
		state->history.frames = NULL;
		return -1;
	}

	return 0;
}

void tui_state_cleanup(tui_app_state_t *state)
{
	if (!state)
		return;

	/* Wake up any paused threads before cleanup */
	pthread_mutex_lock(&state->pause_mutex);
	state->paused = 0;
	state->run_state = TUI_STATE_QUITTING;
	pthread_cond_broadcast(&state->pause_cond);
	pthread_mutex_unlock(&state->pause_mutex);

	/* Free history */
	pthread_mutex_destroy(&state->history.mutex);
	free(state->history.frames);
	state->history.frames = NULL;

	/* Cleanup pause synchronization */
	pthread_mutex_destroy(&state->pause_mutex);
	pthread_cond_destroy(&state->pause_cond);
}

void tui_state_set_view(tui_app_state_t *state, tui_view_t view)
{
	if (!state || view >= TUI_VIEW_COUNT)
		return;

	if (state->current_view != view) {
		state->current_view = view;
		state->needs_redraw = 1;
	}
}

int tui_state_toggle_pause(tui_app_state_t *state)
{
	if (!state)
		return 0;

	pthread_mutex_lock(&state->pause_mutex);

	if (state->run_state == TUI_STATE_RUNNING) {
		state->run_state = TUI_STATE_PAUSED;
		state->paused = 1;
	} else if (state->run_state == TUI_STATE_PAUSED) {
		state->run_state = TUI_STATE_RUNNING;
		state->paused = 0;
		pthread_cond_broadcast(&state->pause_cond);
	}
	/* Don't change state if QUITTING or COMPLETED */

	int result = state->paused;
	pthread_mutex_unlock(&state->pause_mutex);

	state->needs_redraw = 1;
	return result;
}

void tui_state_request_quit(tui_app_state_t *state)
{
	if (!state)
		return;

	pthread_mutex_lock(&state->pause_mutex);
	state->run_state = TUI_STATE_QUITTING;
	state->paused = 0;
	pthread_cond_broadcast(&state->pause_cond);
	pthread_mutex_unlock(&state->pause_mutex);
}

void tui_state_set_completed(tui_app_state_t *state)
{
	if (!state)
		return;

	pthread_mutex_lock(&state->pause_mutex);
	if (state->run_state != TUI_STATE_QUITTING) {
		state->run_state = TUI_STATE_COMPLETED;
	}
	state->paused = 0;
	pthread_cond_broadcast(&state->pause_cond);
	pthread_mutex_unlock(&state->pause_mutex);

	state->needs_redraw = 1;
}

void tui_state_toggle_help(tui_app_state_t *state)
{
	if (!state)
		return;

	state->show_help = !state->show_help;
	state->needs_redraw = 1;
}

void tui_state_set_size(tui_app_state_t *state, int width, int height)
{
	if (!state)
		return;

	state->term_width = width;
	state->term_height = height;
	state->needs_redraw = 1;
}

void tui_history_add(tui_app_state_t *state, const tui_frame_record_t *frame)
{
	if (!state || !frame)
		return;

	pthread_mutex_lock(&state->history.mutex);

	/* Add to ring buffer */
	state->history.frames[state->history.head] = *frame;
	state->history.head =
		(state->history.head + 1) % state->history.capacity;

	if (state->history.count < state->history.capacity)
		state->history.count++;

	pthread_mutex_unlock(&state->history.mutex);

	state->needs_redraw = 1;
}

const tui_frame_record_t *tui_history_get(tui_app_state_t *state, size_t index)
{
	if (!state || index >= state->history.count)
		return NULL;

	/* Calculate actual index in ring buffer */
	/* index 0 = oldest, count-1 = newest */
	size_t start;
	if (state->history.count < state->history.capacity) {
		start = 0;
	} else {
		start = state->history.head;
	}

	size_t actual = (start + index) % state->history.capacity;
	return &state->history.frames[actual];
}

size_t tui_history_count(tui_app_state_t *state)
{
	if (!state)
		return 0;
	return state->history.count;
}

void tui_history_scroll_up(tui_app_state_t *state, size_t lines)
{
	if (!state)
		return;

	if (state->history_scroll_pos >= lines)
		state->history_scroll_pos -= lines;
	else
		state->history_scroll_pos = 0;

	state->needs_redraw = 1;
}

void tui_history_scroll_down(tui_app_state_t *state, size_t lines)
{
	if (!state)
		return;

	size_t count = tui_history_count(state);
	size_t max_scroll = count > 0 ? count - 1 : 0;

	state->history_scroll_pos += lines;
	if (state->history_scroll_pos > max_scroll)
		state->history_scroll_pos = max_scroll;

	state->needs_redraw = 1;
}

void tui_history_scroll_top(tui_app_state_t *state)
{
	if (!state)
		return;

	state->history_scroll_pos = 0;
	state->needs_redraw = 1;
}

void tui_history_scroll_bottom(tui_app_state_t *state)
{
	if (!state)
		return;

	size_t count = tui_history_count(state);
	state->history_scroll_pos = count > 0 ? count - 1 : 0;
	state->needs_redraw = 1;
}

void tui_history_page_up(tui_app_state_t *state)
{
	if (!state)
		return;

	/* Page size based on visible lines (estimate) */
	size_t page_size =
		state->term_height > 10 ? (size_t)(state->term_height - 10) : 5;
	tui_history_scroll_up(state, page_size);
}

void tui_history_page_down(tui_app_state_t *state)
{
	if (!state)
		return;

	size_t page_size =
		state->term_height > 10 ? (size_t)(state->term_height - 10) : 5;
	tui_history_scroll_down(state, page_size);
}

void tui_history_toggle_filter(tui_app_state_t *state)
{
	if (!state)
		return;

	state->history_filter_failures = !state->history_filter_failures;
	state->history_scroll_pos = 0; /* Reset scroll on filter change */
	state->needs_redraw = 1;
}

void tui_history_toggle_details(tui_app_state_t *state)
{
	if (!state)
		return;

	state->history_show_details = !state->history_show_details;
	state->needs_redraw = 1;
}

void tui_check_pause(tui_app_state_t *state)
{
	if (!state)
		return;

	pthread_mutex_lock(&state->pause_mutex);

	while (state->paused && state->run_state == TUI_STATE_PAUSED) {
		pthread_cond_wait(&state->pause_cond, &state->pause_mutex);
	}

	pthread_mutex_unlock(&state->pause_mutex);
}

int tui_should_quit(tui_app_state_t *state)
{
	if (!state)
		return 1;

	return state->run_state == TUI_STATE_QUITTING;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Config menu functions
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_config_next_field(tui_app_state_t *state)
{
	if (!state || state->editing_text)
		return;

	state->selected_field =
		(state->selected_field + 1) % TUI_CONFIG_FIELD_COUNT;
	state->needs_redraw = 1;
}

void tui_config_prev_field(tui_app_state_t *state)
{
	if (!state || state->editing_text)
		return;

	if (state->selected_field == 0)
		state->selected_field = TUI_CONFIG_FIELD_COUNT - 1;
	else
		state->selected_field--;
	state->needs_redraw = 1;
}

void tui_config_next_option(tui_app_state_t *state)
{
	if (!state || state->editing_text)
		return;

	switch (state->selected_field) {
	case TUI_CONFIG_TEST_TYPE:
		state->config.test_type =
			(state->config.test_type + 1) % TUI_TEST_TYPE_COUNT;
		break;
	case TUI_CONFIG_PROFILE:
		state->config.profile =
			(state->config.profile + 1) % TUI_PROFILE_COUNT;
		break;
	case TUI_CONFIG_ACCESS_ORDER:
		state->config.access_order = (state->config.access_order + 1) %
					     TUI_ACCESS_ORDER_COUNT;
		break;
	case TUI_CONFIG_AUTO_CLEANUP:
		state->config.auto_cleanup = !state->config.auto_cleanup;
		break;
	case TUI_CONFIG_OPEN_DASHBOARD:
		state->config.open_dashboard = !state->config.open_dashboard;
		break;
	default:
		/* For numeric fields, increment */
		tui_config_increment(state);
		return;
	}
	state->needs_redraw = 1;
}

void tui_config_prev_option(tui_app_state_t *state)
{
	if (!state || state->editing_text)
		return;

	switch (state->selected_field) {
	case TUI_CONFIG_TEST_TYPE:
		if (state->config.test_type == 0)
			state->config.test_type = TUI_TEST_TYPE_COUNT - 1;
		else
			state->config.test_type--;
		break;
	case TUI_CONFIG_PROFILE:
		if (state->config.profile == 0)
			state->config.profile = TUI_PROFILE_COUNT - 1;
		else
			state->config.profile--;
		break;
	case TUI_CONFIG_ACCESS_ORDER:
		if (state->config.access_order == 0)
			state->config.access_order = TUI_ACCESS_ORDER_COUNT - 1;
		else
			state->config.access_order--;
		break;
	case TUI_CONFIG_AUTO_CLEANUP:
		state->config.auto_cleanup = !state->config.auto_cleanup;
		break;
	case TUI_CONFIG_OPEN_DASHBOARD:
		state->config.open_dashboard = !state->config.open_dashboard;
		break;
	default:
		/* For numeric fields, decrement */
		tui_config_decrement(state);
		return;
	}
	state->needs_redraw = 1;
}

void tui_config_increment(tui_app_state_t *state)
{
	if (!state || state->editing_text)
		return;

	switch (state->selected_field) {
	case TUI_CONFIG_THREADS:
		if (state->config.threads < 64)
			state->config.threads++;
		break;
	case TUI_CONFIG_FRAMES:
		if (state->config.frames < 1000000) {
			if (state->config.frames < 100)
				state->config.frames += 10;
			else if (state->config.frames < 1000)
				state->config.frames += 100;
			else
				state->config.frames += 1000;
		}
		break;
	case TUI_CONFIG_FPS:
		if (state->config.fps < 1000) {
			if (state->config.fps < 60)
				state->config.fps++;
			else
				state->config.fps += 10;
		}
		break;
	case TUI_CONFIG_HEADER_SIZE:
		if (state->config.header_size < 1048576)
			state->config.header_size *= 2;
		break;
	default:
		return;
	}
	state->needs_redraw = 1;
}

void tui_config_decrement(tui_app_state_t *state)
{
	if (!state || state->editing_text)
		return;

	switch (state->selected_field) {
	case TUI_CONFIG_THREADS:
		if (state->config.threads > 1)
			state->config.threads--;
		break;
	case TUI_CONFIG_FRAMES:
		if (state->config.frames > 1) {
			if (state->config.frames <= 100)
				state->config.frames -= 10;
			else if (state->config.frames <= 1000)
				state->config.frames -= 100;
			else
				state->config.frames -= 1000;
			if (state->config.frames < 1)
				state->config.frames = 1;
		}
		break;
	case TUI_CONFIG_FPS:
		if (state->config.fps > 0) {
			if (state->config.fps <= 60)
				state->config.fps--;
			else
				state->config.fps -= 10;
		}
		break;
	case TUI_CONFIG_HEADER_SIZE:
		if (state->config.header_size > 512)
			state->config.header_size /= 2;
		break;
	default:
		return;
	}
	state->needs_redraw = 1;
}

void tui_config_start_edit(tui_app_state_t *state)
{
	if (!state)
		return;

	if (state->selected_field == TUI_CONFIG_PATH) {
		state->editing_text = 1;
		strncpy(state->edit_buffer, state->config.path,
			sizeof(state->edit_buffer) - 1);
		state->edit_buffer[sizeof(state->edit_buffer) - 1] = '\0';
		state->edit_cursor = strlen(state->edit_buffer);
		state->needs_redraw = 1;
	}
}

void tui_config_end_edit(tui_app_state_t *state, int save)
{
	if (!state || !state->editing_text)
		return;

	if (save) {
		strncpy(state->config.path, state->edit_buffer,
			sizeof(state->config.path) - 1);
		state->config.path[sizeof(state->config.path) - 1] = '\0';
	}

	state->editing_text = 0;
	state->edit_buffer[0] = '\0';
	state->edit_cursor = 0;
	state->needs_redraw = 1;
}

void tui_config_edit_char(tui_app_state_t *state, char c)
{
	if (!state || !state->editing_text)
		return;

	size_t len = strlen(state->edit_buffer);
	if (len < sizeof(state->edit_buffer) - 1 && c >= 32 && c < 127) {
		state->edit_buffer[state->edit_cursor] = c;
		state->edit_cursor++;
		state->edit_buffer[state->edit_cursor] = '\0';
		state->needs_redraw = 1;
	}
}

void tui_config_edit_backspace(tui_app_state_t *state)
{
	if (!state || !state->editing_text)
		return;

	if (state->edit_cursor > 0) {
		state->edit_cursor--;
		state->edit_buffer[state->edit_cursor] = '\0';
		state->needs_redraw = 1;
	}
}

int tui_config_start_test(tui_app_state_t *state)
{
	if (!state)
		return 0;

	/* Validate configuration */
	if (state->config.path[0] == '\0')
		return 0; /* Path is required */

	if (state->config.threads < 1 || state->config.threads > 64)
		return 0;

	if (state->config.frames < 1)
		return 0;

	/* Mark test as requested */
	state->start_test_requested = 1;
	state->needs_redraw = 1;

	return 1;
}

int tui_config_test_requested(tui_app_state_t *state)
{
	if (!state)
		return 0;

	if (state->start_test_requested) {
		state->start_test_requested = 0;
		return 1;
	}
	return 0;
}

void tui_state_reset_for_test(tui_app_state_t *state)
{
	if (!state)
		return;

	/* Clear history */
	pthread_mutex_lock(&state->history.mutex);
	state->history.head = 0;
	state->history.count = 0;
	pthread_mutex_unlock(&state->history.mutex);

	/* Reset view state */
	state->history_scroll_pos = 0;
	state->history_selected = 0;
	state->history_filter_failures = 0;
	state->history_show_details = 0;
	state->latency_zoom = 0;
	state->latency_pan_offset = 0;

	state->needs_redraw = 1;
}

void tui_state_set_running(tui_app_state_t *state)
{
	if (!state)
		return;

	pthread_mutex_lock(&state->pause_mutex);
	state->run_state = TUI_STATE_RUNNING;
	state->paused = 0;
	pthread_mutex_unlock(&state->pause_mutex);

	/* Switch to dashboard view */
	state->current_view = TUI_VIEW_DASHBOARD;
	state->needs_redraw = 1;
}
