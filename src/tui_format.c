/*
 * tui_format.c - TUI formatting and calculation utilities
 *
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
#include <string.h>
#include "tui_format.h"

/* ─────────────────────────────────────────────────────────────────────────────
 * Time Formatting
 * ───────────────────────────────────────────────────────────────────────────── */

char *format_time_human(uint64_t ns, char *buf, size_t bufsize)
{
	uint64_t seconds = ns / 1000000000UL;
	uint64_t remaining_ns;
	uint64_t days, hours, minutes;

	if (!buf || bufsize < 16)
		return buf;

	/* Handle very large durations (>= 24 hours) */
	if (seconds >= 86400) {
		days = seconds / 86400;
		remaining_ns = seconds % 86400;
		hours = remaining_ns / 3600;
		snprintf(buf, bufsize, "%lud %luh", (unsigned long)days,
			 (unsigned long)hours);
		return buf;
	}

	/* Handle hours (< 24 hours, >= 1 hour) */
	if (seconds >= 3600) {
		hours = seconds / 3600;
		remaining_ns = seconds % 3600;
		minutes = remaining_ns / 60;
		snprintf(buf, bufsize, "%luh %lum", (unsigned long)hours,
			 (unsigned long)minutes);
		return buf;
	}

	/* Handle minutes (< 1 hour, >= 1 minute) */
	if (seconds >= 60) {
		minutes = seconds / 60;
		remaining_ns = seconds % 60;
		snprintf(buf, bufsize, "%lum %lus", (unsigned long)minutes,
			 (unsigned long)remaining_ns);
		return buf;
	}

	/* Handle seconds (< 1 minute) */
	snprintf(buf, bufsize, "%lus", (unsigned long)seconds);
	return buf;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Data Size Formatting
 * ───────────────────────────────────────────────────────────────────────────── */

char *format_bytes_human(uint64_t bytes, char *buf, size_t bufsize)
{
	double gb;

	if (!buf || bufsize < 32)
		return buf;

	/* Calculate GB value */
	gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);

	/* Format with commas and GB suffix */
	/* Note: snprintf with %'llu not supported on all platforms,
	   so we'll format without commas for compatibility */
	snprintf(buf, bufsize, "%llu (%.1f GB)", (unsigned long long)bytes, gb);

	return buf;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * ETA and Duration Calculation
 * ───────────────────────────────────────────────────────────────────────────── */

int calculate_eta(tui_metrics_t *metrics)
{
	uint64_t frames_remaining;
	uint64_t avg_ns_per_frame;

	if (!metrics || metrics->frames_completed < 5 || metrics->elapsed_ns == 0)
		return 0;

	/* Calculate remaining frames */
	frames_remaining = metrics->frames_total > metrics->frames_completed ?
		metrics->frames_total - metrics->frames_completed : 0;

	/* Calculate average time per frame */
	avg_ns_per_frame = metrics->elapsed_ns / metrics->frames_completed;

	/* Calculate ETA */
	metrics->eta_ns = avg_ns_per_frame * frames_remaining;

	/* Calculate total estimated time */
	metrics->total_estimated_ns = metrics->elapsed_ns + metrics->eta_ns;

	return 1; /* Success */
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Trend Detection
 * ───────────────────────────────────────────────────────────────────────────── */

int detect_latency_trend(tui_metrics_t *metrics)
{
	uint64_t recent_sum = 0, previous_sum = 0;
	uint64_t recent_avg, previous_avg;
	size_t sample_count;
	int i;

	if (!metrics || metrics->percentile_count < 10)
		return 0;

	/* Calculate number of samples to use (min of 5 or available) */
	sample_count = metrics->percentile_count >= 10 ? 5 : 0;
	if (sample_count == 0)
		return 0;

	/* Sum last 5 samples (most recent) */
	for (i = 0; i < (int)sample_count; i++) {
		int idx = (metrics->percentile_idx - 1 - i) %
			  TUI_PERCENTILE_BUFFER_SIZE;
		if (idx < 0)
			idx += TUI_PERCENTILE_BUFFER_SIZE;
		recent_sum += metrics->percentile_buffer[idx];
	}

	/* Sum previous 5 samples (5-10 frames ago) */
	for (i = 0; i < (int)sample_count; i++) {
		int idx = (metrics->percentile_idx - 1 - sample_count - i) %
			  TUI_PERCENTILE_BUFFER_SIZE;
		if (idx < 0)
			idx += TUI_PERCENTILE_BUFFER_SIZE;
		previous_sum += metrics->percentile_buffer[idx];
	}

	recent_avg = recent_sum / sample_count;
	previous_avg = previous_sum / sample_count;

	/* Detect trend with 5% threshold */
	if (recent_avg < previous_avg * 95 / 100) {
		/* Improving: recent is 5% or more faster */
		metrics->latency_trend = 1;
		return 1;
	}

	if (recent_avg > previous_avg * 105 / 100) {
		/* Degrading: recent is 5% or more slower */
		metrics->latency_trend = -1;
		return -1;
	}

	/* Stable: within 5% threshold */
	metrics->latency_trend = 0;
	return 0;
}

const char *get_trend_arrow(int trend)
{
	if (trend > 0)
		return "↗";
	if (trend < 0)
		return "↘";
	return "→";
}
