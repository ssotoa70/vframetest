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
#include <stdio.h>
#include <string.h>
#include "api_json.h"

/* Buffer size for JSON encoding */
#define API_JSON_BUFFER_SIZE 65536

char* api_json_status(const api_status_t *status)
{
	char *json;

	if (!status)
		return NULL;

	json = (char *)malloc(API_JSON_BUFFER_SIZE);
	if (!json)
		return NULL;

	snprintf(json, API_JSON_BUFFER_SIZE,
		 "{"
		 "\"test_running\":%d,"
		 "\"test_paused\":%d,"
		 "\"test_mode\":\"%s\","
		 "\"profile_name\":\"%s\","
		 "\"test_path\":\"%s\""
		 "}",
		 status->test_running, status->test_paused,
		 status->test_mode, status->profile_name, status->test_path);

	return json;
}

char* api_json_metrics(const api_metrics_t *metrics)
{
	char *json;

	if (!metrics)
		return NULL;

	json = (char *)malloc(API_JSON_BUFFER_SIZE);
	if (!json)
		return NULL;

	snprintf(json, API_JSON_BUFFER_SIZE,
		 "{"
		 "\"frames_completed\":%llu,"
		 "\"frames_total\":%llu,"
		 "\"bytes_processed\":%llu,"
		 "\"elapsed_ns\":%llu,"
		 "\"throughput_mbps\":%.2f,"
		 "\"iops\":%.2f,"
		 "\"latency_avg_ms\":%.3f,"
		 "\"latency_min_ms\":%.3f,"
		 "\"latency_max_ms\":%.3f,"
		 "\"latency_p50_ms\":%.3f,"
		 "\"latency_p95_ms\":%.3f,"
		 "\"latency_p99_ms\":%.3f,"
		 "\"progress_percent\":%.1f"
		 "}",
		 (unsigned long long)metrics->frames_completed,
		 (unsigned long long)metrics->frames_total,
		 (unsigned long long)metrics->bytes_processed,
		 (unsigned long long)metrics->elapsed_ns,
		 metrics->throughput_mbps, metrics->iops,
		 metrics->latency_avg_ms, metrics->latency_min_ms,
		 metrics->latency_max_ms, metrics->latency_p50_ms,
		 metrics->latency_p95_ms, metrics->latency_p99_ms,
		 metrics->progress_percent);

	return json;
}

char* api_json_io_modes(const api_io_modes_t *io_modes)
{
	char *json;

	if (!io_modes)
		return NULL;

	json = (char *)malloc(API_JSON_BUFFER_SIZE);
	if (!json)
		return NULL;

	snprintf(json, API_JSON_BUFFER_SIZE,
		 "{"
		 "\"frames_direct_io\":%d,"
		 "\"frames_buffered_io\":%d,"
		 "\"fallback_count\":%d,"
		 "\"direct_io_percent\":%.1f,"
		 "\"buffered_io_percent\":%.1f"
		 "}",
		 io_modes->frames_direct_io,
		 io_modes->frames_buffered_io,
		 io_modes->fallback_count,
		 io_modes->direct_io_percent,
		 io_modes->buffered_io_percent);

	return json;
}

char* api_json_filesystem(const api_filesystem_t *filesystem)
{
	char *json;

	if (!filesystem)
		return NULL;

	json = (char *)malloc(API_JSON_BUFFER_SIZE);
	if (!json)
		return NULL;

	snprintf(json, API_JSON_BUFFER_SIZE,
		 "{"
		 "\"filesystem_type\":\"%s\","
		 "\"is_remote\":%d,"
		 "\"optimization_status\":\"%s\""
		 "}",
		 filesystem->filesystem_type, filesystem->is_remote,
		 filesystem->optimization_status);

	return json;
}

char* api_json_history(const api_history_t *history)
{
	char *json;
	char *pos;
	size_t remaining;
	size_t i;

	if (!history)
		return NULL;

	json = (char *)malloc(API_JSON_BUFFER_SIZE);
	if (!json)
		return NULL;

	pos = json;
	remaining = API_JSON_BUFFER_SIZE;

	/* Start JSON object */
	int written =
		snprintf(pos, remaining,
			 "{\"frames\":[");
	pos += written;
	remaining -= written;

	/* Add frame entries */
	for (i = 0; i < history->count && remaining > 100; i++) {
		api_frame_entry_t *frame = &history->frames[i];

		if (i > 0) {
			written = snprintf(pos, remaining, ",");
			pos += written;
			remaining -= written;
		}

		written = snprintf(
			pos, remaining,
			"{"
			"\"frame_number\":%llu,"
			"\"completion_time_ns\":%llu,"
			"\"bytes_processed\":%llu,"
			"\"io_mode\":\"%s\","
			"\"success\":%d,"
			"\"error_message\":\"%s\""
			"}",
			(unsigned long long)frame->frame_number,
			(unsigned long long)frame->completion_time_ns,
			(unsigned long long)frame->bytes_processed,
			frame->io_mode, frame->success,
			frame->error_message);
		pos += written;
		remaining -= written;
	}

	/* End JSON object */
	snprintf(pos, remaining,
		 "],"
		 "\"total_frames\":%zu,"
		 "\"offset\":%zu,"
		 "\"limit\":%zu"
		 "}",
		 history->total_frames, history->offset, history->limit);

	return json;
}

char* api_json_summary(const api_summary_t *summary)
{
	char *json;

	if (!summary)
		return NULL;

	json = (char *)malloc(API_JSON_BUFFER_SIZE);
	if (!json)
		return NULL;

	snprintf(json, API_JSON_BUFFER_SIZE,
		 "{"
		 "\"total_frames\":%llu,"
		 "\"successful_frames\":%llu,"
		 "\"failed_frames\":%llu,"
		 "\"success_rate_percent\":%.2f,"
		 "\"total_bytes\":%llu,"
		 "\"total_time_ns\":%llu,"
		 "\"throughput_mbps\":%.2f,"
		 "\"iops\":%.2f,"
		 "\"direct_io_available\":%d,"
		 "\"is_remote_filesystem\":%d,"
		 "\"error_count\":%d"
		 "}",
		 (unsigned long long)summary->total_frames,
		 (unsigned long long)summary->successful_frames,
		 (unsigned long long)summary->failed_frames,
		 summary->success_rate_percent,
		 (unsigned long long)summary->total_bytes,
		 (unsigned long long)summary->total_time_ns,
		 summary->throughput_mbps, summary->iops,
		 summary->direct_io_available,
		 summary->is_remote_filesystem,
		 summary->error_count);

	return json;
}

char* api_json_error(int http_status, const char *message)
{
	char *json;

	if (!message)
		message = "Unknown error";

	json = (char *)malloc(API_JSON_BUFFER_SIZE);
	if (!json)
		return NULL;

	snprintf(json, API_JSON_BUFFER_SIZE,
		 "{\"error\":%d,\"message\":\"%s\"}",
		 http_status, message);

	return json;
}

char* api_json_response(int http_status, const char *data)
{
	char *json;

	if (!data)
		data = "{}";

	json = (char *)malloc(API_JSON_BUFFER_SIZE);
	if (!json)
		return NULL;

	snprintf(json, API_JSON_BUFFER_SIZE,
		 "{\"status\":%d,\"data\":%s}", http_status, data);

	return json;
}
