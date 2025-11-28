/*
 * This file is part of tframetest.
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

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include "frametest.h"

enum CompletionStat {
	COMP_FRAME = 0,
	COMP_OPEN,
	COMP_IO,
	COMP_CLOSE,
};

/* Phase 2: Helper function to get filesystem type name */
static const char *get_filesystem_name(filesystem_type_t fs_type)
{
	switch (fs_type) {
	case FILESYSTEM_LOCAL:
		return "LOCAL";
	case FILESYSTEM_SMB:
		return "SMB";
	case FILESYSTEM_NFS:
		return "NFS";
	case FILESYSTEM_OTHER:
	default:
		return "OTHER";
	}
}

static void print_stat_about(const test_result_t *res, const char *label,
			     enum CompletionStat stat, int csv)
{
	uint64_t min = SIZE_MAX;
	uint64_t max = 0;
	uint64_t total = 0;
	size_t i;

	if (res->frames_written == 0) {
		if (csv)
			printf("0,0,0,");
		else
			printf("%s: no data\n", label);
		return;
	}

	for (i = 0; i < res->frames_written; i++) {
		size_t val;

		switch (stat) {
		case COMP_OPEN:
			val = res->completion[i].open;
			val -= res->completion[i].start;
			break;
		case COMP_IO:
			val = res->completion[i].io;
			val -= res->completion[i].open;
			break;
		case COMP_CLOSE:
			val = res->completion[i].close;
			val -= res->completion[i].io;
			break;
		default:
		case COMP_FRAME:
			val = res->completion[i].frame;
			val -= res->completion[i].start;
			break;
		}

		if (val < min)
			min = val;
		if (val > max)
			max = val;
		total += val;
	}
	if (csv) {
		printf("%" PRIu64 ",", min);
		printf("%.9lf,", (double)total / res->frames_written);
		printf("%" PRIu64 ",", max);
	} else {
		printf("%s:\n", label);
		printf(" min   : %.9lf ms\n", (double)min / SEC_IN_MS);
		printf(" avg   : %.9lf ms\n",
		       (double)total / res->frames_written / SEC_IN_MS);
		printf(" max   : %.9lf ms\n", (double)max / SEC_IN_MS);
	}
}

static void print_frames_stat(const test_result_t *res, const opts_t *opts)
{
	if (!res->completion) {
		if (opts->csv)
			printf(",,,");
		return;
	}

	if (opts->csv) {
		print_stat_about(res, "", COMP_FRAME, 1);
		if (opts->times) {
			print_stat_about(res, "", COMP_OPEN, 1);
			print_stat_about(res, "", COMP_IO, 1);
			print_stat_about(res, "", COMP_CLOSE, 1);
		}
	} else {
		print_stat_about(res, "Completion times", COMP_FRAME, 0);
		if (opts->times) {
			print_stat_about(res, "Open times", COMP_OPEN, 0);
			print_stat_about(res, "I/O times", COMP_IO, 0);
			print_stat_about(res, "Close times", COMP_CLOSE, 0);
		}
	}
}

static void print_frame_times(const test_result_t *res, const opts_t *opts)
{
	if (!opts->frametimes)
		return;
	size_t i;

	printf("frame,start,open,io,close,frame\n");
	for (i = 0; i < res->frames_written; i++) {
		printf("%zu,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
		       ",%" PRIu64 "\n",
		       i, res->completion[i].start, res->completion[i].open,
		       res->completion[i].io, res->completion[i].close,
		       res->completion[i].frame);
	}
}

/* Phase 2: Error reporting functions */
static void print_error_stats(const test_result_t *res)
{
	if (!res || res->error_count == 0)
		return;

	printf("\n--- Error Statistics ---\n");
	printf("Total errors: %d\n", res->error_count);

	/* Count errors by operation type */
	int open_errors = 0, read_errors = 0, write_errors = 0, close_errors = 0;
	for (int i = 0; i < res->error_count; i++) {
		if (res->errors[i].operation) {
			if (res->errors[i].operation[0] == 'o') open_errors++;
			else if (res->errors[i].operation[0] == 'r') read_errors++;
			else if (res->errors[i].operation[0] == 'w') write_errors++;
			else if (res->errors[i].operation[0] == 'c') close_errors++;
		}
	}

	if (open_errors > 0) printf("Open errors: %d\n", open_errors);
	if (read_errors > 0) printf("Read errors: %d\n", read_errors);
	if (write_errors > 0) printf("Write errors: %d\n", write_errors);
	if (close_errors > 0) printf("Close errors: %d\n", close_errors);

	printf("Frames affected: %d\n", res->frames_failed);
}

