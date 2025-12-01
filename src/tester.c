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

#ifdef __linux__
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#endif
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "tester.h"
#include "timing.h"
#include "platform.h"

/* Phase 1: Error recording helper function */
static inline void record_error(test_result_t *result, int errno_val,
				const char *operation, int frame_num,
				int thread_id)
{
	/* Expand error array if needed */
	if (result->error_count >= result->max_errors) {
		int new_size =
			(result->max_errors == 0) ? 10 : result->max_errors * 2;
		error_info_t *new_errors = realloc(
			result->errors, new_size * sizeof(error_info_t));
		if (!new_errors)
			return; /* Can't allocate more memory */
		result->errors = new_errors;
		result->max_errors = new_size;
	}

	/* Record error details */
	error_info_t *err = &result->errors[result->error_count++];
	err->errno_value = errno_val;
	err->operation = operation;
	err->frame_number = frame_num;
	err->thread_id = thread_id;
	err->timestamp = timing_time();
	snprintf(err->error_message, sizeof(err->error_message), "%s: %s",
		 operation, platform_strerror(errno_val));
}

static inline size_t tester_frame_write(const platform_t *platform,
					const char *path, frame_t *frame,
					size_t num, test_files_t files,
					test_completion_t *comp,
					int is_remote_fs)
{
	char name[PATH_MAX + 1];
	size_t ret;
	platform_handle_t f;
	io_mode_t io_mode = IO_MODE_UNKNOWN;

	switch (files) {
	case TEST_FILES_MULTIPLE:
		snprintf(name, PATH_MAX, "%s/frame%.6zu.tst", path, num);
		name[PATH_MAX] = 0;
		break;
	case TEST_FILES_SINGLE:
		snprintf(name, PATH_MAX, "%s", path);
		name[PATH_MAX] = 0;
		break;
	default:
		return 1;
	}

	/* Phase 3: Skip Direct I/O on remote filesystems */
	if (is_remote_fs) {
		/* Use buffered I/O directly for remote filesystems */
		f = platform->open(
			name, PLATFORM_OPEN_CREATE | PLATFORM_OPEN_WRITE, 0666);
		if (f > 0) {
			io_mode = IO_MODE_BUFFERED;
		}
	} else {
		/* Phase 2: Try Direct I/O first, fall back to buffered if needed */
		f = platform->open(name,
				   PLATFORM_OPEN_CREATE | PLATFORM_OPEN_WRITE |
					   PLATFORM_OPEN_DIRECT,
				   0666);

		if (f > 0) {
			io_mode = IO_MODE_DIRECT;
		} else {
			/* Fallback: Retry without Direct I/O flag */
			f = platform->open(name,
					   PLATFORM_OPEN_CREATE |
						   PLATFORM_OPEN_WRITE,
					   0666);
			if (f > 0) {
				io_mode = IO_MODE_BUFFERED;
			}
		}
	}

	if (f <= 0)
		return 1;

	if (files == TEST_FILES_SINGLE) {
		long pos =
			platform->seek(f, num * frame->size, PLATFORM_SEEK_SET);
		if (pos < 0) {
			platform->close(f);
			return 1;
		}
	}

	comp->open = timing_start();

	ret = frame_write(platform, f, frame);
	comp->io = timing_start();

	platform->close(f);
	comp->close = timing_start();

	/* Track which I/O mode was used */
	comp->io_mode = io_mode;

	/* Faking the output! */
	if (!ret && !frame->size)
		return 1;
	return ret;
}

