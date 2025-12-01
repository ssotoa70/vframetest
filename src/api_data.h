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

#ifndef VFRAMETEST_API_DATA_H
#define VFRAMETEST_API_DATA_H

#include <stdint.h>
#include <stddef.h>
#include "frametest.h"

/* API Data Structures for REST endpoints */

/* Status information */
typedef struct {
	int test_running;
	int test_paused;
	char test_mode[32];
	char profile_name[64];
	char test_path[512];
} api_status_t;

/* Real-time metrics */
typedef struct {
	uint64_t frames_completed;
	uint64_t frames_total;
	uint64_t bytes_processed;
	uint64_t elapsed_ns;
	double throughput_mbps;
	double iops;
	double latency_avg_ms;
	double latency_min_ms;
	double latency_max_ms;
	double latency_p50_ms;
	double latency_p95_ms;
	double latency_p99_ms;
	double progress_percent;
} api_metrics_t;

/* I/O mode statistics */
typedef struct {
	int frames_direct_io;
	int frames_buffered_io;
	int fallback_count;
	double direct_io_percent;
	double buffered_io_percent;
} api_io_modes_t;

/* Filesystem information */
typedef struct {
	char filesystem_type[32];
	int is_remote;
	char optimization_status[64];
} api_filesystem_t;

/* Individual frame history entry */
typedef struct {
	uint64_t frame_number;
	uint64_t completion_time_ns;
	uint64_t bytes_processed;
	char io_mode[16];
	int success;
	char error_message[128];
} api_frame_entry_t;

/* Frame history with pagination */
typedef struct {
	api_frame_entry_t *frames;
	size_t count;
	size_t total_frames;
	size_t offset;
	size_t limit;
} api_history_t;

/* Test summary */
typedef struct {
	uint64_t total_frames;
	uint64_t successful_frames;
	uint64_t failed_frames;
	double success_rate_percent;
	uint64_t total_bytes;
	uint64_t total_time_ns;
	double throughput_mbps;
	double iops;
	int direct_io_available;
	int is_remote_filesystem;
	int error_count;
} api_summary_t;

/* Global data context for thread-safe access */
typedef struct {
	api_status_t status;
	api_metrics_t metrics;
	api_io_modes_t io_modes;
	api_filesystem_t filesystem;
	api_summary_t summary;
	api_frame_entry_t *frame_history;
	size_t frame_history_capacity;
	size_t frame_history_count;
} api_data_context_t;

/* Initialize API data context */
int api_data_init(api_data_context_t *ctx);

/* Clean up API data context */
void api_data_cleanup(api_data_context_t *ctx);

/* Update metrics from test result */
void api_data_update_from_result(api_data_context_t *ctx,
                                 const test_result_t *result);

/* Record frame completion */
void api_data_record_frame(api_data_context_t *ctx,
                          uint64_t frame_number,
                          uint64_t completion_time_ns,
                          uint64_t bytes_processed,
                          const char *io_mode,
                          int success,
                          const char *error_msg);

/* Get current status */
api_status_t api_data_get_status(const api_data_context_t *ctx);

/* Get current metrics */
api_metrics_t api_data_get_metrics(const api_data_context_t *ctx);

/* Get I/O mode statistics */
api_io_modes_t api_data_get_io_modes(const api_data_context_t *ctx);

/* Get filesystem information */
api_filesystem_t api_data_get_filesystem(const api_data_context_t *ctx);

/* Get frame history with pagination */
api_history_t* api_data_get_history(const api_data_context_t *ctx,
                                   size_t offset,
                                   size_t limit);

/* Free history returned from api_data_get_history */
void api_data_free_history(api_history_t *history);

/* Get test summary */
api_summary_t api_data_get_summary(const api_data_context_t *ctx);

/* Convert nanoseconds to milliseconds */
static inline double api_ns_to_ms(uint64_t ns)
{
	return (double)ns / 1000000.0;
}

/* Convert bytes to megabytes */
static inline double api_bytes_to_mb(uint64_t bytes)
{
	return (double)bytes / (1024.0 * 1024.0);
}

#endif /* VFRAMETEST_API_DATA_H */
