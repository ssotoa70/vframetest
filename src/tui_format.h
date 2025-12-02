/*
 * tui_format.h - TUI formatting and calculation utilities
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

#ifndef FRAMETEST_TUI_FORMAT_H
#define FRAMETEST_TUI_FORMAT_H

#include <stdint.h>
#include <stddef.h>
#include "tui.h"

/* ─────────────────────────────────────────────────────────────────────────────
 * Time Formatting
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * format_time_human - Convert nanoseconds to human-readable time format
 * @ns: Time in nanoseconds
 * @buf: Output buffer
 * @bufsize: Size of output buffer
 *
 * Formats nanoseconds into readable time strings:
 * - < 60s:    "45s"
 * - < 1h:     "2m 15s"
 * - < 24h:    "1h 30m"
 * - >= 24h:   "2d 5h"
 *
 * Returns the buffer pointer for convenience.
 */
char *format_time_human(uint64_t ns, char *buf, size_t bufsize);

/* ─────────────────────────────────────────────────────────────────────────────
 * Data Size Formatting
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * format_bytes_human - Convert bytes to human-readable format with GB suffix
 * @bytes: Number of bytes
 * @buf: Output buffer
 * @bufsize: Size of output buffer
 *
 * Formats bytes as: "5,988,897,280 (5.6 GB)"
 *
 * Returns the buffer pointer for convenience.
 */
char *format_bytes_human(uint64_t bytes, char *buf, size_t bufsize);

/* ─────────────────────────────────────────────────────────────────────────────
 * ETA and Duration Calculation
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * calculate_eta - Estimate remaining time based on frame progress
 * @metrics: Metrics structure to update with ETA
 *
 * Calculates estimated time remaining using average frame time.
 * Only updates if at least 5 frames have completed.
 *
 * Updates metrics->eta_ns and metrics->total_estimated_ns.
 * Returns 0 if insufficient data, 1 if calculation successful.
 */
int calculate_eta(tui_metrics_t *metrics);

/* ─────────────────────────────────────────────────────────────────────────────
 * Trend Detection
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * detect_latency_trend - Analyze latency trend from percentile buffer
 * @metrics: Metrics structure to update with trend
 *
 * Compares recent 5 frames with previous 5 frames from percentile buffer:
 * - Returns 1 if improving (recent < previous × 0.95)
 * - Returns -1 if degrading (recent > previous × 1.05)
 * - Returns 0 if stable (within 5% threshold)
 *
 * Only calculates if at least 10 samples in buffer.
 * Updates metrics->latency_trend.
 */
int detect_latency_trend(tui_metrics_t *metrics);

/**
 * get_trend_arrow - Return arrow string for trend display
 * @trend: Trend value (-1, 0, or 1)
 *
 * Returns:
 * - "↗" for improving (trend > 0)
 * - "↘" for degrading (trend < 0)
 * - "→" for stable (trend == 0)
 */
const char *get_trend_arrow(int trend);

#endif /* FRAMETEST_TUI_FORMAT_H */