static inline size_t tester_frame_read(const platform_t *platform,
				       const char *path, frame_t *frame,
				       size_t num, test_files_t files,
				       test_completion_t *comp,
				       int is_remote_fs)
{
	char name[PATH_MAX + 1];
	size_t ret;
	platform_handle_t f;
	io_mode_t io_mode = IO_MODE_UNKNOWN;

	switch (files) {
	case TEST_FILES_MULTIPLE:
		snprintf(name, PATH_MAX, "%s/frame%.6zu.tst", path, num);
		name[PATH_MAX] = 0;
		break;
	case TEST_FILES_SINGLE:
		snprintf(name, PATH_MAX, "%s", path);
		name[PATH_MAX] = 0;
		break;
	default:
		return 1;
	}

	/* Phase 3: Skip Direct I/O on remote filesystems */
	if (is_remote_fs) {
		/* Use buffered I/O directly for remote filesystems */
		f = platform->open(name, PLATFORM_OPEN_READ, 0666);
		if (f > 0) {
			io_mode = IO_MODE_BUFFERED;
		}
	} else {
		/* Phase 2: Try Direct I/O first, fall back to buffered if needed */
		f = platform->open(
			name, PLATFORM_OPEN_READ | PLATFORM_OPEN_DIRECT, 0666);
		if (f > 0) {
			io_mode = IO_MODE_DIRECT;
		} else {
			/* Fallback: Retry without Direct I/O flag */
			f = platform->open(name, PLATFORM_OPEN_READ, 0666);
			if (f > 0) {
				io_mode = IO_MODE_BUFFERED;
			}
		}
	}

	if (f <= 0)
		return 0;

	if (files == TEST_FILES_SINGLE) {
		long pos =
			platform->seek(f, num * frame->size, PLATFORM_SEEK_SET);
		if (pos < 0) {
			platform->close(f);
			return 1;
		}
	}

	comp->open = timing_start();

	ret = frame_read(platform, f, frame);
	comp->io = timing_start();

	platform->close(f);
	comp->close = timing_start();

	/* Track which I/O mode was used */
	comp->io_mode = io_mode;

	/* Faking the output! */
	if (!ret && !frame->size)
		return 1;
	return ret;
}

frame_t *tester_get_frame_read(const platform_t *platform, const char *path,
			       size_t frame_size)
{
	char name[PATH_MAX + 1];

	snprintf(name, PATH_MAX, "%s/frame%.6lu.tst", path, 0UL);
	name[PATH_MAX] = 0;

	return frame_from_file(platform, name, frame_size);
}

static inline void shuffle_array(size_t *arr, size_t size)
{
	size_t i;

	if (!arr || size <= 1)
		return;

	for (i = size - 1; i >= 1; i--) {
		size_t j = rand() % size;
		size_t tmp;

		tmp = arr[j];
		arr[j] = arr[i];
		arr[i] = tmp;
	}
}