static void print_errors_csv(const test_result_t *res)
{
	if (!res || res->error_count == 0)
		return;

	printf("\nerror_frame,error_operation,error_errno,error_message\n");
	for (int i = 0; i < res->error_count; i++) {
		printf("%d,\"%s\",%d,\"%s\"\n",
		       res->errors[i].frame_number,
		       res->errors[i].operation,
		       res->errors[i].errno_value,
		       res->errors[i].error_message);
	}
}

static void print_errors_json(const test_result_t *res)
{
	if (!res || res->error_count == 0) {
		printf("      \"errors\": null\n");
		return;
	}

	printf("      \"errors\": {\n");
	printf("        \"total_count\": %d,\n", res->error_count);
	printf("        \"frames_affected\": %d,\n", res->frames_failed);
	printf("        \"error_list\": [\n");

	for (int i = 0; i < res->error_count; i++) {
		printf("          {\n");
		printf("            \"frame\": %d,\n", res->errors[i].frame_number);
		printf("            \"operation\": \"%s\",\n", res->errors[i].operation);
		printf("            \"errno\": %d,\n", res->errors[i].errno_value);
		printf("            \"message\": \"%s\",\n", res->errors[i].error_message);
		printf("            \"timestamp_ns\": %" PRIu64 "\n", res->errors[i].timestamp);
		if (i < res->error_count - 1)
			printf("          },\n");
		else
			printf("          }\n");
	}

	printf("        ]\n");
	printf("      }\n");
}

void print_results(const char *tcase, const opts_t *opts,
		   const test_result_t *res)
{
	if (!res)
		return;
	if (!res->time_taken_ns)
		return;

	printf("Results %s:\n", tcase);
	printf(" frames: %" PRIu64 "\n", res->frames_written);
	printf(" bytes : %" PRIu64 "\n", res->bytes_written);
	printf(" time  : %" PRIu64 "\n", res->time_taken_ns);
	printf(" fps   : %lf\n",
	       (double)res->frames_written * SEC_IN_NS / res->time_taken_ns);
	printf(" B/s   : %lf\n",
	       (double)res->bytes_written * SEC_IN_NS / res->time_taken_ns);
	printf(" MiB/s : %lf\n", (double)res->bytes_written * SEC_IN_NS /
					 (1024 * 1024) / res->time_taken_ns);
	print_frames_stat(res, opts);
	print_frame_times(res, opts);
	/* Phase 2: Print error statistics */
	print_error_stats(res);
}

void print_header_csv(const opts_t *opts)
{
	const char *extra = "";

	if (opts->times)
		extra = ",omin,oavg,omax,iomin,ioavg,iomax,cmin,cavg,cmax";

	/* Phase 2: Add filesystem, success rate, and I/O stats columns */
	/* Phase 3: Add performance metrics and trend analysis */
	printf("case,profile,threads,frames,bytes,time,fps,bps,mibps,"
	       "fmin,favg,fmax%s,"
	       "filesystem,success_rate,frames_failed,frames_succeeded,"
	       "direct_io_frames,buffered_io_frames,fallback_count,direct_io_rate,"
	       "is_remote,min_frame_time,avg_frame_time,max_frame_time,performance_trend,network_timeout\n",
	       extra);
}

void print_results_csv(const char *tcase, const opts_t *opts,
		       const test_result_t *res)
{
	if (!res)
		return;
	if (!res->time_taken_ns)
		return;

	printf("\"%s\",", tcase);
	printf("\"%s\",", opts->profile.name);
	printf("%zu,", opts->threads);
	printf("%" PRIu64 ",", res->frames_written);
	printf("%" PRIu64 ",", res->bytes_written);
	printf("%" PRIu64 ",", res->time_taken_ns);
	printf("%.9lf,",
	       (double)res->frames_written * SEC_IN_NS / res->time_taken_ns);
	printf("%.9lf,",
	       (double)res->bytes_written * SEC_IN_NS / res->time_taken_ns);
	printf("%.9lf,", (double)res->bytes_written * SEC_IN_NS / (1024 * 1024) /
			       res->time_taken_ns);
	print_frames_stat(res, opts);
	/* Phase 2: Add filesystem, success rate, and I/O stats */
	/* Phase 3: Add performance metrics and trend analysis */
	printf("%s,%.2f,%d,%d,%d,%d,%d,%.2f,"
	       "%d,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.2f,%" PRIu64 "\n",
	       get_filesystem_name(res->filesystem_type),
	       res->success_rate_percent,
	       res->frames_failed,
	       res->frames_succeeded,
	       res->frames_direct_io,
	       res->frames_buffered_io,
	       res->fallback_count,
	       res->direct_io_success_rate,
	       res->is_remote_filesystem,
	       res->min_frame_time_ns,
	       res->avg_frame_time_ns,
	       res->max_frame_time_ns,
	       res->performance_trend,
	       res->network_timeout_ns);
	print_frame_times(res, opts);
	/* Phase 2: Print error data in CSV format */
	print_errors_csv(res);
}

