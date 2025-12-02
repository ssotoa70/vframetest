/*
 * tui_render.c - Simple TUI rendering using screen buffer
 *
 * This file is part of vframetest.
 * Copyright (c) 2023-2025 Tuxera Inc.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "tui_render.h"
#include "tui_state.h"
#include "tui_format.h"

static screen_t scr;

/* ─────────────────────────────────────────────────────────────────────────────
 * Helper macros for themed colors
 * ───────────────────────────────────────────────────────────────────────────── */

#define SET_BORDER() screen_set_fg(&scr, scr.theme->border_fg)
#define SET_TITLE() screen_set_fg(&scr, scr.theme->title_fg)
#define SET_TEXT() screen_set_fg(&scr, scr.theme->text_fg)
#define SET_HIGHLIGHT() screen_set_fg(&scr, scr.theme->highlight_fg)
#define SET_SELECTED() \
	screen_set_color(&scr, scr.theme->selected_fg, scr.theme->selected_bg)
#define SET_SUCCESS() screen_set_fg(&scr, scr.theme->success_fg)
#define SET_ERROR() screen_set_fg(&scr, scr.theme->error_fg)
#define SET_WARNING() screen_set_fg(&scr, scr.theme->warning_fg)
#define SET_INFO() screen_set_fg(&scr, scr.theme->info_fg)
#define SET_VALUE() screen_set_fg(&scr, scr.theme->value_fg)
#define SET_PROGRESS() screen_set_fg(&scr, scr.theme->progress_fg)
#define SET_TAB_ACTIVE()                                 \
	screen_set_color(&scr, scr.theme->tab_active_fg, \
			 scr.theme->tab_active_bg)
#define SET_TAB_INACTIVE() screen_set_fg(&scr, scr.theme->tab_inactive_fg)
#define SET_STATUS() screen_set_fg(&scr, scr.theme->status_fg)
#define RESET_COLOR() screen_reset_color(&scr)

/* ─────────────────────────────────────────────────────────────────────────────
 * Helper functions
 * ───────────────────────────────────────────────────────────────────────────── */

static void draw_box(int row, int col, int w, int h)
{
	SET_BORDER();
	screen_box(&scr, row, col, w, h);
	RESET_COLOR();
}

static void draw_hline(int row, int col, int len)
{
	SET_BORDER();
	scr.cells[row][col] = '+';
	scr.colors[row][col] = MAKE_COLOR(scr.theme->border_fg, COLOR_DEFAULT);
	for (int c = col + 1; c < col + len - 1 && c < scr.width; c++) {
		scr.cells[row][c] = '-';
		scr.colors[row][c] =
			MAKE_COLOR(scr.theme->border_fg, COLOR_DEFAULT);
	}
	scr.cells[row][col + len - 1] = '+';
	scr.colors[row][col + len - 1] =
		MAKE_COLOR(scr.theme->border_fg, COLOR_DEFAULT);
	RESET_COLOR();
}

static void draw_text(int row, int col, const char *text)
{
	screen_move(&scr, row, col);
	screen_print(&scr, text);
}

/* Suppress unused warning - may be used in future */
static void draw_textf(int row, int col, const char *fmt, ...)
	__attribute__((unused));

static void draw_textf(int row, int col, const char *fmt, ...)
{
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	draw_text(row, col, buf);
}