test_result_t tester_run_write(const platform_t *platform, const char *path,
			       frame_t *frame, size_t start_frame,
			       size_t frames, size_t fps, test_mode_t mode,
			       test_files_t files)
{
	test_result_t res = { 0 };
	size_t i;
	size_t budget;
	size_t end_frame;
	size_t *seq = NULL;

	res.completion = platform->calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	/* Phase 1: Initialize error tracking */
	res.frames_failed = 0;
	res.frames_succeeded = 0;
	res.error_count = 0;
	res.max_errors = 0;
	res.errors = NULL;
	res.direct_io_available = 1;

	/* Phase 2: Initialize I/O fallback tracking */
	res.frames_direct_io = 0;
	res.frames_buffered_io = 0;
	res.fallback_count = 0;
	res.direct_io_success_rate = 0.0;

	/* Phase 3: Initialize NFS/SMB optimization and performance tracking */
	res.filesystem_type = platform_detect_filesystem(path);
	res.is_remote_filesystem = (res.filesystem_type == FILESYSTEM_NFS ||
				    res.filesystem_type == FILESYSTEM_SMB) ?
					   1 :
					   0;
	res.skipped_direct_io_attempt = 0;
	res.min_frame_time_ns = UINT64_MAX;
	res.max_frame_time_ns = 0;
	res.avg_frame_time_ns = 0;
	res.performance_trend = 0.0;
	res.network_timeout_ns =
		platform_get_network_timeout(res.filesystem_type);

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	if (mode == TEST_MODE_RANDOM) {
		seq = platform->malloc(sizeof(*seq) * frames);
		if (!seq) {
			platform->free(res.completion);
			res.completion = NULL;
			return res;
		}

		for (i = 0; i < frames; i++)
			seq[i] = start_frame + i;
		shuffle_array(seq, frames);
	}

	for (i = start_frame; i < end_frame; i++) {
		uint64_t frame_start = timing_start();
		size_t frame_idx;

		res.completion[i - start_frame].start = frame_start;
		switch (mode) {
		case TEST_MODE_REVERSE:
			frame_idx = end_frame - i + start_frame - 1;
			break;
		case TEST_MODE_RANDOM:
			frame_idx = seq[i - start_frame];
			break;
		case TEST_MODE_NORM:
		default:
			frame_idx = i;
			break;
		}
		if (!tester_frame_write(platform, path, frame, frame_idx, files,
					&res.completion[i - start_frame],
					res.is_remote_filesystem)) {
			/* Phase 1: Record error and continue tracking */
			res.frames_failed++;
			record_error(&res, errno, "write", frame_idx, 0);
			/* Phase 2: Continue instead of break to allow test to continue */
			continue;
		}
		res.completion[i - start_frame].frame = timing_start();
		++res.frames_written;
		res.frames_succeeded++;
		res.bytes_written += frame->size;

		/* Phase 2: Track which I/O mode was used */
		if (res.completion[i - start_frame].io_mode == IO_MODE_DIRECT) {
			res.frames_direct_io++;
		} else if (res.completion[i - start_frame].io_mode ==
			   IO_MODE_BUFFERED) {
			res.frames_buffered_io++;
			res.fallback_count++;
		}

		/* Phase 3: Track performance metrics */
		uint64_t frame_time = res.completion[i - start_frame].frame;
		if (frame_time > 0) {
			if (frame_time < res.min_frame_time_ns) {
				res.min_frame_time_ns = frame_time;
			}
			if (frame_time > res.max_frame_time_ns) {
				res.max_frame_time_ns = frame_time;
			}
		}

		/* If fps limit is enabled loop until frame budget is gone */
		if (fps && budget) {
			uint64_t frame_elapsed = timing_elapsed(frame_start);

			while (frame_elapsed < budget) {
				platform->usleep(100);
				frame_elapsed = timing_elapsed(frame_start);
			}
		}
	}

	/* Phase 1: Calculate success rate */
	if (res.frames_succeeded + res.frames_failed > 0) {
		res.success_rate_percent =
			(res.frames_succeeded * 100.0) /
			(res.frames_succeeded + res.frames_failed);
	}

	/* Phase 2: Calculate Direct I/O success rate */
	if (res.frames_direct_io + res.frames_buffered_io > 0) {
		res.direct_io_success_rate =
			(res.frames_direct_io * 100.0) /
			(res.frames_direct_io + res.frames_buffered_io);
	}

	/* Phase 3: Calculate performance trend and average frame time */
	if (res.frames_succeeded > 0 && res.min_frame_time_ns != UINT64_MAX) {
		/* Calculate average frame time */
		uint64_t total_frame_time = 0;
		for (size_t j = 0; j < frames; j++) {
			if (res.completion[j].frame > 0) {
				total_frame_time += res.completion[j].frame;
			}
		}
		res.avg_frame_time_ns =
			(res.frames_succeeded > 0) ?
				(total_frame_time / res.frames_succeeded) :
				0;

		/* Calculate performance trend using first half vs second half comparison */
		if (res.frames_succeeded > 10) {
			uint64_t first_half_time = 0, second_half_time = 0;
			int first_half_count = 0, second_half_count = 0;
			int mid = res.frames_succeeded / 2;

			for (size_t j = 0; j < frames; j++) {
				if (res.completion[j].frame > 0) {
					if (first_half_count < mid) {
						first_half_time +=
							res.completion[j].frame;
						first_half_count++;
					} else {
						second_half_time +=
							res.completion[j].frame;
						second_half_count++;
					}
				}
			}

			if (first_half_count > 0 && second_half_count > 0) {
				uint64_t first_avg =
					first_half_time / first_half_count;
				uint64_t second_avg =
					second_half_time / second_half_count;

				if (second_avg < first_avg) {
					res.performance_trend =
						1.0; /* Improving */
				} else if (second_avg > first_avg) {
					res.performance_trend =
						-1.0; /* Degrading */
				} else {
					res.performance_trend =
						0.0; /* Stable */
				}
			}
		}
	}

	if (seq)
		platform->free(seq);
	return res;
}

