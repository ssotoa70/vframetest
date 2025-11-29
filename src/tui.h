/*
 * This file is part of vframetest.
 *
 * Copyright (c) 2023-2025 Tuxera Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FRAMETEST_TUI_H
#define FRAMETEST_TUI_H

#include <stdint.h>
#include <stddef.h>
#include "frametest.h"

/* TUI display width (characters) */
#define TUI_WIDTH 60

/* Sparkline history size */
#define TUI_SPARKLINE_SIZE 20

/* Percentile calculation buffer size (larger = more accurate) */
#define TUI_PERCENTILE_BUFFER_SIZE 512

/* ANSI escape codes for colors and formatting */
#define TUI_RESET       "\033[0m"
#define TUI_BOLD        "\033[1m"
#define TUI_DIM         "\033[2m"
#define TUI_GREEN       "\033[32m"
#define TUI_YELLOW      "\033[33m"
#define TUI_RED         "\033[31m"
#define TUI_CYAN        "\033[36m"
#define TUI_WHITE       "\033[37m"
#define TUI_BG_BLACK    "\033[40m"

/* Cursor control */
#define TUI_CLEAR_SCREEN "\033[2J"
#define TUI_HOME         "\033[H"
#define TUI_HIDE_CURSOR  "\033[?25l"
#define TUI_SHOW_CURSOR  "\033[?25h"
#define TUI_CLEAR_LINE   "\033[2K"

/* Box drawing characters (Unicode) */
#define TUI_BOX_TL "┌"
#define TUI_BOX_TR "┐"
#define TUI_BOX_BL "└"
#define TUI_BOX_BR "┘"
#define TUI_BOX_H  "─"
#define TUI_BOX_V  "│"
#define TUI_BOX_LT "├"
#define TUI_BOX_RT "┤"

/* Progress bar characters */
#define TUI_PROG_FULL "█"
#define TUI_PROG_EMPTY "░"

/* Status symbols */
#define TUI_CHECK "✓"
#define TUI_CROSS "✗"

/* Sparkline characters (8 levels) */
extern const char *TUI_SPARK_CHARS[8];

/* Live metrics structure updated during test */
typedef struct tui_metrics_t {
	/* Progress */
	size_t frames_total;
	size_t frames_completed;
	size_t frames_succeeded;
	size_t frames_failed;

	/* Throughput */
	uint64_t bytes_written;
	uint64_t elapsed_ns;
	double throughput_mibs;
	double iops;

	/* Latency percentiles (nanoseconds) */
	uint64_t latency_min_ns;
	uint64_t latency_max_ns;
	uint64_t latency_p50_ns;
	uint64_t latency_p95_ns;
	uint64_t latency_p99_ns;

	/* Sparkline history for latency visualization */
	uint64_t sparkline_history[TUI_SPARKLINE_SIZE];
	int sparkline_idx;

	/* Percentile calculation buffer (circular, stores recent frame times) */
	uint64_t percentile_buffer[TUI_PERCENTILE_BUFFER_SIZE];
	size_t percentile_idx;
	size_t percentile_count;  /* Total samples added (for knowing if buffer is full) */

	/* I/O mode tracking */
	int frames_direct_io;
	int frames_buffered_io;
	io_mode_t current_io_mode;

	/* Test configuration */
	const char *profile_name;
	const char *target_path;
	size_t thread_count;
	const char *test_type;  /* "write" or "read" */
	filesystem_type_t fs_type;
} tui_metrics_t;

/* Percentile data for calculation */
typedef struct tui_percentiles_t {
	uint64_t p50;
	uint64_t p95;
	uint64_t p99;
	uint64_t p999;
} tui_percentiles_t;

/*
 * Initialize TUI mode
 * Sets up terminal for TUI display (hides cursor, etc.)
 * Returns 0 on success, non-zero on failure
 */
int tui_init(void);

/*
 * Cleanup TUI mode
 * Restores terminal to normal state
 */
void tui_cleanup(void);

/*
 * Initialize metrics structure with test configuration
 */
void tui_metrics_init(tui_metrics_t *metrics, const char *profile_name,
                      const char *target_path, size_t thread_count,
                      size_t frames_total, const char *test_type,
                      filesystem_type_t fs_type);

/*
 * Update metrics with a completed frame
 */
void tui_metrics_update(tui_metrics_t *metrics, uint64_t frame_time_ns,
                        uint64_t bytes, io_mode_t io_mode, int success);

/*
 * Calculate percentiles from completion time array
 */
tui_percentiles_t tui_calculate_percentiles(uint64_t *times, size_t count);

/*
 * Render the TUI display
 * Call this periodically (e.g., every 100ms or after each frame batch)
 * Note: This function calculates running percentiles, so metrics is non-const
 */
void tui_render(tui_metrics_t *metrics);

/*
 * Render final summary after test completion
 */
void tui_render_summary(const tui_metrics_t *metrics,
                        const test_result_t *result);

/*
 * Check if terminal supports TUI mode
 * Returns 1 if TUI is supported, 0 otherwise
 */
int tui_is_supported(void);

/*
 * Get terminal width (for adaptive rendering)
 */
int tui_get_terminal_width(void);

#endif /* FRAMETEST_TUI_H */
