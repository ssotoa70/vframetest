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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifdef __APPLE__
#include <sys/ioctl.h>
#endif
#ifdef __linux__
#include <sys/ioctl.h>
#endif

#include "tui.h"

/* Sparkline characters representing 8 intensity levels */
const char *TUI_SPARK_CHARS[8] = { "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█" };

/* Static flag to track initialization state */
static int tui_initialized = 0;

/* Comparison function for qsort */
static int compare_uint64(const void *a, const void *b)
{
	uint64_t va = *(const uint64_t *)a;
	uint64_t vb = *(const uint64_t *)b;
	if (va < vb)
		return -1;
	if (va > vb)
		return 1;
	return 0;
}

/* Signal handler to restore terminal on interrupt */
static void tui_signal_handler(int sig)
{
	(void)sig;
	tui_cleanup();
	exit(1);
}

int tui_is_supported(void)
{
	/* Check if stdout is a terminal */
	if (!isatty(STDOUT_FILENO))
		return 0;

	/* Check for TERM environment variable */
	const char *term = getenv("TERM");
	if (!term || strcmp(term, "dumb") == 0)
		return 0;

	return 1;
}

int tui_get_terminal_width(void)
{
#if defined(__APPLE__) || defined(__linux__)
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
		return TUI_WIDTH;
	return ws.ws_col > 0 ? ws.ws_col : TUI_WIDTH;
#else
	return TUI_WIDTH;
#endif
}

int tui_init(void)
{
	if (!tui_is_supported())
		return 1;

	/* Set up signal handlers for clean exit */
	signal(SIGINT, tui_signal_handler);
	signal(SIGTERM, tui_signal_handler);

	/* Hide cursor and clear screen */
	printf("%s%s", TUI_HIDE_CURSOR, TUI_CLEAR_SCREEN);
	fflush(stdout);

	tui_initialized = 1;
	return 0;
}

void tui_cleanup(void)
{
	if (!tui_initialized)
		return;

	/* Show cursor and reset colors */
	printf("%s%s\n", TUI_SHOW_CURSOR, TUI_RESET);
	fflush(stdout);

	tui_initialized = 0;
}

void tui_metrics_init(tui_metrics_t *metrics, const char *profile_name,
		      const char *target_path, size_t thread_count,
		      size_t frames_total, const char *test_type,
		      filesystem_type_t fs_type)
{
	if (!metrics)
		return;

	memset(metrics, 0, sizeof(*metrics));
	metrics->profile_name = profile_name;
	metrics->target_path = target_path;
	metrics->thread_count = thread_count;
	metrics->frames_total = frames_total;
	metrics->test_type = test_type;
	metrics->fs_type = fs_type;
	metrics->latency_min_ns = UINT64_MAX;
	metrics->sparkline_idx = 0;
	metrics->percentile_idx = 0;
	metrics->percentile_count = 0;
}

void tui_metrics_update(tui_metrics_t *metrics, uint64_t frame_time_ns,
			uint64_t bytes, io_mode_t io_mode, int success)
{
	if (!metrics)
		return;

	metrics->frames_completed++;
	metrics->bytes_written += bytes;
	metrics->current_io_mode = io_mode;

	if (success) {
		metrics->frames_succeeded++;
	} else {
		metrics->frames_failed++;
	}

	if (io_mode == IO_MODE_DIRECT) {
		metrics->frames_direct_io++;
	} else if (io_mode == IO_MODE_BUFFERED) {
		metrics->frames_buffered_io++;
	}

	/* Update min/max latency */
	if (frame_time_ns > 0 && frame_time_ns < metrics->latency_min_ns)
		metrics->latency_min_ns = frame_time_ns;
	if (frame_time_ns > metrics->latency_max_ns)
		metrics->latency_max_ns = frame_time_ns;

	/* Add to sparkline history (circular buffer) */
	metrics->sparkline_history[metrics->sparkline_idx] = frame_time_ns;
	metrics->sparkline_idx =
		(metrics->sparkline_idx + 1) % TUI_SPARKLINE_SIZE;

	/* Add to percentile buffer (circular buffer for running percentiles) */
	if (frame_time_ns > 0) {
		metrics->percentile_buffer[metrics->percentile_idx] =
			frame_time_ns;
		metrics->percentile_idx = (metrics->percentile_idx + 1) %
					  TUI_PERCENTILE_BUFFER_SIZE;
		metrics->percentile_count++;
	}
}