test_result_t tester_run_read(const platform_t *platform, const char *path,
			      frame_t *frame, size_t start_frame, size_t frames,
			      size_t fps, test_mode_t mode, test_files_t files)
{
	test_result_t res = { 0 };
	size_t i;
	size_t budget;
	size_t end_frame;
	size_t *seq = NULL;

	res.completion = platform->calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	/* Phase 1: Initialize error tracking */
	res.frames_failed = 0;
	res.frames_succeeded = 0;
	res.error_count = 0;
	res.max_errors = 0;
	res.errors = NULL;
	res.direct_io_available = 1;

	/* Phase 2: Initialize I/O fallback tracking */
	res.frames_direct_io = 0;
	res.frames_buffered_io = 0;
	res.fallback_count = 0;
	res.direct_io_success_rate = 0.0;

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	if (mode == TEST_MODE_RANDOM) {
		seq = platform->malloc(sizeof(*seq) * frames);
		if (!seq) {
			platform->free(res.completion);
			res.completion = NULL;
			return res;
		}

		for (i = 0; i < frames; i++)
			seq[i] = i + start_frame;
		shuffle_array(seq, frames);
	}

	for (i = start_frame; i < start_frame + frames; i++) {
		uint64_t frame_start = timing_start();
		size_t frame_idx;

		res.completion[i - start_frame].start = frame_start;
		switch (mode) {
		case TEST_MODE_REVERSE:
			frame_idx = end_frame - i + start_frame - 1;
			break;
		case TEST_MODE_RANDOM:
			frame_idx = seq[i - start_frame];
			break;
		case TEST_MODE_NORM:
		default:
			frame_idx = i;
			break;
		}
		if (!tester_frame_read(platform, path, frame, frame_idx, files,
				       &res.completion[i - start_frame],
				       res.is_remote_filesystem)) {
			/* Phase 1: Record error and continue tracking */
			res.frames_failed++;
			record_error(&res, errno, "read", frame_idx, 0);
			/* Phase 2: Continue instead of break to allow test to continue */
			continue;
		}
		res.completion[i - start_frame].frame = timing_start();
		++res.frames_written;
		res.frames_succeeded++;
		res.bytes_written += frame->size;

		/* Phase 2: Track which I/O mode was used */
		if (res.completion[i - start_frame].io_mode == IO_MODE_DIRECT) {
			res.frames_direct_io++;
		} else if (res.completion[i - start_frame].io_mode ==
			   IO_MODE_BUFFERED) {
			res.frames_buffered_io++;
			res.fallback_count++;
		}

		/* Phase 3: Track performance metrics */
		uint64_t frame_time = res.completion[i - start_frame].frame;
		if (frame_time > 0) {
			if (frame_time < res.min_frame_time_ns) {
				res.min_frame_time_ns = frame_time;
			}
			if (frame_time > res.max_frame_time_ns) {
				res.max_frame_time_ns = frame_time;
			}
		}

		/* If fps limit is enabled loop until frame budget is gone */
		if (fps && budget) {
			uint64_t frame_elapsed = timing_elapsed(frame_start);

			while (frame_elapsed < budget) {
				platform->usleep(100);
				frame_elapsed = timing_elapsed(frame_start);
			}
		}
	}

	/* Phase 1: Calculate success rate */
	if (res.frames_succeeded + res.frames_failed > 0) {
		res.success_rate_percent =
			(res.frames_succeeded * 100.0) /
			(res.frames_succeeded + res.frames_failed);
	}

	/* Phase 2: Calculate Direct I/O success rate */
	if (res.frames_direct_io + res.frames_buffered_io > 0) {
		res.direct_io_success_rate =
			(res.frames_direct_io * 100.0) /
			(res.frames_direct_io + res.frames_buffered_io);
	}

	/* Phase 3: Calculate performance trend and average frame time */
	if (res.frames_succeeded > 0 && res.min_frame_time_ns != UINT64_MAX) {
		/* Calculate average frame time */
		uint64_t total_frame_time = 0;
		for (size_t j = 0; j < frames; j++) {
			if (res.completion[j].frame > 0) {
				total_frame_time += res.completion[j].frame;
			}
		}
		res.avg_frame_time_ns =
			(res.frames_succeeded > 0) ?
				(total_frame_time / res.frames_succeeded) :
				0;

		/* Calculate performance trend using first half vs second half comparison */
		if (res.frames_succeeded > 10) {
			uint64_t first_half_time = 0, second_half_time = 0;
			int first_half_count = 0, second_half_count = 0;
			int mid = res.frames_succeeded / 2;

			for (size_t j = 0; j < frames; j++) {
				if (res.completion[j].frame > 0) {
					if (first_half_count < mid) {
						first_half_time +=
							res.completion[j].frame;
						first_half_count++;
					} else {
						second_half_time +=
							res.completion[j].frame;
						second_half_count++;
					}
				}
			}

			if (first_half_count > 0 && second_half_count > 0) {
				uint64_t first_avg =
					first_half_time / first_half_count;
				uint64_t second_avg =
					second_half_time / second_half_count;

				if (second_avg < first_avg) {
					res.performance_trend =
						1.0; /* Improving */
				} else if (second_avg > first_avg) {
					res.performance_trend =
						-1.0; /* Degrading */
				} else {
					res.performance_trend =
						0.0; /* Stable */
				}
			}
		}
	}

	if (seq)
		platform->free(seq);
	return res;
}

