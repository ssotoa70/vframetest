/*
 * tui_state.h - Application state machine for interactive TUI
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

#ifndef FRAMETEST_TUI_STATE_H
#define FRAMETEST_TUI_STATE_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include "frametest.h"

/* Default and limits for frame history */
#define TUI_HISTORY_DEFAULT 10000
#define TUI_HISTORY_MIN 100
#define TUI_HISTORY_MAX 1000000

/* Views */
typedef enum tui_view_t {
	TUI_VIEW_DASHBOARD = 0,
	TUI_VIEW_HISTORY,
	TUI_VIEW_LATENCY,
	TUI_VIEW_CONFIG, /* Test configuration/start menu */
	TUI_VIEW_COUNT
} tui_view_t;

/* Test type for configuration */
typedef enum tui_test_type_t {
	TUI_TEST_WRITE = 0,
	TUI_TEST_READ,
	TUI_TEST_EMPTY,
	TUI_TEST_STREAMING,
	TUI_TEST_TYPE_COUNT
} tui_test_type_t;

/* Access order for configuration */
typedef enum tui_access_order_t {
	TUI_ACCESS_NORMAL = 0,
	TUI_ACCESS_REVERSE,
	TUI_ACCESS_RANDOM,
	TUI_ACCESS_ORDER_COUNT
} tui_access_order_t;

/* Profile type for configuration */
typedef enum tui_profile_t {
	TUI_PROFILE_SD = 0,
	TUI_PROFILE_HD,
	TUI_PROFILE_FULLHD,
	TUI_PROFILE_2K,
	TUI_PROFILE_4K,
	TUI_PROFILE_8K,
	TUI_PROFILE_CUSTOM,
	TUI_PROFILE_COUNT
} tui_profile_t;

/* Config menu field indices */
typedef enum tui_config_field_t {
	TUI_CONFIG_PATH = 0,
	TUI_CONFIG_TEST_TYPE,
	TUI_CONFIG_PROFILE,
	TUI_CONFIG_THREADS,
	TUI_CONFIG_FRAMES,
	TUI_CONFIG_FPS,
	TUI_CONFIG_ACCESS_ORDER,
	TUI_CONFIG_HEADER_SIZE,
	TUI_CONFIG_AUTO_CLEANUP,
	TUI_CONFIG_OPEN_DASHBOARD,
	TUI_CONFIG_START_BUTTON,
	TUI_CONFIG_FIELD_COUNT
} tui_config_field_t;

/* Test configuration structure */
typedef struct tui_test_config_t {
	char path[512];
	tui_test_type_t test_type;
	tui_profile_t profile;
	size_t custom_width;
	size_t custom_height;
	size_t custom_bpp;
	size_t threads;
	size_t frames;
	size_t fps; /* 0 = unlimited */
	tui_access_order_t access_order;
	size_t header_size;
	int auto_cleanup; /* 1 = auto-cleanup after test, 0 = prompt */
	int open_dashboard; /* 1 = open dashboard in browser after test */
} tui_test_config_t;

/* Run states */
typedef enum tui_run_state_t {
	TUI_STATE_IDLE, /* Before test starts - in config menu */
	TUI_STATE_RUNNING, /* Test is executing */
	TUI_STATE_PAUSED, /* Test is paused */
	TUI_STATE_COMPLETED, /* Test finished */
	TUI_STATE_QUITTING /* User requested quit */
} tui_run_state_t;

/* Single frame record in history */
typedef struct tui_frame_record_t {
	size_t frame_num;
	uint64_t start_ns;
	uint64_t duration_ns;
	size_t bytes;
	io_mode_t io_mode;
	int success;
	int thread_id;
} tui_frame_record_t;

/* Frame history ring buffer */
typedef struct tui_history_t {
	tui_frame_record_t *frames;
	size_t capacity;
	size_t head; /* Next write position */
	size_t count; /* Number of frames stored */
	pthread_mutex_t mutex;
} tui_history_t;

