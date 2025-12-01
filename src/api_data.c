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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include "api_data.h"

/* Thread synchronization for data access */
static pthread_mutex_t api_data_lock = PTHREAD_MUTEX_INITIALIZER;

/* Frame history initial capacity */
#define API_FRAME_HISTORY_INITIAL_CAPACITY 1000

int api_data_init(api_data_context_t *ctx)
{
	if (!ctx)
		return -1;

	memset(ctx, 0, sizeof(*ctx));

	/* Initialize frame history buffer */
	ctx->frame_history_capacity = API_FRAME_HISTORY_INITIAL_CAPACITY;
	ctx->frame_history = (api_frame_entry_t *)malloc(
		ctx->frame_history_capacity * sizeof(api_frame_entry_t));

	if (!ctx->frame_history) {
		ctx->frame_history_capacity = 0;
		return -1;
	}

	return 0;
}

void api_data_cleanup(api_data_context_t *ctx)
{
	if (!ctx)
		return;

	if (ctx->frame_history) {
		free(ctx->frame_history);
		ctx->frame_history = NULL;
	}
	ctx->frame_history_capacity = 0;
	ctx->frame_history_count = 0;
}

void api_data_update_from_result(api_data_context_t *ctx,
                                 const test_result_t *result)
{
	if (!ctx || !result)
		return;

	pthread_mutex_lock(&api_data_lock);

	/* Update metrics */
	ctx->metrics.frames_completed = result->frames_written;
	ctx->metrics.bytes_processed = result->bytes_written;
	ctx->metrics.elapsed_ns = result->time_taken_ns;

	/* Calculate throughput (MB/s) */
	if (result->time_taken_ns > 0) {
		double elapsed_sec = (double)result->time_taken_ns / 1000000000.0;
		double mb = api_bytes_to_mb(result->bytes_written);
		ctx->metrics.throughput_mbps = mb / elapsed_sec;
		ctx->metrics.iops = result->frames_written / elapsed_sec;

		/* Calculate average latency */
		ctx->metrics.latency_avg_ms =
			api_ns_to_ms(result->avg_frame_time_ns);
		ctx->metrics.latency_min_ms =
			api_ns_to_ms(result->min_frame_time_ns);
		ctx->metrics.latency_max_ms =
			api_ns_to_ms(result->max_frame_time_ns);
	}

	/* Update I/O modes */
	ctx->io_modes.frames_direct_io = result->frames_direct_io;
	ctx->io_modes.frames_buffered_io = result->frames_buffered_io;
	ctx->io_modes.fallback_count = result->fallback_count;

	if (result->frames_written > 0) {
		ctx->io_modes.direct_io_percent =
			(result->frames_direct_io * 100.0) /
			result->frames_written;
		ctx->io_modes.buffered_io_percent =
			(result->frames_buffered_io * 100.0) /
			result->frames_written;
	}

	/* Update filesystem info */
	const char *fs_types[] = {"LOCAL", "SMB", "NFS", "OTHER"};
	int fs_idx = result->filesystem_type;
	if (fs_idx < 0 || fs_idx > 3)
		fs_idx = 3;
	strncpy(ctx->filesystem.filesystem_type, fs_types[fs_idx],
		sizeof(ctx->filesystem.filesystem_type) - 1);

	ctx->filesystem.is_remote = result->is_remote_filesystem;
	if (result->is_remote_filesystem) {
		strncpy(ctx->filesystem.optimization_status, "optimized-remote",
			sizeof(ctx->filesystem.optimization_status) - 1);
	} else {
		strncpy(ctx->filesystem.optimization_status, "local",
			sizeof(ctx->filesystem.optimization_status) - 1);
	}

	/* Update summary */
	ctx->summary.total_frames = result->frames_written;
	ctx->summary.successful_frames = result->frames_succeeded;
	ctx->summary.failed_frames = result->frames_failed;
	ctx->summary.success_rate_percent = result->success_rate_percent;
	ctx->summary.total_bytes = result->bytes_written;
	ctx->summary.total_time_ns = result->time_taken_ns;
	ctx->summary.throughput_mbps = ctx->metrics.throughput_mbps;
	ctx->summary.iops = ctx->metrics.iops;
	ctx->summary.direct_io_available = result->direct_io_available;
	ctx->summary.is_remote_filesystem = result->is_remote_filesystem;
	ctx->summary.error_count = result->error_count;

	pthread_mutex_unlock(&api_data_lock);
}