/* Callback-enabled versions for TUI progress updates */

test_result_t tester_run_write_cb(const platform_t *platform, const char *path,
				  frame_t *frame, size_t start_frame,
				  size_t frames, size_t fps, test_mode_t mode,
				  test_files_t files, tester_progress_cb cb,
				  void *cb_ctx)
{
	test_result_t res = { 0 };
	size_t i;
	size_t budget;
	size_t end_frame;
	size_t *seq = NULL;

	res.completion = platform->calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	res.frames_failed = 0;
	res.frames_succeeded = 0;
	res.error_count = 0;
	res.max_errors = 0;
	res.errors = NULL;
	res.direct_io_available = 1;
	res.frames_direct_io = 0;
	res.frames_buffered_io = 0;
	res.fallback_count = 0;
	res.direct_io_success_rate = 0.0;
	res.filesystem_type = platform_detect_filesystem(path);
	res.is_remote_filesystem = (res.filesystem_type == FILESYSTEM_NFS ||
				    res.filesystem_type == FILESYSTEM_SMB) ?
					   1 :
					   0;
	res.skipped_direct_io_attempt = 0;
	res.min_frame_time_ns = UINT64_MAX;
	res.max_frame_time_ns = 0;
	res.avg_frame_time_ns = 0;
	res.performance_trend = 0.0;
	res.network_timeout_ns =
		platform_get_network_timeout(res.filesystem_type);

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	if (mode == TEST_MODE_RANDOM) {
		seq = platform->malloc(sizeof(*seq) * frames);
		if (!seq) {
			platform->free(res.completion);
			res.completion = NULL;
			return res;
		}
		for (i = 0; i < frames; i++)
			seq[i] = start_frame + i;
		shuffle_array(seq, frames);
	}

	for (i = start_frame; i < end_frame; i++) {
		uint64_t frame_start = timing_start();
		size_t frame_idx;
		int success = 0;
		uint64_t frame_time_ns = 0;
		io_mode_t io_mode = IO_MODE_UNKNOWN;

		res.completion[i - start_frame].start = frame_start;
		switch (mode) {
		case TEST_MODE_REVERSE:
			frame_idx = end_frame - i + start_frame - 1;
			break;
		case TEST_MODE_RANDOM:
			frame_idx = seq[i - start_frame];
			break;
		case TEST_MODE_NORM:
		default:
			frame_idx = i;
			break;
		}

		if (!tester_frame_write(platform, path, frame, frame_idx, files,
					&res.completion[i - start_frame],
					res.is_remote_filesystem)) {
			res.frames_failed++;
			record_error(&res, errno, "write", frame_idx, 0);
			if (cb)
				cb(cb_ctx,
				   res.frames_written + res.frames_failed, 0, 0,
				   IO_MODE_UNKNOWN, 0);
			continue;
		}

		res.completion[i - start_frame].frame = timing_start();
		++res.frames_written;
		res.frames_succeeded++;
		res.bytes_written += frame->size;
		success = 1;
		io_mode = res.completion[i - start_frame].io_mode;

		if (io_mode == IO_MODE_DIRECT)
			res.frames_direct_io++;
		else if (io_mode == IO_MODE_BUFFERED) {
			res.frames_buffered_io++;
			res.fallback_count++;
		}

		frame_time_ns = timing_elapsed(frame_start);
		if (frame_time_ns > 0) {
			if (frame_time_ns < res.min_frame_time_ns)
				res.min_frame_time_ns = frame_time_ns;
			if (frame_time_ns > res.max_frame_time_ns)
				res.max_frame_time_ns = frame_time_ns;
		}

		if (cb)
			cb(cb_ctx, res.frames_written + res.frames_failed,
			   frame->size, frame_time_ns, io_mode, success);

		if (fps && budget) {
			uint64_t frame_elapsed = timing_elapsed(frame_start);
			while (frame_elapsed < budget) {
				platform->usleep(100);
				frame_elapsed = timing_elapsed(frame_start);
			}
		}
	}

	if (res.frames_succeeded + res.frames_failed > 0) {
		res.success_rate_percent =
			(res.frames_succeeded * 100.0) /
			(res.frames_succeeded + res.frames_failed);
	}
	if (res.frames_direct_io + res.frames_buffered_io > 0) {
		res.direct_io_success_rate =
			(res.frames_direct_io * 100.0) /
			(res.frames_direct_io + res.frames_buffered_io);
	}
	if (res.frames_succeeded > 0 && res.min_frame_time_ns != UINT64_MAX) {
		uint64_t total_frame_time = 0;
		for (size_t j = 0; j < frames; j++) {
			if (res.completion[j].frame > 0)
				total_frame_time += res.completion[j].frame;
		}
		res.avg_frame_time_ns =
			(res.frames_succeeded > 0) ?
				(total_frame_time / res.frames_succeeded) :
				0;
	}

	if (seq)
		platform->free(seq);
	return res;
}