/* Draw a section header with double-line separators */
static void draw_section_header(int row, int col, int width, const char *title)
{
	SET_BORDER();

	/* Draw left edge */
	scr.cells[row][col] = '|';
	scr.colors[row][col] = MAKE_COLOR(scr.theme->border_fg, COLOR_DEFAULT);

	/* Draw the section header line with title */
	screen_move(&scr, row, col + 2);
	SET_HIGHLIGHT();
	screen_print(&scr, title);

	/* Fill rest of line with spaces to width */
	int title_len = strlen(title);
	int remaining = width - 4 - title_len;
	SET_TEXT();
	for (int i = 0; i < remaining; i++) {
		screen_putc(&scr, ' ');
	}

	/* Draw right edge */
	SET_BORDER();
	scr.cells[row][width - 1] = '|';
	scr.colors[row][width - 1] = MAKE_COLOR(scr.theme->border_fg, COLOR_DEFAULT);

	RESET_COLOR();
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Tab bar
 * ───────────────────────────────────────────────────────────────────────────── */

static void render_tab_bar(tui_app_state_t *state, int width)
{
	const char *tabs[] = { "Dashboard", "History", "Latency", "Config" };
	const char *status;
	screen_color_t status_color;

	switch (state->run_state) {
	case TUI_STATE_RUNNING:
		status = "LIVE";
		status_color = scr.theme->success_fg;
		break;
	case TUI_STATE_PAUSED:
		status = "PAUSED";
		status_color = scr.theme->warning_fg;
		break;
	case TUI_STATE_COMPLETED:
		status = "DONE";
		status_color = scr.theme->info_fg;
		break;
	default:
		status = "IDLE";
		status_color = scr.theme->text_fg;
		break;
	}

	/* Draw tabs */
	int col = 2;
	for (int i = 0; i < 4; i++) {
		screen_move(&scr, 1, col);

		if (i == state->current_view) {
			SET_TAB_ACTIVE();
			screen_printf(&scr, "[%d]>%s<", i + 1, tabs[i]);
			RESET_COLOR();
			col += 4 + strlen(tabs[i]) + 2;
		} else {
			SET_TAB_INACTIVE();
			screen_printf(&scr, "[%d] %s ", i + 1, tabs[i]);
			RESET_COLOR();
			col += 4 + strlen(tabs[i]) + 2;
		}
	}

	/* Status on right */
	screen_move(&scr, 1, width - 8);
	screen_set_fg(&scr, status_color);
	screen_printf(&scr, "[%s]", status);
	RESET_COLOR();
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Status bar
 * ───────────────────────────────────────────────────────────────────────────── */

static void render_status_bar(tui_app_state_t *state, int width, int row)
{
	const char *hint;

	switch (state->current_view) {
	case TUI_VIEW_CONFIG:
		hint = "[Up/Down] navigate  [Left/Right] change  [+/-] adjust  [s] start  [q] quit";
		break;
	case TUI_VIEW_DASHBOARD:
		hint = "[p] pause  [1-4] views  [q] quit";
		break;
	case TUI_VIEW_HISTORY:
		hint = "[Up/Down] scroll  [q] quit";
		break;
	case TUI_VIEW_LATENCY:
		hint = "[q] quit";
		break;
	default:
		hint = "[q] quit";
	}

	SET_STATUS();
	draw_text(row, 2, hint);
	RESET_COLOR();
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Config view
 * ───────────────────────────────────────────────────────────────────────────── */

static void render_config(tui_app_state_t *state, int width, int start_row)
{
	int row = start_row;

	/* Title */
	SET_TITLE();
	screen_set_attr(&scr, ATTR_BOLD);
	draw_text(row++, (width - 20) / 2, "=== Test Configuration ===");
	RESET_COLOR();
	row++;

	/* Path */
	const char *path;
	int is_selected = (state->selected_field == TUI_CONFIG_PATH);
	int is_editing = state->editing_text && is_selected;

	if (is_editing) {
		path = state->edit_buffer;
	} else {
		path = state->config.path[0] ? state->config.path : "(not set)";
	}

	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Target Path:  ");
	if (is_editing) {
		screen_set_attr(&scr, ATTR_BOLD);
		screen_set_fg(&scr, COLOR_CYAN);
		screen_print(&scr, path);
		screen_print(&scr, "_"); /* Cursor */
	} else {
		SET_VALUE();
		screen_print(&scr, path);
	}
	RESET_COLOR();
	row++;

	draw_hline(row++, 1, width - 2);

	/* Test Type */
	is_selected = (state->selected_field == TUI_CONFIG_TEST_TYPE);
	const char *test_types[] = { "Write", "Read", "Empty", "Streaming" };

	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Test Type:    ");

	for (int i = 0; i < 4; i++) {
		if (i == (int)state->config.test_type) {
			SET_SUCCESS();
			screen_print(&scr, "(*)");
		} else {
			SET_TEXT();
			screen_print(&scr, "(o)");
		}
		screen_printf(&scr, " %s  ", test_types[i]);
	}
	RESET_COLOR();
	row++;

	/* Profile Category Filter */
	is_selected = (state->selected_field == TUI_CONFIG_PROFILE);
	const char *categories[] = { "All", "Standard", "DPX", "EXR" };
	const char *profiles[] = { "SD", "HD", "FULLHD", "2K", "4K", "8K",
				   "DPX-2K", "DPX-FHD", "DPX-4K", "DPX-8K",
				   "EXR-FHD-h", "EXR-4K-h", "EXR-8K-h",
				   "EXR-FHD-f", "EXR-4K-f", "EXR-8K-f" };

	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Categories:   ");

	for (int i = 0; i < 4; i++) {
		if (i == (int)state->config.profile_category) {
			SET_SUCCESS();
			screen_print(&scr, "(*)");
		} else {
			SET_TEXT();
			screen_print(&scr, "(o)");
		}
		screen_printf(&scr, " %s ", categories[i]);
	}
	RESET_COLOR();
	row++;

	/* Profile Selection */
	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, " ");
	SET_TEXT();
	screen_print(&scr, "Profiles:     ");

	for (int i = 0; i < 16; i++) {
		/* Check if profile belongs to selected category */
		int show_profile =
			(state->config.profile_category == 0 || /* All */
			 (state->config.profile_category == 1 && i < 6) ||  /* Standard */
			 (state->config.profile_category == 2 && i >= 6 && i < 10) || /* DPX */
			 (state->config.profile_category == 3 && i >= 10 && i < 16)); /* EXR */

		if (!show_profile) {
			continue;
		}

		if (i == (int)state->config.profile) {
			SET_SUCCESS();
			screen_print(&scr, "(*)");
		} else {
			SET_TEXT();
			screen_print(&scr, "(o)");
		}
		screen_printf(&scr, " %s ", profiles[i]);
		if ((i + 1) % 3 == 0) {
			/* Wrap to next line for readability */
			row++;
			screen_move(&scr, row, 18);
		}
	}
	RESET_COLOR();
	row++;

	draw_hline(row++, 1, width - 2);

	/* Threads */
	is_selected = (state->selected_field == TUI_CONFIG_THREADS);
	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Threads:      [ ");
	SET_VALUE();
	screen_printf(&scr, "%zu", state->config.threads);
	SET_TEXT();
	screen_print(&scr, " ]");
	RESET_COLOR();
	row++;

	/* Frames */
	is_selected = (state->selected_field == TUI_CONFIG_FRAMES);
	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Frames:       [ ");
	SET_VALUE();
	screen_printf(&scr, "%zu", state->config.frames);
	SET_TEXT();
	screen_print(&scr, " ]");
	RESET_COLOR();
	row++;

	/* FPS */
	is_selected = (state->selected_field == TUI_CONFIG_FPS);
	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " FPS Limit:    [ ");
	SET_VALUE();
	if (state->config.fps == 0) {
		screen_print(&scr, "unlimited");
	} else {
		screen_printf(&scr, "%zu", state->config.fps);
	}
	SET_TEXT();
	screen_print(&scr, " ]");
	RESET_COLOR();
	row++;

	draw_hline(row++, 1, width - 2);

	/* Access Order */
	is_selected = (state->selected_field == TUI_CONFIG_ACCESS_ORDER);
	const char *orders[] = { "Normal", "Reverse", "Random" };

	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Access Order: ");

	for (int i = 0; i < 3; i++) {
		if (i == (int)state->config.access_order) {
			SET_SUCCESS();
			screen_print(&scr, "(*)");
		} else {
			SET_TEXT();
			screen_print(&scr, "(o)");
		}
		screen_printf(&scr, " %s  ", orders[i]);
	}
	RESET_COLOR();
	row++;

	/* Header Size */
	is_selected = (state->selected_field == TUI_CONFIG_HEADER_SIZE);
	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Header Size:  [ ");
	SET_VALUE();
	screen_printf(&scr, "%zu", state->config.header_size);
	SET_TEXT();
	screen_print(&scr, " ] bytes");
	RESET_COLOR();
	row++;

	/* Auto-cleanup */
	is_selected = (state->selected_field == TUI_CONFIG_AUTO_CLEANUP);
	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Auto-cleanup: ");

	if (state->config.auto_cleanup) {
		SET_SUCCESS();
		screen_print(&scr, "(*) Yes  ");
		SET_TEXT();
		screen_print(&scr, "(o) No");
	} else {
		SET_TEXT();
		screen_print(&scr, "(o) Yes  ");
		SET_WARNING();
		screen_print(&scr, "(*) No");
	}
	RESET_COLOR();
	row++;

	/* Open Dashboard */
	is_selected = (state->selected_field == TUI_CONFIG_OPEN_DASHBOARD);
	screen_move(&scr, row, 2);
	if (is_selected) {
		SET_HIGHLIGHT();
	} else {
		SET_TEXT();
	}
	screen_print(&scr, is_selected ? ">" : " ");
	SET_TEXT();
	screen_print(&scr, " Open Dashboard:");

	if (state->config.open_dashboard) {
		SET_SUCCESS();
		screen_print(&scr, "(*) Yes  ");
		SET_TEXT();
		screen_print(&scr, "(o) No");
	} else {
		SET_TEXT();
		screen_print(&scr, "(o) Yes  ");
		SET_INFO();
		screen_print(&scr, "(*) No");
	}
	RESET_COLOR();
	row++;

	row++;

	/* Start button */
	is_selected = (state->selected_field == TUI_CONFIG_START_BUTTON);
	screen_move(&scr, row, (width - 24) / 2);
	if (is_selected) {
		SET_SELECTED();
		screen_set_attr(&scr, ATTR_BOLD);
		screen_print(&scr, ">>> [S] START TEST <<<");
	} else {
		SET_INFO();
		screen_print(&scr, "[S] START TEST");
	}
	RESET_COLOR();
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Dashboard view
 * ───────────────────────────────────────────────────────────────────────────── */

static void render_dashboard(tui_app_state_t *state, tui_metrics_t *metrics,
			     int width, int start_row)
{
	int row = start_row;
	(void)state;

	if (!metrics) {
		SET_WARNING();
		draw_text(row, (width - 45) / 2,
			  "No test data. Switch to Config to start a test.");
		RESET_COLOR();
		return;
	}

	/* Calculate ETA and trend */
	calculate_eta(metrics);
	detect_latency_trend(metrics);

	/* Profile and path */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Profile: ");
	SET_VALUE();
	screen_print(&scr, metrics->profile_name ? metrics->profile_name : "?");
	SET_TEXT();
	screen_print(&scr, "   Threads: ");
	SET_VALUE();
	screen_printf(&scr, "%zu", metrics->thread_count);
	SET_TEXT();
	screen_print(&scr, "   Test: ");
	SET_VALUE();
	screen_print(&scr, metrics->test_type ? metrics->test_type : "?");
	RESET_COLOR();
	row++;

	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Target: ");
	SET_VALUE();
	screen_print(&scr,
		     metrics->target_path ? metrics->target_path : "(none)");
	SET_TEXT();
	screen_print(&scr, "   FS: ");
	SET_VALUE();
	const char *fs_name = metrics->fs_type == FILESYSTEM_SMB ? "SMB" :
			      metrics->fs_type == FILESYSTEM_NFS ? "NFS" :
			      "LOCAL";
	screen_print(&scr, fs_name);
	RESET_COLOR();
	row++;

	/* Filesystem optimization status */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Filesystem Status: ");
	SET_VALUE();
	if (metrics->fs_type == FILESYSTEM_NFS) {
		screen_print(&scr, "✓ Direct I/O optimized for NFS");
	} else if (metrics->fs_type == FILESYSTEM_SMB) {
		screen_print(&scr, "⚠ Network filesystem (buffered I/O)");
	} else {
		screen_print(&scr, "✓ Local filesystem - optimal");
	}
	RESET_COLOR();
	row++;

	draw_hline(row++, 1, width - 2);

	/* Progress */
	int pct = metrics->frames_total > 0 ?
			  (int)((metrics->frames_completed * 100) /
				metrics->frames_total) :
			  0;
	int bar_len = 30;
	int filled = (pct * bar_len) / 100;

	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Progress: [");

	SET_PROGRESS();
	for (int i = 0; i < filled; i++) {
		screen_putc(&scr, '#');
	}
	screen_set_fg(&scr, scr.theme->progress_bg);
	for (int i = filled; i < bar_len; i++) {
		screen_putc(&scr, '.');
	}
	SET_TEXT();
	screen_printf(&scr, "] %3d%%  ", pct);
	SET_VALUE();
	screen_printf(&scr, "%zu/%zu", metrics->frames_completed,
		      metrics->frames_total);
	SET_TEXT();
	screen_print(&scr, " frames");
	RESET_COLOR();
	row++;

	/* Elapsed/ETA/Total time */
	char elapsed_str[32], eta_str[32], total_str[32];
	format_time_human(metrics->elapsed_ns, elapsed_str, sizeof(elapsed_str));

	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Elapsed: ");
	SET_VALUE();
	screen_print(&scr, elapsed_str);
	SET_TEXT();
	screen_print(&scr, "   ETA: ");
	SET_VALUE();
	if (metrics->frames_completed >= 5) {
		format_time_human(metrics->eta_ns, eta_str, sizeof(eta_str));
		screen_print(&scr, eta_str);
		SET_TEXT();
		screen_print(&scr, "   Total: ~");
		SET_VALUE();
		format_time_human(metrics->total_estimated_ns, total_str,
				  sizeof(total_str));
		screen_print(&scr, total_str);
	} else {
		screen_print(&scr, "Calculating...");
	}
	RESET_COLOR();
	row++;

	draw_hline(row++, 1, width - 2);

	/* PERFORMANCE METRICS Section */
	draw_section_header(row++, 1, width - 2, "PERFORMANCE METRICS");

	/* Throughput with MB/s */
	double elapsed = (double)metrics->elapsed_ns / 1e9;
	double mibs =
		elapsed > 0 ?
			((double)metrics->bytes_written / (1024.0 * 1024.0)) /
				elapsed :
			0;
	double mbs = mibs * 1.048576;
	double fps = elapsed > 0 ? (double)metrics->frames_completed / elapsed :
				   0;

	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Throughput: ");
	SET_VALUE();
	screen_printf(&scr, "%.1f MiB/s", mibs);
	SET_TEXT();
	screen_print(&scr, "  |  FPS: ");
	SET_VALUE();
	screen_printf(&scr, "%.1f", fps);
	RESET_COLOR();
	row++;

	/* Bytes transferred */
	char bytes_str[64];
	format_bytes_human(metrics->bytes_written, bytes_str, sizeof(bytes_str));

	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Bytes: ");
	SET_VALUE();
	screen_print(&scr, bytes_str);
	SET_TEXT();
	screen_print(&scr, "  |  Data Rate: ");
	SET_VALUE();
	screen_printf(&scr, "%.1f MB/s", mbs);
	RESET_COLOR();
	row++;
	row++; /* Empty line for spacing */

	draw_hline(row++, 1, width - 2);

	/* LATENCY ANALYSIS Section */
	draw_section_header(row++, 1, width - 2, "LATENCY ANALYSIS");

	/* Latency with Trend */
	const char *trend_arrow = get_trend_arrow(metrics->latency_trend);
	const char *trend_text = metrics->latency_trend > 0 ? "Improving" :
				 metrics->latency_trend < 0 ? "Degrading" : "Stable";

	row++; /* Skip line after header */

	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "  Min: ");
	SET_VALUE();
	screen_printf(&scr, "%.2fms", (double)metrics->latency_min_ns / 1e6);
	SET_TEXT();
	screen_print(&scr, "   Max: ");
	SET_VALUE();
	screen_printf(&scr, "%.2fms", (double)metrics->latency_max_ns / 1e6);
	SET_TEXT();
	screen_print(&scr, "   Trend: ");
	SET_VALUE();
	screen_printf(&scr, "%s %s", trend_arrow, trend_text);
	RESET_COLOR();
	row++;

	/* Latency Percentiles */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "  P50: ");
	SET_VALUE();
	screen_printf(&scr, "%.2fms", (double)metrics->latency_p50_ns / 1e6);
	SET_TEXT();
	screen_print(&scr, "   P95: ");
	SET_VALUE();
	screen_printf(&scr, "%.2fms", (double)metrics->latency_p95_ns / 1e6);
	SET_TEXT();
	screen_print(&scr, "   P99: ");
	SET_VALUE();
	screen_printf(&scr, "%.2fms", (double)metrics->latency_p99_ns / 1e6);
	RESET_COLOR();
	row++;

	/* Frame Time Stats */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Frame Time: Min: ");
	SET_VALUE();
	screen_printf(&scr, "%.2fms", (double)metrics->frame_time_min_ns / 1e6);
	SET_TEXT();
	screen_print(&scr, "  Avg: ");
	SET_VALUE();
	screen_printf(&scr, "%.2fms", (double)metrics->frame_time_avg_ns / 1e6);
	SET_TEXT();
	screen_print(&scr, "  Max: ");
	SET_VALUE();
	screen_printf(&scr, "%.2fms", (double)metrics->frame_time_max_ns / 1e6);
	RESET_COLOR();
	row++;

	draw_hline(row++, 1, width - 2);

	/* TEST RESULTS Section */
	draw_section_header(row++, 1, width - 2, "TEST RESULTS");

	/* Status and Success Rate */
	screen_move(&scr, row, 2);
	SET_TEXT();
	screen_print(&scr, "Frames Succeeded: ");
	SET_SUCCESS();
	screen_printf(&scr, "%zu/%zu", metrics->frames_succeeded,
		      metrics->frames_total);
	SET_TEXT();
	screen_printf(&scr, " (%.1f%%)", metrics->success_rate_percent);
	RESET_COLOR();
	row++;

	screen_move(&scr, row, 2);
	SET_TEXT();
	screen_print(&scr, "Frames Failed:    ");
	if (metrics->frames_failed > 0) {
		SET_ERROR();
	} else {
		SET_SUCCESS();
	}
	screen_printf(&scr, "%zu/%zu", metrics->frames_failed,
		      metrics->frames_total);
	SET_TEXT();
	screen_printf(&scr, " (%.1f%%)",
		      metrics->frames_total > 0 ?
			      (double)metrics->frames_failed * 100.0 /
				      metrics->frames_total :
			      0.0);
	RESET_COLOR();
	row++;

	screen_move(&scr, row, 2);
	SET_TEXT();
	screen_print(&scr, "Success Rate:     ");
	if (metrics->success_rate_percent >= 99.5) {
		SET_SUCCESS();
		screen_printf(&scr, "%.2f%% ✓", metrics->success_rate_percent);
	} else if (metrics->success_rate_percent >= 90.0) {
		SET_WARNING();
		screen_printf(&scr, "%.2f%% ⚠", metrics->success_rate_percent);
	} else {
		SET_ERROR();
		screen_printf(&scr, "%.2f%% ✗", metrics->success_rate_percent);
	}
	RESET_COLOR();
	row++;
	row++; /* Empty line for spacing */

	draw_hline(row++, 1, width - 2);

	/* I/O MODE DISTRIBUTION Section */
	draw_section_header(row++, 1, width - 2, "I/O MODE DISTRIBUTION");

	/* I/O Statistics with visual breakdown */
	size_t total_io_frames = metrics->frames_direct_io + metrics->frames_buffered_io;
	double direct_percent = total_io_frames > 0 ?
		(double)metrics->frames_direct_io * 100.0 / total_io_frames : 0;
	double buffered_percent = total_io_frames > 0 ?
		(double)metrics->frames_buffered_io * 100.0 / total_io_frames : 0;

	row++; /* Skip line after header */

	/* Direct I/O bar */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "  Direct I/O:   ");
	SET_VALUE();
	int direct_bar_width = (int)(direct_percent / 3.0); /* 100% = ~33 chars */
	if (direct_bar_width > 30) direct_bar_width = 30;
	for (int i = 0; i < direct_bar_width; i++) screen_print(&scr, "█");
	for (int i = direct_bar_width; i < 30; i++) screen_print(&scr, "░");
	SET_TEXT();
	screen_printf(&scr, "  %5.1f%% (%zu frames)", direct_percent, metrics->frames_direct_io);
	if (direct_percent > 80) {
		SET_SUCCESS();
		screen_print(&scr, " ✓");
	}
	RESET_COLOR();
	row++;

	/* Buffered I/O bar */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "  Buffered I/O: ");
	SET_VALUE();
	int buffered_bar_width = (int)(buffered_percent / 3.0);
	if (buffered_bar_width > 30) buffered_bar_width = 30;
	for (int i = 0; i < buffered_bar_width; i++) screen_print(&scr, "█");
	for (int i = buffered_bar_width; i < 30; i++) screen_print(&scr, "░");
	SET_TEXT();
	screen_printf(&scr, "  %5.1f%% (%zu frames)", buffered_percent, metrics->frames_buffered_io);
	if (buffered_percent > 20) {
		SET_WARNING();
		screen_print(&scr, " ⚠");
	}
	RESET_COLOR();
	row++;

	/* Current I/O mode */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "  Current Mode: ");
	SET_VALUE();
	screen_print(&scr, metrics->current_io_mode == IO_MODE_DIRECT ?
				   "Direct I/O" :
				   "Buffered I/O");
	RESET_COLOR();
	row++;
	row++; /* Empty line for spacing */

	draw_hline(row++, 1, width - 2);

	/* FILESYSTEM & OPTIMIZATION Section */
	draw_section_header(row++, 1, width - 2, "FILESYSTEM & OPTIMIZATION");

	/* Filesystem type and status */
	screen_move(&scr, row, 2);
	SET_TEXT();
	screen_print(&scr, "Filesystem: ");
	SET_VALUE();
	const char *fs_type_str = metrics->fs_type == FILESYSTEM_SMB ? "SMB" :
			      metrics->fs_type == FILESYSTEM_NFS ? "NFS" :
			      "LOCAL";
	screen_print(&scr, fs_type_str);
	SET_TEXT();
	screen_print(&scr, "  |  Status: ");
	SET_VALUE();
	if (metrics->fs_type == FILESYSTEM_NFS || metrics->fs_type == FILESYSTEM_LOCAL) {
		SET_SUCCESS();
		screen_print(&scr, "Optimal");
	} else {
		SET_WARNING();
		screen_print(&scr, "Network filesystem");
	}
	RESET_COLOR();
	row++;

	screen_move(&scr, row, 2);
	SET_TEXT();
	screen_print(&scr, "Optimization: ");
	if (direct_percent > 80.0) {
		SET_SUCCESS();
		screen_print(&scr, "✓ Direct I/O active (expected)");
	} else if (buffered_percent > 50.0) {
		SET_WARNING();
		screen_print(&scr, "⚠ Buffered I/O active");
	} else {
		SET_INFO();
		screen_print(&scr, "Mixed I/O modes");
	}
	RESET_COLOR();
	row++;
	row++; /* Empty line for spacing */

	draw_hline(row++, 1, width - 2);

	/* SYSTEM STATUS Section */
	draw_section_header(row++, 1, width - 2, "SYSTEM STATUS");

	/* System Status Summary */
	screen_move(&scr, row, 2);
	SET_TEXT();
	screen_print(&scr, "Status: ");
	SET_VALUE();
	int total_frames = metrics->frames_succeeded + metrics->frames_failed;
	if (total_frames > 0) {
		if (metrics->success_rate_percent >= 99.5) {
			SET_SUCCESS();
			screen_print(&scr, "EXCELLENT");
		} else if (metrics->success_rate_percent >= 95.0) {
			SET_INFO();
			screen_print(&scr, "GOOD");
		} else if (metrics->success_rate_percent >= 90.0) {
			SET_WARNING();
			screen_print(&scr, "FAIR");
		} else {
			SET_ERROR();
			screen_print(&scr, "POOR");
		}
	} else {
		SET_INFO();
		screen_print(&scr, "RUNNING");
	}
	RESET_COLOR();
	row++;

	/* Performance Assessment */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Performance: ");
	SET_VALUE();
	if (mibs > 500.0) {
		SET_SUCCESS();
		screen_print(&scr, "EXCELLENT (>500 MiB/s)");
	} else if (mibs > 250.0) {
		SET_INFO();
		screen_print(&scr, "GOOD (250-500 MiB/s)");
	} else if (mibs > 100.0) {
		SET_WARNING();
		screen_print(&scr, "FAIR (100-250 MiB/s)");
	} else {
		SET_WARNING();
		screen_print(&scr, "SLOW (<100 MiB/s)");
	}
	RESET_COLOR();
	row++;

	/* Stability Assessment */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "Stability: ");
	SET_VALUE();
	double avg_latency = (double)metrics->latency_min_ns / 1e6;
	double max_latency = (double)metrics->latency_max_ns / 1e6;
	double variation = max_latency > 0 ? ((max_latency - avg_latency) / max_latency) * 100.0 : 0;
	if (variation < 10.0) {
		SET_SUCCESS();
		screen_print(&scr, "STABLE");
	} else if (variation < 25.0) {
		SET_INFO();
		screen_print(&scr, "MODERATE");
	} else {
		SET_WARNING();
		screen_print(&scr, "VARIABLE");
	}
	RESET_COLOR();
	row++;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * History view
 * ───────────────────────────────────────────────────────────────────────────── */

static void render_history(tui_app_state_t *state, int width, int start_row)
{
	int row = start_row;

	SET_TITLE();
	draw_text(row++, 2, "Frame History (most recent first)");
	RESET_COLOR();
	draw_hline(row++, 1, width - 2);

	size_t count = tui_history_count(state);
	if (count == 0) {
		SET_WARNING();
		draw_text(row, 2, "No frames recorded yet.");
		RESET_COLOR();
		return;
	}

	SET_HIGHLIGHT();
	draw_text(row++, 2, " Frame#   Time(ms)   Status   I/O Mode");
	RESET_COLOR();

	int max_lines = 10;
	/* Show most recent first - iterate from count-1 down to 0 */
	for (size_t i = 0; i < count && i < (size_t)max_lines; i++) {
		size_t idx = count - 1 - i; /* Reverse order: newest first */
		const tui_frame_record_t *f = tui_history_get(state, idx);
		if (!f)
			continue;

		screen_move(&scr, row, 2);
		SET_VALUE();
		screen_printf(&scr, " %6zu   %8.2f   ", f->frame_num,
			      (double)f->duration_ns / 1e6);

		if (f->success) {
			SET_SUCCESS();
			screen_print(&scr, "OK  ");
		} else {
			SET_ERROR();
			screen_print(&scr, "FAIL");
		}

		SET_TEXT();
		screen_print(&scr, "   ");
		SET_VALUE();
		screen_print(&scr, f->io_mode == IO_MODE_DIRECT ? "Direct  " :
								  "Buffered");
		RESET_COLOR();
		row++;
	}
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Latency view
 * ───────────────────────────────────────────────────────────────────────────── */

static void render_latency(tui_app_state_t *state, tui_metrics_t *metrics,
			   int width, int start_row)
{
	int row = start_row;

	SET_TITLE();
	draw_text(row++, 2, "Latency Distribution (all frames)");
	RESET_COLOR();
	draw_hline(row++, 1, width - 2);

	size_t frame_count = tui_history_count(state);

	if (!metrics || frame_count == 0) {
		SET_WARNING();
		draw_text(row, 2, "No latency data available yet.");
		RESET_COLOR();
		return;
	}

/* Build histogram from ALL frames in history */
#define HIST_BUCKETS 40
	size_t histogram[HIST_BUCKETS] = { 0 };
	size_t max_bucket = 0;

	uint64_t min_ns = metrics->latency_min_ns;
	uint64_t max_ns = metrics->latency_max_ns;
	uint64_t range_ns = max_ns - min_ns;

	/* Count frames in each bucket */
	for (size_t i = 0; i < frame_count; i++) {
		const tui_frame_record_t *f = tui_history_get(state, i);
		if (!f || f->duration_ns == 0)
			continue;

		int bucket;
		if (range_ns > 0) {
			bucket = (int)(((f->duration_ns - min_ns) *
					(HIST_BUCKETS - 1)) /
				       range_ns);
		} else {
			bucket = HIST_BUCKETS / 2;
		}
		if (bucket < 0)
			bucket = 0;
		if (bucket >= HIST_BUCKETS)
			bucket = HIST_BUCKETS - 1;

		histogram[bucket]++;
		if (histogram[bucket] > max_bucket) {
			max_bucket = histogram[bucket];
		}
	}

	/* Draw histogram header */
	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_printf(&scr, "Distribution (%zu frames):", frame_count);
	row++;

	/* Unicode bar characters: ▁▂▃▄▅▆▇█ (8 levels) */
	static const char *bars[] = { " ", "▁", "▂", "▃", "▄",
				      "▅", "▆", "▇", "█" };

	/* Draw the histogram using Unicode sparkline bars */
	screen_move(&scr, row, 2);
	for (int i = 0; i < HIST_BUCKETS; i++) {
		int level = 0;
		if (max_bucket > 0 && histogram[i] > 0) {
			level = 1 + (int)((histogram[i] * 7) / max_bucket);
			if (level > 8)
				level = 8;
		}

		/* Color based on latency position: green (fast) -> yellow -> red (slow) */
		if (i < HIST_BUCKETS / 3) {
			SET_SUCCESS(); /* Green - low latency */
		} else if (i < (HIST_BUCKETS * 2) / 3) {
			SET_WARNING(); /* Yellow - medium latency */
		} else {
			SET_ERROR(); /* Red - high latency */
		}
		screen_print(&scr, bars[level]);
	}
	RESET_COLOR();
	row++;

	/* Labels under histogram: min on left, max on right */
	screen_move(&scr, row, 2);
	SET_SUCCESS();
	screen_printf(&scr, "%.1fms", (double)min_ns / 1e6);

	/* Right-align max label */
	char max_label[16];
	snprintf(max_label, sizeof(max_label), "%.1fms", (double)max_ns / 1e6);
	screen_move(&scr, row, 2 + HIST_BUCKETS - (int)strlen(max_label));
	SET_ERROR();
	screen_printf(&scr, "%.1fms", (double)max_ns / 1e6);
	RESET_COLOR();
	row++;

	row++;

	/* Statistics */
	SET_HIGHLIGHT();
	draw_text(row++, 2, "Statistics:");
	RESET_COLOR();

	SET_TEXT();
	screen_move(&scr, row, 2);
	screen_print(&scr, "  Min: ");
	SET_SUCCESS();
	screen_printf(&scr, "%.2fms", (double)min_ns / 1e6);
	SET_TEXT();
	screen_print(&scr, "   Max: ");
	SET_ERROR();
	screen_printf(&scr, "%.2fms", (double)max_ns / 1e6);
	SET_TEXT();
	screen_print(&scr, "   Avg: ");
	SET_VALUE();
	/* Calculate average from history */
	uint64_t total_ns = 0;
	for (size_t i = 0; i < frame_count; i++) {
		const tui_frame_record_t *f = tui_history_get(state, i);
		if (f)
			total_ns += f->duration_ns;
	}
	double avg_ms = frame_count > 0 ? (double)total_ns / frame_count / 1e6 :
					  0;
	screen_printf(&scr, "%.2fms", avg_ms);
	RESET_COLOR();
	row++;

	/* Percentiles if available */
	if (metrics->latency_p50_ns > 0 || metrics->latency_p95_ns > 0) {
		row++;
		SET_HIGHLIGHT();
		draw_text(row++, 2, "Percentiles:");
		RESET_COLOR();

		/* P50 - Median */
		SET_TEXT();
		screen_move(&scr, row, 2);
		screen_print(&scr, "  P50:  ");
		SET_SUCCESS();
		screen_printf(&scr, "%-8.2fms",
			      (double)metrics->latency_p50_ns / 1e6);
		SET_TEXT();
		screen_print(&scr, " ✓ Good");
		RESET_COLOR();
		row++;

		/* P95 - 95th percentile */
		SET_TEXT();
		screen_move(&scr, row, 2);
		screen_print(&scr, "  P95:  ");
		SET_WARNING();
		screen_printf(&scr, "%-8.2fms",
			      (double)metrics->latency_p95_ns / 1e6);
		SET_TEXT();
		screen_print(&scr, " ⚠ Fair");
		RESET_COLOR();
		row++;

		/* P99 - 99th percentile */
		SET_TEXT();
		screen_move(&scr, row, 2);
		screen_print(&scr, "  P99:  ");
		SET_ERROR();
		screen_printf(&scr, "%-8.2fms",
			      (double)metrics->latency_p99_ns / 1e6);
		SET_TEXT();
		screen_print(&scr, " ✗ Slow");
		RESET_COLOR();
		row++;
		RESET_COLOR();
	}

#undef HIST_BUCKETS
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Main render function
 * ───────────────────────────────────────────────────────────────────────────── */

void tui_render_screen(tui_app_state_t *state, tui_metrics_t *metrics)
{
	int width = state->term_width > 80 ? 80 : state->term_width;
	int height = state->term_height > 24 ? 24 : state->term_height;

	screen_init(&scr, width, height);

	/* Draw outer box */
	draw_box(0, 0, width, height);

	/* Tab bar (row 1) */
	render_tab_bar(state, width);

	/* Separator after tabs */
	draw_hline(2, 0, width);

	/* Content area starts at row 3 */
	int content_start = 3;

	switch (state->current_view) {
	case TUI_VIEW_CONFIG:
		render_config(state, width, content_start);
		break;
	case TUI_VIEW_DASHBOARD:
		render_dashboard(state, metrics, width, content_start);
		break;
	case TUI_VIEW_HISTORY:
		render_history(state, width, content_start);
		break;
	case TUI_VIEW_LATENCY:
		render_latency(state, metrics, width, content_start);
		break;
	default:
		SET_ERROR();
		draw_text(content_start, 2, "Unknown view");
		RESET_COLOR();
	}

	/* Status bar (second to last row) */
	draw_hline(height - 2, 0, width);
	render_status_bar(state, width, height - 1);

	/* Render to terminal */
	screen_render(&scr);

	state->needs_redraw = 0;
}