void api_data_record_frame(api_data_context_t *ctx,
                          uint64_t frame_number,
                          uint64_t completion_time_ns,
                          uint64_t bytes_processed,
                          const char *io_mode,
                          int success,
                          const char *error_msg)
{
	if (!ctx)
		return;

	pthread_mutex_lock(&api_data_lock);

	/* Expand history buffer if needed */
	if (ctx->frame_history_count >= ctx->frame_history_capacity) {
		size_t new_capacity = ctx->frame_history_capacity * 2;
		api_frame_entry_t *new_history =
			(api_frame_entry_t *)realloc(ctx->frame_history,
						    new_capacity *
							    sizeof(api_frame_entry_t));

		if (new_history) {
			ctx->frame_history = new_history;
			ctx->frame_history_capacity = new_capacity;
		} else {
			/* If realloc fails, keep existing data */
			pthread_mutex_unlock(&api_data_lock);
			return;
		}
	}

	/* Add frame entry */
	api_frame_entry_t *entry = &ctx->frame_history[ctx->frame_history_count];
	entry->frame_number = frame_number;
	entry->completion_time_ns = completion_time_ns;
	entry->bytes_processed = bytes_processed;
	entry->success = success;

	if (io_mode)
		strncpy(entry->io_mode, io_mode, sizeof(entry->io_mode) - 1);
	else
		strcpy(entry->io_mode, "unknown");

	if (error_msg)
		strncpy(entry->error_message, error_msg,
			sizeof(entry->error_message) - 1);
	else
		strcpy(entry->error_message, "");

	ctx->frame_history_count++;

	pthread_mutex_unlock(&api_data_lock);
}

api_status_t api_data_get_status(const api_data_context_t *ctx)
{
	api_status_t status;

	if (!ctx) {
		memset(&status, 0, sizeof(status));
		return status;
	}

	pthread_mutex_lock(&api_data_lock);
	status = ctx->status;
	pthread_mutex_unlock(&api_data_lock);

	return status;
}

api_metrics_t api_data_get_metrics(const api_data_context_t *ctx)
{
	api_metrics_t metrics;

	if (!ctx) {
		memset(&metrics, 0, sizeof(metrics));
		return metrics;
	}

	pthread_mutex_lock(&api_data_lock);
	metrics = ctx->metrics;
	pthread_mutex_unlock(&api_data_lock);

	return metrics;
}

api_io_modes_t api_data_get_io_modes(const api_data_context_t *ctx)
{
	api_io_modes_t io_modes;

	if (!ctx) {
		memset(&io_modes, 0, sizeof(io_modes));
		return io_modes;
	}

	pthread_mutex_lock(&api_data_lock);
	io_modes = ctx->io_modes;
	pthread_mutex_unlock(&api_data_lock);

	return io_modes;
}

api_filesystem_t api_data_get_filesystem(const api_data_context_t *ctx)
{
	api_filesystem_t filesystem;

	if (!ctx) {
		memset(&filesystem, 0, sizeof(filesystem));
		return filesystem;
	}

	pthread_mutex_lock(&api_data_lock);
	filesystem = ctx->filesystem;
	pthread_mutex_unlock(&api_data_lock);

	return filesystem;
}

api_history_t* api_data_get_history(const api_data_context_t *ctx,
                                   size_t offset,
                                   size_t limit)
{
	api_history_t *history;
	size_t count;

	if (!ctx)
		return NULL;

	history = (api_history_t *)malloc(sizeof(api_history_t));
	if (!history)
		return NULL;

	pthread_mutex_lock(&api_data_lock);

	history->total_frames = ctx->frame_history_count;
	history->offset = offset;
	history->limit = limit;

	/* Calculate actual count to return */
	if (offset >= ctx->frame_history_count) {
		count = 0;
	} else {
		count = ctx->frame_history_count - offset;
		if (count > limit)
			count = limit;
	}

	history->count = count;

	if (count > 0) {
		history->frames =
			(api_frame_entry_t *)malloc(count * sizeof(api_frame_entry_t));
		if (history->frames) {
			memcpy(history->frames,
			       &ctx->frame_history[offset],
			       count * sizeof(api_frame_entry_t));
		} else {
			history->count = 0;
			count = 0;
		}
	} else {
		history->frames = NULL;
	}

	pthread_mutex_unlock(&api_data_lock);

	return history;
}

void api_data_free_history(api_history_t *history)
{
	if (!history)
		return;

	if (history->frames)
		free(history->frames);
	free(history);
}

api_summary_t api_data_get_summary(const api_data_context_t *ctx)
{
	api_summary_t summary;

	if (!ctx) {
		memset(&summary, 0, sizeof(summary));
		return summary;
	}

	pthread_mutex_lock(&api_data_lock);
	summary = ctx->summary;
	pthread_mutex_unlock(&api_data_lock);

	return summary;
}