void print_header_json(void)
{
	printf("{\n  \"results\": [\n");
}

void print_footer_json(void)
{
	printf("  ]\n}\n");
}

void print_results_json(const char *tcase, const opts_t *opts,
		       const test_result_t *res)
{
	uint64_t min = SIZE_MAX;
	uint64_t max = 0;
	uint64_t total = 0;
	size_t i;

	if (!res)
		return;
	if (!res->time_taken_ns)
		return;

	printf("    {\n");
	printf("      \"case\": \"%s\",\n", tcase);
	printf("      \"profile\": \"%s\",\n", opts->profile.name);
	printf("      \"threads\": %zu,\n", opts->threads);
	printf("      \"frames\": %" PRIu64 ",\n", res->frames_written);
	printf("      \"bytes\": %" PRIu64 ",\n", res->bytes_written);
	printf("      \"time_ns\": %" PRIu64 ",\n", res->time_taken_ns);
	printf("      \"fps\": %.9lf,\n",
	       (double)res->frames_written * SEC_IN_NS / res->time_taken_ns);
	printf("      \"bps\": %.9lf,\n",
	       (double)res->bytes_written * SEC_IN_NS / res->time_taken_ns);
	printf("      \"mibps\": %.9lf,\n",
	       (double)res->bytes_written * SEC_IN_NS / (1024 * 1024) /
	       res->time_taken_ns);

	/* Completion times */
	if (res->completion) {
		for (i = 0; i < res->frames_written; i++) {
			uint64_t val = res->completion[i].frame -
				       res->completion[i].start;
			if (val < min)
				min = val;
			if (val > max)
				max = val;
			total += val;
		}
		printf("      \"completion\": {\n");
		printf("        \"min_ms\": %.9lf,\n", (double)min / SEC_IN_MS);
		if (res->frames_written > 0) {
			printf("        \"avg_ms\": %.9lf,\n",
			       (double)total / res->frames_written / SEC_IN_MS);
		} else {
			printf("        \"avg_ms\": 0,\n");
		}
		printf("        \"max_ms\": %.9lf\n", (double)max / SEC_IN_MS);
		printf("      }\n");
	} else {
		printf("      \"completion\": null,\n");
	}

	/* Phase 2: Add filesystem, success rate, and I/O stats to JSON */
	printf("      \"filesystem\": \"%s\",\n", get_filesystem_name(res->filesystem_type));
	printf("      \"success_metrics\": {\n");
	printf("        \"success_rate_percent\": %.2f,\n", res->success_rate_percent);
	printf("        \"frames_failed\": %d,\n", res->frames_failed);
	printf("        \"frames_succeeded\": %d\n", res->frames_succeeded);
	printf("      },\n");
	printf("      \"io_fallback_stats\": {\n");
	printf("        \"direct_io_frames\": %d,\n", res->frames_direct_io);
	printf("        \"buffered_io_frames\": %d,\n", res->frames_buffered_io);
	printf("        \"fallback_events\": %d,\n", res->fallback_count);
	printf("        \"direct_io_success_rate\": %.2f\n", res->direct_io_success_rate);
	printf("      },\n");

	/* Phase 3: Add NFS/SMB optimization metrics to JSON */
	printf("      \"optimization_metrics\": {\n");
	printf("        \"is_remote_filesystem\": %d,\n", res->is_remote_filesystem);
	printf("        \"min_frame_time_ns\": %" PRIu64 ",\n", res->min_frame_time_ns);
	printf("        \"avg_frame_time_ns\": %" PRIu64 ",\n", res->avg_frame_time_ns);
	printf("        \"max_frame_time_ns\": %" PRIu64 ",\n", res->max_frame_time_ns);
	printf("        \"performance_trend\": %.2f,\n", res->performance_trend);
	printf("        \"network_timeout_ns\": %" PRIu64 "\n", res->network_timeout_ns);
	printf("      },\n");

	/* Phase 2: Add error data in JSON format */
	print_errors_json(res);

	printf("    }\n");
}
