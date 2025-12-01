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
#include "api_csv.h"

/* Buffer size for CSV encoding */
#define API_CSV_BUFFER_SIZE 131072

char* api_csv_metrics(const api_metrics_t *metrics)
{
	char *csv;

	if (!metrics)
		return NULL;

	csv = (char *)malloc(API_CSV_BUFFER_SIZE);
	if (!csv)
		return NULL;

	/* CSV header and single data row */
	snprintf(csv, API_CSV_BUFFER_SIZE,
		 "frames_completed,frames_total,bytes_processed,"
		 "elapsed_ns,throughput_mbps,iops,latency_avg_ms,"
		 "latency_min_ms,latency_max_ms,latency_p50_ms,"
		 "latency_p95_ms,latency_p99_ms,progress_percent\n"
		 "%llu,%llu,%llu,%llu,%.2f,%.2f,%.3f,%.3f,%.3f,"
		 "%.3f,%.3f,%.3f,%.1f\n",
		 (unsigned long long)metrics->frames_completed,
		 (unsigned long long)metrics->frames_total,
		 (unsigned long long)metrics->bytes_processed,
		 (unsigned long long)metrics->elapsed_ns,
		 metrics->throughput_mbps, metrics->iops,
		 metrics->latency_avg_ms, metrics->latency_min_ms,
		 metrics->latency_max_ms, metrics->latency_p50_ms,
		 metrics->latency_p95_ms, metrics->latency_p99_ms,
		 metrics->progress_percent);

	return csv;
}

char* api_csv_history(const api_history_t *history)
{
	char *csv;
	char *pos;
	size_t remaining;
	size_t i;
	int written;

	if (!history)
		return NULL;

	csv = (char *)malloc(API_CSV_BUFFER_SIZE);
	if (!csv)
		return NULL;

	pos = csv;
	remaining = API_CSV_BUFFER_SIZE;

	/* Write CSV header */
	written = snprintf(pos, remaining,
			   "frame_number,completion_time_ns,bytes_processed,"
			   "io_mode,success,error_message\n");
	pos += written;
	remaining -= written;

	/* Write frame entries */
	for (i = 0; i < history->count && remaining > 100; i++) {
		api_frame_entry_t *frame = &history->frames[i];

		written = snprintf(
			pos, remaining,
			"%llu,%llu,%llu,%s,%d,%s\n",
			(unsigned long long)frame->frame_number,
			(unsigned long long)frame->completion_time_ns,
			(unsigned long long)frame->bytes_processed,
			frame->io_mode, frame->success,
			frame->error_message);
		pos += written;
		remaining -= written;
	}

	return csv;
}

char* api_csv_summary(const api_summary_t *summary)
{
	char *csv;

	if (!summary)
		return NULL;

	csv = (char *)malloc(API_CSV_BUFFER_SIZE);
	if (!csv)
		return NULL;

	/* CSV header and single data row */
	snprintf(csv, API_CSV_BUFFER_SIZE,
		 "total_frames,successful_frames,failed_frames,"
		 "success_rate_percent,total_bytes,total_time_ns,"
		 "throughput_mbps,iops,direct_io_available,"
		 "is_remote_filesystem,error_count\n"
		 "%llu,%llu,%llu,%.2f,%llu,%llu,%.2f,%.2f,%d,%d,%d\n",
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

	return csv;
}