tui_percentiles_t tui_calculate_percentiles(uint64_t *times, size_t count)
{
	tui_percentiles_t result = { 0 };

	if (!times || count == 0)
		return result;

	/* Create a sorted copy */
	uint64_t *sorted = malloc(count * sizeof(uint64_t));
	if (!sorted)
		return result;

	memcpy(sorted, times, count * sizeof(uint64_t));
	qsort(sorted, count, sizeof(uint64_t), compare_uint64);

	/* Calculate percentile indices */
	result.p50 = sorted[(size_t)(count * 0.50)];
	result.p95 = sorted[(size_t)(count * 0.95)];
	result.p99 = sorted[(size_t)(count * 0.99)];
	result.p999 =
		sorted[count > 1000 ? (size_t)(count * 0.999) : count - 1];

	free(sorted);
	return result;
}

/* Calculate running percentiles from the metrics percentile buffer */
static void calculate_running_percentiles(tui_metrics_t *metrics)
{
	if (!metrics || metrics->percentile_count == 0)
		return;

	/* Determine how many samples we have in the buffer */
	size_t count =
		(metrics->percentile_count < TUI_PERCENTILE_BUFFER_SIZE) ?
			metrics->percentile_count :
			TUI_PERCENTILE_BUFFER_SIZE;

	if (count < 2)
		return;

	/* Create a sorted copy of the buffer */
	uint64_t *sorted = malloc(count * sizeof(uint64_t));
	if (!sorted)
		return;

	memcpy(sorted, metrics->percentile_buffer, count * sizeof(uint64_t));
	qsort(sorted, count, sizeof(uint64_t), compare_uint64);

	/* Calculate percentiles from sorted data */
	metrics->latency_p50_ns = sorted[(size_t)(count * 0.50)];
	metrics->latency_p95_ns = sorted[(size_t)(count * 0.95)];
	metrics->latency_p99_ns = sorted[(size_t)(count * 0.99)];

	free(sorted);
}

/* Helper: Format nanoseconds as human readable */
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

/* Get filesystem type string */
static const char *get_fs_type_str(filesystem_type_t fs_type)
{
	switch (fs_type) {
	case FILESYSTEM_LOCAL:
		return "LOCAL";
	case FILESYSTEM_SMB:
		return "SMB";
	case FILESYSTEM_NFS:
		return "NFS";
	default:
		return "OTHER";
	}
}

/* Get I/O mode string */
static const char *get_io_mode_str(io_mode_t mode)
{
	switch (mode) {
	case IO_MODE_DIRECT:
		return "Direct";
	case IO_MODE_BUFFERED:
		return "Buffered";
	default:
		return "---";
	}
}

/* Helper: print spaces for padding */
static void print_pad(int n)
{
	while (n-- > 0)
		putchar(' ');
}

