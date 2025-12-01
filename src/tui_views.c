/*
 * tui_views.c - View implementations for interactive TUI
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

#include <stdio.h>
#include <string.h>
#include "tui_views.h"
#include "tty.h"

/* ─────────────────────────────────────────────────────────────────────────────
 * ANSI Color and formatting codes
 * ───────────────────────────────────────────────────────────────────────────── */

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RED "\033[31m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define REVERSE "\033[7m"

/* Box drawing characters */
#define BOX_TL "┌"
#define BOX_TR "┐"
#define BOX_BL "└"
#define BOX_BR "┘"
#define BOX_H "─"
#define BOX_V "│"
#define BOX_LT "├"
#define BOX_RT "┤"

/* Double box for config */
#define DBOX_TL "╔"
#define DBOX_TR "╗"
#define DBOX_BL "╚"
#define DBOX_BR "╝"
#define DBOX_H "═"
#define DBOX_V "║"

/* Status symbols */
#define SYM_CHECK "✓"
#define SYM_CROSS "✗"
#define SYM_RADIO_ON "◉"
#define SYM_RADIO_OFF "○"
#define SYM_ARROW "▶"

/* Progress bar */
#define PROG_FULL "█"
#define PROG_EMPTY "░"

/* Sparkline characters (8 levels) */
static const char *SPARK_CHARS[8] = { "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█" };

/* ─────────────────────────────────────────────────────────────────────────────
 * Helper functions
 * ───────────────────────────────────────────────────────────────────────────── */

static void print_pad(int n)
{
	while (n-- > 0)
		putchar(' ');
}

/* Print a horizontal line (border) */
static void print_hline(int width, const char *left, const char *mid,
			const char *right)
{
	printf("\r%s", left);
	for (int i = 0; i < width - 2; i++)
		printf("%s", mid);
	printf("%s\r\n", right);
}

/* Print an empty line */
static void print_empty_line(int width)
{
	printf("\r%s", BOX_V);
	print_pad(width - 2);
	printf("%s\r\n", BOX_V);
}

/* Print a dim separator line */
static void print_separator(int width)
{
	printf("\r%s%s", BOX_V, DIM);
	for (int i = 0; i < width - 2; i++)
		printf("%s", BOX_H);
	printf("%s%s\r\n", RESET, BOX_V);
}

static void print_centered(const char *text, int width)
{
	int len = (int)strlen(text);
	int pad_left = (width - len) / 2;
	int pad_right = width - len - pad_left;
	print_pad(pad_left);
	printf("%s", text);
	print_pad(pad_right);
}

/* Format nanoseconds as human readable */
static void format_latency(uint64_t ns, char *buf, size_t bufsize)
{
	double ms = (double)ns / 1000000.0;
	if (ns == 0 || ns == UINT64_MAX) {
		snprintf(buf, bufsize, "---");
	} else if (ms >= 1000.0) {
		snprintf(buf, bufsize, "%.1fs", ms / 1000.0);
	} else if (ms >= 1.0) {
		snprintf(buf, bufsize, "%.1fms", ms);
	} else {
		snprintf(buf, bufsize, "%.0fus", ms * 1000.0);
	}
}

/* Format bytes as human readable */
static void format_bytes(size_t bytes, char *buf, size_t bufsize)
{
	if (bytes >= 1073741824) {
		snprintf(buf, bufsize, "%.1f GB", (double)bytes / 1073741824.0);
	} else if (bytes >= 1048576) {
		snprintf(buf, bufsize, "%.1f MB", (double)bytes / 1048576.0);
	} else if (bytes >= 1024) {
		snprintf(buf, bufsize, "%.1f KB", (double)bytes / 1024.0);
	} else {
		snprintf(buf, bufsize, "%zu B", bytes);
	}
}

/* Get run state string */
static const char *get_run_state_str(tui_run_state_t state)
{
	switch (state) {
	case TUI_STATE_IDLE:
		return "IDLE";
	case TUI_STATE_RUNNING:
		return "LIVE";
	case TUI_STATE_PAUSED:
		return "PAUSED";
	case TUI_STATE_COMPLETED:
		return "DONE";
	case TUI_STATE_QUITTING:
		return "QUIT";
	default:
		return "???";
	}
}

/* Get run state color */
static const char *get_run_state_color(tui_run_state_t state)
{
	switch (state) {
	case TUI_STATE_RUNNING:
		return GREEN;
	case TUI_STATE_PAUSED:
		return YELLOW;
	case TUI_STATE_COMPLETED:
		return CYAN;
	default:
		return WHITE;
	}
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Tab bar rendering
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_render_tab_bar(tui_app_state_t *state, int width)
{
	const char *tabs[] = { "Dashboard", "History", "Latency", "Config" };
	const int tab_count = 4;

	/* Top border */
	print_hline(width, BOX_TL, BOX_H, BOX_TR);

	/* Build tab line content and count display columns */
	char line[256];
	int pos = 0;
	int cols = 0;

	for (int i = 0; i < tab_count; i++) {
		if (i == state->current_view) {
			pos += snprintf(line + pos, sizeof(line) - pos,
					"%s[%d] %s%s ", REVERSE, i + 1, tabs[i],
					RESET);
		} else {
			pos += snprintf(line + pos, sizeof(line) - pos,
					"[%d] %s ", i + 1, tabs[i]);
		}
		cols += 4 + (int)strlen(tabs[i]) + 1; /* "[N] Name " */
	}

	/* Add help hint */
	pos += snprintf(line + pos, sizeof(line) - pos, " [?] Help");
	cols += 10; /* " [?] Help" */

	/* Add status indicator */
	const char *status = get_run_state_str(state->run_state);
	int status_cols = 2 + (int)strlen(status); /* "[STATUS]" */

	/* Calculate padding: width - 2 (borders) - cols - status_cols */
	int pad = width - 2 - cols - status_cols;
	if (pad < 1)
		pad = 1;

	/* Print: │ <tabs> <pad> [STATUS]│ */
	printf("\r%s %s", BOX_V, line);
	print_pad(pad - 1);
	printf("%s[%s]%s%s\r\n", get_run_state_color(state->run_state), status,
	       RESET, BOX_V);

	/* Separator */
	print_hline(width, BOX_LT, BOX_H, BOX_RT);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Status bar rendering
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_render_status_bar(tui_app_state_t *state, int width)
{
	print_hline(width, BOX_LT, BOX_H, BOX_RT);

	/* Build hint text and count display columns */
	const char *hint;
	int hint_cols;

	switch (state->current_view) {
	case TUI_VIEW_CONFIG:
		if (state->editing_text) {
			hint = "[Enter] save  [Esc] cancel";
			hint_cols = 26;
		} else {
			hint = "[↑↓] navigate  [←→] change  [+/-] adjust  [s] start  [q] quit";
			hint_cols = 61; /* arrows are 1 col each */
		}
		break;

	case TUI_VIEW_DASHBOARD:
		hint = "[p] pause  [1-4] views  [q] quit";
		hint_cols = 32;
		break;

	case TUI_VIEW_HISTORY:
		hint = "[↑↓] scroll  [f] filter  [Enter] details  [q] quit";
		hint_cols = 50;
		break;

	case TUI_VIEW_LATENCY:
		hint = "[←→] pan  [z] zoom  [q] quit";
		hint_cols = 28;
		break;

	default:
		hint = "[q] quit";
		hint_cols = 8;
	}

	/* Print: │ <hint> <pad>│ */
	printf("\r%s %s", BOX_V, hint);
	print_pad(width - 3 - hint_cols);
	printf("%s\r\n", BOX_V);

	/* Bottom border */
	print_hline(width, BOX_BL, BOX_H, BOX_BR);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Config view rendering
 * ───────────────────────────────────────────────────────────────────────────── */

static void render_config_field_path(tui_app_state_t *state, int width,
				     int selected)
{
	const char *path;
	int path_len;

	if (state->editing_text) {
		path = state->edit_buffer;
		path_len =
			(int)strlen(state->edit_buffer) + 1; /* +1 for cursor */
	} else {
		path = state->config.path[0] ? state->config.path :
					       "(press Enter to set)";
		path_len = (int)strlen(path);
	}

	/* "│ Target Path   │ <path><pad>│" = 1+1+14+1+1+path+pad+1 = width */
	int pad = width - 19 - path_len;
	if (pad < 0)
		pad = 0;

	printf("\r%s ", BOX_V);
	if (selected)
		printf("%s", REVERSE);
	printf("Target Path   ");
	if (selected)
		printf("%s", RESET);
	printf("%s ", BOX_V);

	if (state->editing_text) {
		printf("%s%s%s_", CYAN, path, RESET);
	} else {
		printf("%s", path);
	}
	print_pad(pad);
	printf("%s\r\n", BOX_V);
}

static void render_config_field_radio(int width, tui_config_field_t field,
				      int selected, const char *label,
				      int current, int count)
{
	/* Count display columns for options */
	int cols = 0;
	for (int i = 0; i < count; i++) {
		const char *name;
		if (field == TUI_CONFIG_TEST_TYPE)
			name = tui_test_type_name((tui_test_type_t)i);
		else if (field == TUI_CONFIG_PROFILE)
			name = tui_profile_name((tui_profile_t)i);
		else
			name = tui_access_order_name((tui_access_order_t)i);
		cols += 1 + 1 + (int)strlen(name) +
			1; /* symbol + space + name + space */
	}

	/* "│ label          │ <options><pad>│" */
	int pad = width - 19 - cols;
	if (pad < 0)
		pad = 0;

	printf("\r%s ", BOX_V);
	if (selected)
		printf("%s", REVERSE);
	printf("%-14s", label);
	if (selected)
		printf("%s", RESET);
	printf("%s ", BOX_V);

	for (int i = 0; i < count; i++) {
		const char *name;
		if (field == TUI_CONFIG_TEST_TYPE)
			name = tui_test_type_name((tui_test_type_t)i);
		else if (field == TUI_CONFIG_PROFILE)
			name = tui_profile_name((tui_profile_t)i);
		else
			name = tui_access_order_name((tui_access_order_t)i);

		if (i == current) {
			printf("%s%s %s%s ", GREEN, SYM_RADIO_ON, name, RESET);
		} else {
			printf("%s %s ", SYM_RADIO_OFF, name);
		}
	}

	print_pad(pad);
	printf("%s\r\n", BOX_V);
}

static void render_config_field_numeric(int width, int selected,
					const char *label, size_t value,
					const char *suffix)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "[ %zu ]", value);

	int content_cols = (int)strlen(buf);
	if (suffix && suffix[0]) {
		content_cols += 1 + (int)strlen(suffix);
	}

	int pad = width - 19 - content_cols;
	if (pad < 0)
		pad = 0;

	printf("\r%s ", BOX_V);
	if (selected)
		printf("%s", REVERSE);
	printf("%-14s", label);
	if (selected)
		printf("%s", RESET);
	printf("%s ", BOX_V);

	printf("%s%s%s", CYAN, buf, RESET);
	if (suffix && suffix[0]) {
		printf(" %s", suffix);
	}

	print_pad(pad);
	printf("%s\r\n", BOX_V);
}

static void render_config_start_button(int width, int selected)
{
	/* Button text is 27 chars: "  [S] START TEST  [Enter]  " */
	int button_len = 27;
	int left_pad = (width - 2 - button_len) / 2;
	int right_pad = width - 2 - button_len - left_pad;

	printf("\r%s", BOX_V);
	print_pad(left_pad);

	if (selected) {
		printf("%s%s  [S] START TEST  [Enter]  %s", REVERSE GREEN, BOLD,
		       RESET);
	} else {
		printf("  [S] START TEST  [Enter]  ");
	}

	print_pad(right_pad);
	printf("%s\r\n", BOX_V);
}

void tui_view_config(tui_app_state_t *state, tui_metrics_t *metrics)
{
	(void)metrics;
	int width = state->term_width > 80 ? 80 : state->term_width;

	/* Empty line */
	print_empty_line(width);

	/* Title: "═══ Test Configuration ═══" = 26 display cols */
	int title_len = 26;
	int left_pad = (width - 2 - title_len) / 2;
	int right_pad = width - 2 - title_len - left_pad;
	printf("\r%s", BOX_V);
	print_pad(left_pad);
	printf("%s%s%s Test Configuration %s%s%s", DBOX_H DBOX_H DBOX_H, BOLD,
	       RESET, DBOX_H DBOX_H DBOX_H, "", "");
	print_pad(right_pad);
	printf("%s\r\n", BOX_V);

	/* Empty line */
	print_empty_line(width);

	/* Path field */
	render_config_field_path(state, width,
				 state->selected_field == TUI_CONFIG_PATH);

	/* Separator */
	print_separator(width);

	/* Test Type */
	render_config_field_radio(width, TUI_CONFIG_TEST_TYPE,
				  state->selected_field == TUI_CONFIG_TEST_TYPE,
				  "Test Type", (int)state->config.test_type,
				  TUI_TEST_TYPE_COUNT);

	/* Profile */
	render_config_field_radio(width, TUI_CONFIG_PROFILE,
				  state->selected_field == TUI_CONFIG_PROFILE,
				  "Profile", (int)state->config.profile,
				  TUI_PROFILE_COUNT);

	/* Separator */
	print_separator(width);

	/* Threads */
	render_config_field_numeric(width,
				    state->selected_field == TUI_CONFIG_THREADS,
				    "Threads", state->config.threads, NULL);

	/* Frames */
	render_config_field_numeric(width,
				    state->selected_field == TUI_CONFIG_FRAMES,
				    "Frames", state->config.frames, NULL);

	/* FPS */
	render_config_field_numeric(
		width, state->selected_field == TUI_CONFIG_FPS, "FPS Limit",
		state->config.fps,
		state->config.fps == 0 ? "(unlimited)" : "fps");

	/* Separator */
	print_separator(width);

	/* Access Order */
	render_config_field_radio(
		width, TUI_CONFIG_ACCESS_ORDER,
		state->selected_field == TUI_CONFIG_ACCESS_ORDER,
		"Access Order", (int)state->config.access_order,
		TUI_ACCESS_ORDER_COUNT);

	/* Header Size */
	render_config_field_numeric(
		width, state->selected_field == TUI_CONFIG_HEADER_SIZE,
		"Header Size", state->config.header_size, "bytes");

	/* Empty line */
	print_empty_line(width);

	/* Start button */
	render_config_start_button(width, state->selected_field ==
						  TUI_CONFIG_START_BUTTON);

	/* Empty line */
	print_empty_line(width);

	/* Info line - estimated size */
	/* Calculate estimated size based on profile */
	size_t frame_size = 0;
	switch (state->config.profile) {
	case TUI_PROFILE_SD:
		frame_size = 720 * 576 * 2;
		break;
	case TUI_PROFILE_HD:
		frame_size = 1280 * 720 * 2;
		break;
	case TUI_PROFILE_FULLHD:
		frame_size = 1920 * 1080 * 2;
		break;
	case TUI_PROFILE_2K:
		frame_size = 2048 * 1080 * 2;
		break;
	case TUI_PROFILE_4K:
		frame_size = 3840 * 2160 * 2;
		break;
	case TUI_PROFILE_8K:
		frame_size = 7680 * 4320 * 2;
		break;
	default:
		frame_size = 1920 * 1080 * 2;
		break;
	}
	size_t total_size =
		(frame_size + state->config.header_size) * state->config.frames;

	char size_str[32];
	format_bytes(total_size, size_str, sizeof(size_str));

	/* Estimated time at ~100 MiB/s */
	double est_seconds = (double)total_size / (100.0 * 1024 * 1024);

	char info_line[128];
	int info_len = snprintf(
		info_line, sizeof(info_line),
		" Filesystem: LOCAL    Est. Size: %s    Est. Time: ~%.0fs @ 100 MiB/s",
		size_str, est_seconds);

	int pad = width - 2 - info_len;
	if (pad < 0)
		pad = 0;

	printf("\r%s%s%s%s", BOX_V, DIM, info_line, RESET);
	print_pad(pad);
	printf("%s\r\n", BOX_V);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Dashboard view rendering  
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_view_dashboard(tui_app_state_t *state, tui_metrics_t *metrics)
{
	int width = state->term_width > 80 ? 80 : state->term_width;
	char lat_p50[16], lat_p99[16], lat_min[16], lat_max[16];

	if (!metrics) {
		const char *msg =
			"No test data available. Start a test from Config view.";
		int msg_len = (int)strlen(msg);
		int left_pad = (width - 2 - msg_len) / 2;
		int right_pad = width - 2 - msg_len - left_pad;
		printf("\r%s", BOX_V);
		print_pad(left_pad);
		printf("%s", msg);
		print_pad(right_pad);
		printf("%s\r\n", BOX_V);
		return;
	}

	/* Calculate metrics */
	double elapsed_sec = (double)metrics->elapsed_ns / 1000000000.0;
	double throughput_mibs = 0.0;
	double iops = 0.0;

	if (elapsed_sec > 0.001) {
		throughput_mibs =
			((double)metrics->bytes_written / (1024.0 * 1024.0)) /
			elapsed_sec;
		iops = (double)metrics->frames_completed / elapsed_sec;
	}

	int percent = 0;
	if (metrics->frames_total > 0) {
		percent = (int)((metrics->frames_completed * 100) /
				metrics->frames_total);
	}

	format_latency(metrics->latency_p50_ns, lat_p50, sizeof(lat_p50));
	format_latency(metrics->latency_p99_ns, lat_p99, sizeof(lat_p99));
	format_latency(metrics->latency_min_ns, lat_min, sizeof(lat_min));
	format_latency(metrics->latency_max_ns, lat_max, sizeof(lat_max));

	/* Profile and config */
	printf("\r%s  %s%s%s %s Test", BOX_V, BOLD,
	       metrics->profile_name ? metrics->profile_name : "Unknown", RESET,
	       metrics->test_type ? metrics->test_type : "");
	print_pad(width - 50);
	printf("FS: %s  Threads: %zu  %s\r\n",
	       metrics->fs_type == FILESYSTEM_LOCAL ? "LOCAL" :
	       metrics->fs_type == FILESYSTEM_SMB   ? "SMB" :
	       metrics->fs_type == FILESYSTEM_NFS   ? "NFS" :
						      "OTHER",
	       metrics->thread_count, BOX_V);

	/* Target path */
	printf("\r%s  Target: %s", BOX_V,
	       metrics->target_path ? metrics->target_path : "(none)");
	print_pad(
		width - 12 -
		strlen(metrics->target_path ? metrics->target_path : "(none)"));
	printf("  %s\r\n", BOX_V);

	/* Separator */
	print_hline(width, BOX_LT, BOX_H, BOX_RT);

	/* Progress bar */
	int bar_width = 30;
	int filled = (metrics->frames_total > 0) ?
			     (int)((metrics->frames_completed * bar_width) /
				   metrics->frames_total) :
			     0;

	printf("\r%s  Progress: [", BOX_V);
	for (int i = 0; i < bar_width; i++) {
		if (i < filled)
			printf("%s%s%s", GREEN, PROG_FULL, RESET);
		else
			printf("%s", PROG_EMPTY);
	}
	printf("] %3d%%  %zu/%zu frames", percent, metrics->frames_completed,
	       metrics->frames_total);
	print_pad(width - 65);
	printf("  %s\r\n", BOX_V);

	/* Separator */
	print_hline(width, BOX_LT, BOX_H, BOX_RT);

	/* Throughput and IOPS */
	printf("\r%s  Throughput: %s%.1f MiB/s%s   IOPS: %.1f fps", BOX_V, CYAN,
	       throughput_mibs, RESET, iops);
	print_pad(width - 48);
	printf("  %s\r\n", BOX_V);

	/* Latency */
	printf("\r%s  Latency:  P50: %-8s  P99: %-8s", BOX_V, lat_p50, lat_p99);
	print_pad(width - 42);
	printf("  %s\r\n", BOX_V);

	printf("\r%s            Min: %-8s  Max: %-8s", BOX_V, lat_min, lat_max);
	print_pad(width - 42);
	printf("  %s\r\n", BOX_V);

	/* Separator */
	print_hline(width, BOX_LT, BOX_H, BOX_RT);

	/* Sparkline */
	printf("\r%s  Trend: ", BOX_V);
	for (int i = 0; i < TUI_SPARKLINE_SIZE; i++) {
		int idx = (metrics->sparkline_idx + i) % TUI_SPARKLINE_SIZE;
		uint64_t val = metrics->sparkline_history[idx];
		int level = 0;
		if (val > 0 &&
		    metrics->latency_max_ns > metrics->latency_min_ns) {
			level = (int)(((val - metrics->latency_min_ns) * 7) /
				      (metrics->latency_max_ns -
				       metrics->latency_min_ns));
			if (level > 7)
				level = 7;
			if (level < 0)
				level = 0;
		}
		printf("%s", SPARK_CHARS[level]);
	}
	print_pad(width - 10 - TUI_SPARKLINE_SIZE);
	printf("  %s\r\n", BOX_V);

	/* Separator */
	print_hline(width, BOX_LT, BOX_H, BOX_RT);

	/* Status */
	printf("\r%s  %s%s%s %zu ok   %s%s%s %zu fail   I/O: %s", BOX_V, GREEN,
	       SYM_CHECK, RESET, metrics->frames_succeeded, RED, SYM_CROSS,
	       RESET, metrics->frames_failed,
	       metrics->current_io_mode == IO_MODE_DIRECT   ? "Direct" :
	       metrics->current_io_mode == IO_MODE_BUFFERED ? "Buffered" :
							      "---");

	/* Elapsed time */
	print_pad(width - 55);
	printf("Elapsed: %.1fs  %s\r\n", elapsed_sec, BOX_V);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * History view rendering
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_view_history(tui_app_state_t *state, tui_metrics_t *metrics)
{
	(void)metrics;
	int width = state->term_width > 80 ? 80 : state->term_width;
	size_t count = tui_history_count(state);

	/* Header */
	printf("\r%s  Frame History (%zu frames)    [f] filter: %s", BOX_V,
	       count, state->history_filter_failures ? "failures only" : "all");
	print_pad(width - 55);
	printf("  %s\r\n", BOX_V);

	/* Separator */
	print_hline(width, BOX_LT, BOX_H, BOX_RT);

	/* Calculate visible range */
	int visible_lines = state->term_height - 10;
	if (visible_lines < 5)
		visible_lines = 5;

	size_t start_idx = state->history_scroll_pos;
	if (start_idx > count)
		start_idx = count > 0 ? count - 1 : 0;

	/* Render frames */
	int lines_shown = 0;
	for (size_t i = start_idx; i < count && lines_shown < visible_lines;
	     i++) {
		const tui_frame_record_t *frame = tui_history_get(state, i);
		if (!frame)
			continue;

		/* Filter if needed */
		if (state->history_filter_failures && frame->success)
			continue;

		char lat_str[16];
		format_latency(frame->duration_ns, lat_str, sizeof(lat_str));

		char bytes_str[16];
		format_bytes(frame->bytes, bytes_str, sizeof(bytes_str));

		int selected = (i == state->history_selected);

		printf("\r%s  %s", BOX_V, selected ? SYM_ARROW : " ");
		printf("#%-4zu  ", frame->frame_num);

		if (frame->success) {
			printf("%s%s%s  ", GREEN, SYM_CHECK, RESET);
		} else {
			printf("%s%s%s  ", RED, SYM_CROSS, RESET);
		}

		printf("%-8s  %-8s  %-8s  thread-%d", lat_str,
		       frame->io_mode == IO_MODE_DIRECT ? "Direct" : "Buffer",
		       bytes_str, frame->thread_id);

		/* Flag slow frames */
		if (frame->duration_ns > 50000000) { /* > 50ms */
			printf("  %s← SLOW%s", YELLOW, RESET);
		}

		print_pad(width - 65);
		printf("  %s\r\n", BOX_V);
		lines_shown++;
	}

	/* Fill remaining lines */
	while (lines_shown < visible_lines) {
		printf("\r%s", BOX_V);
		print_pad(width - 2);
		printf("%s\r\n", BOX_V);
		lines_shown++;
	}

	/* Show details panel if enabled */
	if (state->history_show_details && state->history_selected < count) {
		const tui_frame_record_t *frame =
			tui_history_get(state, state->history_selected);
		if (frame) {
			print_hline(width, BOX_LT, BOX_H, BOX_RT);

			printf("\r%s  %sFrame #%zu Details%s", BOX_V, BOLD,
			       frame->frame_num, RESET);
			print_pad(width - 22);
			printf("  %s\r\n", BOX_V);

			char dur_str[16];
			format_latency(frame->duration_ns, dur_str,
				       sizeof(dur_str));

			printf("\r%s  Duration: %s   Bytes: %zu   Thread: %d   I/O: %s",
			       BOX_V, dur_str, frame->bytes, frame->thread_id,
			       frame->io_mode == IO_MODE_DIRECT ? "Direct" :
								  "Buffered");
			print_pad(width - 60);
			printf("  %s\r\n", BOX_V);
		}
	}
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Latency view rendering
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_view_latency(tui_app_state_t *state, tui_metrics_t *metrics)
{
	int width = state->term_width > 80 ? 80 : state->term_width;

	printf("\r%s  Latency Heatmap   [z] zoom: %dx   [←→] pan", BOX_V,
	       1 << state->latency_zoom);
	print_pad(width - 48);
	printf("  %s\r\n", BOX_V);

	print_hline(width, BOX_LT, BOX_H, BOX_RT);

	if (!metrics || metrics->frames_completed == 0) {
		const char *msg = "No latency data available yet.";
		int msg_len = (int)strlen(msg);
		int pad = width - 2 - msg_len;
		printf("\r%s %s", BOX_V, msg);
		print_pad(pad - 1);
		printf("%s\r\n", BOX_V);
		return;
	}

	/* Simple ASCII heatmap based on sparkline history */
	/* Buckets: <5ms, 5-10ms, 10-20ms, 20-50ms, >50ms */
	const char *bucket_labels[] = { ">50ms", "20-50", "10-20", "5-10",
					"<5ms" };
	int buckets[5] = { 0 };

	for (int i = 0; i < TUI_SPARKLINE_SIZE; i++) {
		uint64_t ns = metrics->sparkline_history[i];
		double ms = (double)ns / 1000000.0;

		if (ms > 50)
			buckets[0]++;
		else if (ms > 20)
			buckets[1]++;
		else if (ms > 10)
			buckets[2]++;
		else if (ms > 5)
			buckets[3]++;
		else
			buckets[4]++;
	}

	/* Render buckets */
	for (int b = 0; b < 5; b++) {
		printf("\r%s  %6s %s ", BOX_V, bucket_labels[b], BOX_V);

		int bar_len = buckets[b] * 3;
		for (int i = 0; i < bar_len && i < width - 20; i++) {
			if (b == 0)
				printf("%s█%s", RED, RESET);
			else if (b == 1)
				printf("%s█%s", YELLOW, RESET);
			else
				printf("%s█%s", GREEN, RESET);
		}

		print_pad(width - 12 - bar_len);
		printf(" %2d %s\n", buckets[b], BOX_V);
	}

	print_hline(width, BOX_LT, BOX_H, BOX_RT);

	/* Legend */
	printf("\r%s  Legend: %s█%s <5ms (good)  %s█%s 5-20ms  %s█%s >20ms (slow)",
	       BOX_V, GREEN, RESET, YELLOW, RESET, RED, RESET);
	print_pad(width - 58);
	printf("  %s\r\n", BOX_V);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Help overlay rendering
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_view_help_overlay(tui_app_state_t *state)
{
	int width = 50;
	int height = 18;
	int start_col = (state->term_width - width) / 2;
	int start_row = (state->term_height - height) / 2;

	if (start_col < 1)
		start_col = 1;
	if (start_row < 1)
		start_row = 1;

	/* Draw help box */
	tty_move_cursor(start_row, start_col);
	print_hline(width, DBOX_TL, DBOX_H, DBOX_TR);

	tty_move_cursor(start_row + 1, start_col);
	printf("%s", DBOX_V);
	print_centered(" Keyboard Shortcuts ", width - 2);
	printf("%s", DBOX_V);

	tty_move_cursor(start_row + 2, start_col);
	print_hline(width, DBOX_V " ", BOX_H, " " DBOX_V);

	const char *help_lines[] = {
		"  q         Quit",
		"  p         Pause/Resume test",
		"  1-4       Switch views",
		"  ?/h       Toggle this help",
		"",
		"  Config View:",
		"  ↑↓        Navigate fields",
		"  ←→        Change option",
		"  +/-       Adjust numeric value",
		"  Enter     Edit field / Start",
		"  s         Start test",
		"",
		"  History View:",
		"  ↑↓        Scroll",
		"  f         Filter failures",
	};

	int num_lines = sizeof(help_lines) / sizeof(help_lines[0]);
	for (int i = 0; i < num_lines && i < height - 4; i++) {
		tty_move_cursor(start_row + 3 + i, start_col);
		printf("%s %-*s %s", DBOX_V, width - 4, help_lines[i], DBOX_V);
	}

	tty_move_cursor(start_row + height - 1, start_col);
	print_hline(width, DBOX_BL, DBOX_H, DBOX_BR);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Main view dispatch
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_views_render(tui_app_state_t *state, tui_metrics_t *metrics)
{
	if (!state)
		return;

	int width = state->term_width > 80 ? 80 : state->term_width;

	/* Clear and move to home */
	tty_clear_screen();
	tty_move_cursor(1, 1);

	/* Render tab bar */
	tui_render_tab_bar(state, width);

	/* Render current view */
	switch (state->current_view) {
	case TUI_VIEW_DASHBOARD:
		tui_view_dashboard(state, metrics);
		break;

	case TUI_VIEW_HISTORY:
		tui_view_history(state, metrics);
		break;

	case TUI_VIEW_LATENCY:
		tui_view_latency(state, metrics);
		break;

	case TUI_VIEW_CONFIG:
		tui_view_config(state, metrics);
		break;

	default:
		printf("\r%s  Unknown view  %s\r\n", BOX_V, BOX_V);
		break;
	}

	/* Render status bar */
	tui_render_status_bar(state, width);

	/* Render help overlay if active */
	if (state->show_help) {
		tui_view_help_overlay(state);
	}

	/* Flush output */
	tty_flush();

	/* Clear redraw flag */
	state->needs_redraw = 0;
}