/* Application state */
typedef struct tui_app_state_t {
	/* Current view and run state */
	tui_view_t current_view;
	tui_run_state_t run_state;
	int show_help;

	/* Terminal dimensions */
	int term_width;
	int term_height;

	/* Test configuration (Config view) */
	tui_test_config_t config;
	tui_config_field_t selected_field;
	int editing_text; /* Currently editing a text field */
	char edit_buffer[512]; /* Buffer for text editing */
	size_t edit_cursor; /* Cursor position in edit buffer */

	/* History view state */
	size_t history_scroll_pos;
	size_t history_selected;
	int history_filter_failures;
	int history_show_details;

	/* Latency view state */
	int latency_zoom;
	int latency_pan_offset;

	/* Frame history */
	tui_history_t history;

	/* Pause synchronization */
	volatile int paused;
	pthread_mutex_t pause_mutex;
	pthread_cond_t pause_cond;

	/* Dirty flag for rendering */
	int needs_redraw;

	/* Flag to signal test should start */
	volatile int start_test_requested;
} tui_app_state_t;

/*
 * Initialize application state
 * history_capacity: number of frames to keep in history (0 = use default)
 */
int tui_state_init(tui_app_state_t *state, size_t history_capacity);

/*
 * Cleanup application state
 */
void tui_state_cleanup(tui_app_state_t *state);

/*
 * Switch to a different view
 */
void tui_state_set_view(tui_app_state_t *state, tui_view_t view);

/*
 * Toggle pause state
 * Returns new pause state (1 = paused, 0 = running)
 */
int tui_state_toggle_pause(tui_app_state_t *state);

/*
 * Request quit
 */
void tui_state_request_quit(tui_app_state_t *state);

/*
 * Mark test as completed
 */
void tui_state_set_completed(tui_app_state_t *state);

/*
 * Toggle help overlay
 */
void tui_state_toggle_help(tui_app_state_t *state);

/*
 * Update terminal dimensions
 */
void tui_state_set_size(tui_app_state_t *state, int width, int height);

/*
 * Add a frame to history (thread-safe)
 */
void tui_history_add(tui_app_state_t *state, const tui_frame_record_t *frame);

/*
 * Get frame from history by index (0 = oldest, count-1 = newest)
 * Returns NULL if index out of range
 */
const tui_frame_record_t *tui_history_get(tui_app_state_t *state, size_t index);

/*
 * Get number of frames in history
 */
size_t tui_history_count(tui_app_state_t *state);

/*
 * History navigation
 */
void tui_history_scroll_up(tui_app_state_t *state, size_t lines);
void tui_history_scroll_down(tui_app_state_t *state, size_t lines);
void tui_history_scroll_top(tui_app_state_t *state);
void tui_history_scroll_bottom(tui_app_state_t *state);
void tui_history_page_up(tui_app_state_t *state);
void tui_history_page_down(tui_app_state_t *state);
void tui_history_toggle_filter(tui_app_state_t *state);
void tui_history_toggle_details(tui_app_state_t *state);

/*
 * Check if worker threads should pause (call from worker thread)
 * Blocks until unpaused if paused
 */
void tui_check_pause(tui_app_state_t *state);

/*
 * Check if we should quit
 */
int tui_should_quit(tui_app_state_t *state);

/*
 * Initialize test configuration with defaults
 */
void tui_config_init(tui_test_config_t *config);

/*
 * Config menu navigation
 */
void tui_config_next_field(tui_app_state_t *state);
void tui_config_prev_field(tui_app_state_t *state);
void tui_config_next_option(tui_app_state_t *state);
void tui_config_prev_option(tui_app_state_t *state);
void tui_config_increment(tui_app_state_t *state);
void tui_config_decrement(tui_app_state_t *state);

/*
 * Text editing for path field
 */
void tui_config_start_edit(tui_app_state_t *state);
void tui_config_end_edit(tui_app_state_t *state, int save);
void tui_config_edit_char(tui_app_state_t *state, char c);
void tui_config_edit_backspace(tui_app_state_t *state);

/*
 * Request test start (from config menu)
 * Returns 1 if config is valid and test can start, 0 otherwise
 */
int tui_config_start_test(tui_app_state_t *state);

/*
 * Check if test start was requested (and clear the flag)
 */
int tui_config_test_requested(tui_app_state_t *state);

/*
 * Get profile name string
 */
const char *tui_profile_name(tui_profile_t profile);

/*
 * Get test type name string
 */
const char *tui_test_type_name(tui_test_type_t type);

/*
 * Get access order name string
 */
const char *tui_access_order_name(tui_access_order_t order);

/*
 * Reset state for a new test (clear history, reset counters)
 */
void tui_state_reset_for_test(tui_app_state_t *state);

/*
 * Set state to running (called when test starts)
 */
void tui_state_set_running(tui_app_state_t *state);

#endif /* FRAMETEST_TUI_STATE_H */