void tui_render(tui_metrics_t *metrics)
{
	char lat_p50[16], lat_p99[16], lat_min[16], lat_max[16];
	char line[256];
	int len;
	const int W = 58; /* Content width between │ and │ */

	if (!metrics)
		return;

	/* Calculate running percentiles from recent samples */
	calculate_running_percentiles(metrics);

	/* Move cursor to home position */
	printf("%s", TUI_HOME);

	/* Calculate live metrics */
	double elapsed_sec = (double)metrics->elapsed_ns / 1000000000.0;
	double throughput_mibs = 0.0;
	double iops = 0.0;

	if (elapsed_sec > 0.001) {
		throughput_mibs =
			((double)metrics->bytes_written / (1024.0 * 1024.0)) /
			elapsed_sec;
		iops = (double)metrics->frames_completed / elapsed_sec;
	}

	/* Calculate progress */
	int percent = 0;
	if (metrics->frames_total > 0) {
		percent = (int)((metrics->frames_completed * 100) /
				metrics->frames_total);
	}

	/* Format latencies */
	format_latency(metrics->latency_p50_ns, lat_p50, sizeof(lat_p50));
	format_latency(metrics->latency_p99_ns, lat_p99, sizeof(lat_p99));
	format_latency(metrics->latency_min_ns, lat_min, sizeof(lat_min));
	format_latency(metrics->latency_max_ns, lat_max, sizeof(lat_max));

	/* Row 1: Top border */
	printf("\xe2\x94\x8c"); /* ┌ */
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80"); /* ─ */
	printf("\xe2\x94\x90\n"); /* ┐ */

	/* Row 2: Title */
	len = snprintf(line, sizeof(line), "  vframetest v%d.%d.%d - %s test",
		       MAJOR, MINOR, PATCH, metrics->test_type);
	printf("\xe2\x94\x82%s", line); /* │ */
	print_pad(W - len);
	printf("\xe2\x94\x82\n"); /* │ */

	/* Row 3: Separator */
	printf("\xe2\x94\x9c"); /* ├ */
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\xa4\n"); /* ┤ */

	/* Row 4: Profile info */
	len = snprintf(line, sizeof(line),
		       "  Profile: %-12s  Threads: %-2zu  FS: %s",
		       metrics->profile_name, metrics->thread_count,
		       get_fs_type_str(metrics->fs_type));
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 5: Target path */
	len = snprintf(line, sizeof(line), "  Target: %.46s",
		       metrics->target_path);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 6: Separator */
	printf("\xe2\x94\x9c");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\xa4\n");

	/* Row 7: Progress bar - careful with UTF-8 block chars */
	int bar_width = 30;
	int filled = (metrics->frames_total > 0) ?
			     (int)((metrics->frames_completed * bar_width) /
				   metrics->frames_total) :
			     0;
	printf("\xe2\x94\x82  Progress: [");
	for (int i = 0; i < bar_width; i++) {
		if (i < filled)
			printf("%s\xe2\x96\x88%s", TUI_GREEN,
			       TUI_RESET); /* █ */
		else
			printf("\xe2\x96\x91"); /* ░ */
	}
	/* "  Progress: [" = 13, bar = 30, "] XXX%" = 6, total = 49, need W-49 = 9 spaces */
	printf("] %3d%%", percent);
	print_pad(W - 49);
	printf("\xe2\x94\x82\n");

	/* Row 8: Frame count */
	len = snprintf(line, sizeof(line), "  Frames: %zu / %zu",
		       metrics->frames_completed, metrics->frames_total);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 9: Separator */
	printf("\xe2\x94\x9c");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\xa4\n");

	/* Row 10: Throughput */
	len = snprintf(line, sizeof(line),
		       "  Throughput: %.1f MiB/s   IOPS: %.0f", throughput_mibs,
		       iops);
	printf("\xe2\x94\x82  Throughput: %s%.1f MiB/s%s   IOPS: %.0f",
	       TUI_CYAN, throughput_mibs, TUI_RESET, iops);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 11: Latency P50/P99 */
	len = snprintf(line, sizeof(line), "  Latency:    P50: %-8s  P99: %-8s",
		       lat_p50, lat_p99);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 12: Latency Min/Max */
	len = snprintf(line, sizeof(line), "              Min: %-8s  Max: %-8s",
		       lat_min, lat_max);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 13: Separator */
	printf("\xe2\x94\x9c");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\xa4\n");

	/* Row 14: Sparkline trend - count display columns not bytes */
	printf("\xe2\x94\x82  Trend: ");
	int spark_cols = 0;
	for (int i = 0; i < TUI_SPARKLINE_SIZE; i++) {
		int idx = (metrics->sparkline_idx + i) % TUI_SPARKLINE_SIZE;
		uint64_t val = metrics->sparkline_history[idx];
		if (val == 0) {
			printf("%s", TUI_SPARK_CHARS[0]);
		} else {
			uint64_t min_v = metrics->latency_min_ns;
			uint64_t max_v = metrics->latency_max_ns;
			int level = 0;
			if (max_v > min_v && val >= min_v) {
				level = (int)(((val - min_v) * 7) /
					      (max_v - min_v));
				if (level > 7)
					level = 7;
			}
			printf("%s", TUI_SPARK_CHARS[level]);
		}
		spark_cols++;
	}
	/* "  Trend: " = 9 chars, sparkline = TUI_SPARKLINE_SIZE cols */
	print_pad(W - 9 - spark_cols);
	printf("\xe2\x94\x82\n");

	/* Row 15: Separator */
	printf("\xe2\x94\x9c");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\xa4\n");

	/* Row 16: Status - ✓ and ✗ are each 1 display column */
	len = snprintf(line, sizeof(line), "  X %zu ok   X %zu fail   I/O: %s",
		       metrics->frames_succeeded, metrics->frames_failed,
		       get_io_mode_str(metrics->current_io_mode));
	printf("\xe2\x94\x82  %s\xe2\x9c\x93%s %zu ok   %s\xe2\x9c\x97%s %zu fail   I/O: %s",
	       TUI_GREEN, TUI_RESET, metrics->frames_succeeded, TUI_RED,
	       TUI_RESET, metrics->frames_failed,
	       get_io_mode_str(metrics->current_io_mode));
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 17: Bottom border */
	printf("\xe2\x94\x94"); /* └ */
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\x98\n"); /* ┘ */

	fflush(stdout);
}