test_result_t tester_run_read_cb(const platform_t *platform, const char *path,
				 frame_t *frame, size_t start_frame,
				 size_t frames, size_t fps, test_mode_t mode,
				 test_files_t files, tester_progress_cb cb,
				 void *cb_ctx)
{
	test_result_t res = { 0 };
	size_t i;
	size_t budget;
	size_t end_frame;
	size_t *seq = NULL;

	res.completion = platform->calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	res.frames_failed = 0;
	res.frames_succeeded = 0;
	res.error_count = 0;
	res.max_errors = 0;
	res.errors = NULL;
	res.direct_io_available = 1;
	res.frames_direct_io = 0;
	res.frames_buffered_io = 0;
	res.fallback_count = 0;
	res.direct_io_success_rate = 0.0;

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	if (mode == TEST_MODE_RANDOM) {
		seq = platform->malloc(sizeof(*seq) * frames);
		if (!seq) {
			platform->free(res.completion);
			res.completion = NULL;
			return res;
		}
		for (i = 0; i < frames; i++)
			seq[i] = i + start_frame;
		shuffle_array(seq, frames);
	}

	for (i = start_frame; i < start_frame + frames; i++) {
		uint64_t frame_start = timing_start();
		size_t frame_idx;
		int success = 0;
		uint64_t frame_time_ns = 0;
		io_mode_t io_mode = IO_MODE_UNKNOWN;

		res.completion[i - start_frame].start = frame_start;
		switch (mode) {
		case TEST_MODE_REVERSE:
			frame_idx = end_frame - i + start_frame - 1;
			break;
		case TEST_MODE_RANDOM:
			frame_idx = seq[i - start_frame];
			break;
		case TEST_MODE_NORM:
		default:
			frame_idx = i;
			break;
		}

		if (!tester_frame_read(platform, path, frame, frame_idx, files,
				       &res.completion[i - start_frame],
				       res.is_remote_filesystem)) {
			res.frames_failed++;
			record_error(&res, errno, "read", frame_idx, 0);
			if (cb)
				cb(cb_ctx,
				   res.frames_written + res.frames_failed, 0, 0,
				   IO_MODE_UNKNOWN, 0);
			continue;
		}

		res.completion[i - start_frame].frame = timing_start();
		++res.frames_written;
		res.frames_succeeded++;
		res.bytes_written += frame->size;
		success = 1;
		io_mode = res.completion[i - start_frame].io_mode;

		if (io_mode == IO_MODE_DIRECT)
			res.frames_direct_io++;
		else if (io_mode == IO_MODE_BUFFERED) {
			res.frames_buffered_io++;
			res.fallback_count++;
		}

		frame_time_ns = timing_elapsed(frame_start);
		if (frame_time_ns > 0) {
			if (frame_time_ns < res.min_frame_time_ns)
				res.min_frame_time_ns = frame_time_ns;
			if (frame_time_ns > res.max_frame_time_ns)
				res.max_frame_time_ns = frame_time_ns;
		}

		if (cb)
			cb(cb_ctx, res.frames_written + res.frames_failed,
			   frame->size, frame_time_ns, io_mode, success);

		if (fps && budget) {
			uint64_t frame_elapsed = timing_elapsed(frame_start);
			while (frame_elapsed < budget) {
				platform->usleep(100);
				frame_elapsed = timing_elapsed(frame_start);
			}
		}
	}

	if (res.frames_succeeded + res.frames_failed > 0) {
		res.success_rate_percent =
			(res.frames_succeeded * 100.0) /
			(res.frames_succeeded + res.frames_failed);
	}
	if (res.frames_direct_io + res.frames_buffered_io > 0) {
		res.direct_io_success_rate =
			(res.frames_direct_io * 100.0) /
			(res.frames_direct_io + res.frames_buffered_io);
	}
	if (res.frames_succeeded > 0 && res.min_frame_time_ns != UINT64_MAX) {
		uint64_t total_frame_time = 0;
		for (size_t j = 0; j < frames; j++) {
			if (res.completion[j].frame > 0)
				total_frame_time += res.completion[j].frame;
		}
		res.avg_frame_time_ns =
			(res.frames_succeeded > 0) ?
				(total_frame_time / res.frames_succeeded) :
				0;
	}

	if (seq)
		platform->free(seq);
	return res;
}