void tui_render_summary(const tui_metrics_t *metrics,
			const test_result_t *result)
{
	char lat_min[16], lat_max[16], lat_avg[16];
	char line[256];
	int len;
	const int W = 58;

	if (!metrics || !result)
		return;

	/* Clear screen */
	printf("%s%s", TUI_CLEAR_SCREEN, TUI_HOME);

	/* Calculate metrics */
	double elapsed_sec = (double)result->time_taken_ns / 1000000000.0;
	double throughput = 0.0;
	double iops_val = 0.0;

	if (elapsed_sec > 0.001) {
		throughput =
			((double)result->bytes_written / (1024.0 * 1024.0)) /
			elapsed_sec;
		iops_val = (double)result->frames_written / elapsed_sec;
	}

	/* Calculate latency from completion data (most reliable source) */
	uint64_t min_ns = UINT64_MAX;
	uint64_t max_ns = 0;
	uint64_t avg_ns = 0;

	if (result->completion && result->frames_written > 0) {
		uint64_t total = 0;
		for (size_t i = 0; i < result->frames_written; i++) {
			uint64_t frame_dur = result->completion[i].frame -
					     result->completion[i].start;
			if (frame_dur > 0) {
				total += frame_dur;
				if (frame_dur < min_ns)
					min_ns = frame_dur;
				if (frame_dur > max_ns)
					max_ns = frame_dur;
			}
		}
		avg_ns = total / result->frames_written;
	}

	/* Fallback to metrics if completion data not available */
	if (min_ns == UINT64_MAX && metrics->latency_min_ns != UINT64_MAX)
		min_ns = metrics->latency_min_ns;
	if (max_ns == 0 && metrics->latency_max_ns > 0)
		max_ns = metrics->latency_max_ns;

	format_latency(min_ns, lat_min, sizeof(lat_min));
	format_latency(max_ns, lat_max, sizeof(lat_max));
	format_latency(avg_ns, lat_avg, sizeof(lat_avg));

	/* Row 1: Top border */
	printf("\xe2\x94\x8c");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\x90\n");

	/* Row 2: Title with checkmark - center "✓ Test Complete" (15 display cols) */
	int title_len =
		15; /* "✓ Test Complete" = 1 + 1 + 13 = 15 display cols */
	int left_pad = (W - title_len) / 2;
	int right_pad = W - title_len - left_pad;
	printf("\xe2\x94\x82");
	print_pad(left_pad);
	printf("%s\xe2\x9c\x93 Test Complete%s", TUI_GREEN, TUI_RESET);
	print_pad(right_pad);
	printf("\xe2\x94\x82\n");

	/* Row 3: Separator */
	printf("\xe2\x94\x9c");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\xa4\n");

	/* Row 4: Profile */
	len = snprintf(line, sizeof(line), "  Profile: %-12s   Test: %s",
		       metrics->profile_name, metrics->test_type);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 5: Target */
	len = snprintf(line, sizeof(line), "  Target: %.46s",
		       metrics->target_path);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 6: Separator */
	printf("\xe2\x94\x9c");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\xa4\n");

	/* Row 7: Frames and Time */
	len = snprintf(line, sizeof(line), "  Frames: %llu   Time: %.2f sec",
		       (unsigned long long)result->frames_written, elapsed_sec);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 8: Throughput */
	len = snprintf(line, sizeof(line),
		       "  Throughput: %.1f MiB/s   IOPS: %.0f", throughput,
		       iops_val);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 9: Success */
	len = snprintf(line, sizeof(line), "  Success: %.1f%%",
		       result->success_rate_percent);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 10: Separator */
	printf("\xe2\x94\x9c");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\xa4\n");

	/* Row 11: Latency */
	len = snprintf(line, sizeof(line),
		       "  Latency: Min: %-8s Avg: %-8s Max: %-8s", lat_min,
		       lat_avg, lat_max);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 12: I/O Mode */
	len = snprintf(line, sizeof(line),
		       "  I/O Mode: Direct: %d   Buffered: %d",
		       result->frames_direct_io, result->frames_buffered_io);
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 13: Filesystem */
	len = snprintf(line, sizeof(line), "  Filesystem: %s",
		       get_fs_type_str(result->filesystem_type));
	printf("\xe2\x94\x82%s", line);
	print_pad(W - len);
	printf("\xe2\x94\x82\n");

	/* Row 14: Bottom border */
	printf("\xe2\x94\x94");
	for (int i = 0; i < W; i++)
		printf("\xe2\x94\x80");
	printf("\xe2\x94\x98\n");
	printf("\n");

	fflush(stdout);
}
