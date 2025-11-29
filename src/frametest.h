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

#ifndef FRAMETEST_FRAMETEST_H
#define FRAMETEST_FRAMETEST_H

#include "profile.h"
#include "frame.h"

#define SEC_IN_NS 1000000000UL
#define SEC_IN_MS (SEC_IN_NS / 1000UL)

enum TestMode {
	TEST_WRITE = 1 << 0,
	TEST_READ = 1 << 1,
	TEST_EMPTY = 1 << 2,
};

typedef struct opts_t {
	enum TestMode mode;

	enum ProfileType prof;
	enum ProfileType stream_prof;
	size_t write_size;
	size_t frame_size;
	profile_t profile;

	frame_t *frm;
	const char *path;

	size_t threads;
	size_t frames;
	size_t fps;
	size_t header_size;

	unsigned int reverse : 1;
	unsigned int random : 1;
	unsigned int csv : 1;
	unsigned int no_csv_header : 1;
	unsigned int json : 1;
	unsigned int times : 1;
	unsigned int frametimes : 1;
	unsigned int histogram : 1;
	unsigned int single_file : 1;
	unsigned int tui : 1;
} opts_t;

/* I/O mode enumeration */
typedef enum io_mode_t {
	IO_MODE_UNKNOWN = 0,
	IO_MODE_DIRECT = 1,
	IO_MODE_BUFFERED = 2,
} io_mode_t;

typedef struct test_completion_t {
	uint64_t start;
	uint64_t open;
	uint64_t io;
	uint64_t close;
	uint64_t frame;
	io_mode_t io_mode;              /* Which I/O mode was used for this frame */
} test_completion_t;

/* Filesystem type enumeration */
typedef enum filesystem_type_t {
	FILESYSTEM_LOCAL = 0,
	FILESYSTEM_SMB = 1,
	FILESYSTEM_NFS = 2,
	FILESYSTEM_OTHER = 3,
} filesystem_type_t;

/* Error tracking structure */
typedef struct error_info_t {
	int errno_value;                /* System errno when error occurred */
	char error_message[256];        /* Human-readable error message */
	const char *operation;          /* Which operation failed (open/read/write/close) */
	int frame_number;               /* Which frame failed */
	int thread_id;                  /* Which thread encountered error */
	uint64_t timestamp;             /* When error occurred (nanoseconds) */
} error_info_t;

typedef struct test_result_t {
	uint64_t frames_written;
	uint64_t bytes_written;
	uint64_t time_taken_ns;
	test_completion_t *completion;

	/* Error tracking (Phase 1) */
	int frames_failed;              /* Count of failed frames */
	int frames_succeeded;           /* Count of successful frames */
	float success_rate_percent;     /* (succeeded/total)*100 */
	error_info_t *errors;           /* Array of errors */
	int error_count;                /* Number of errors recorded */
	int max_errors;                 /* Allocated error array size */

	/* Filesystem info (Phase 1) */
	int direct_io_available;        /* Was direct I/O actually used? (1=yes, 0=no) */
	filesystem_type_t filesystem_type; /* Type of filesystem being tested */

	/* I/O fallback tracking (Phase 2) */
	int frames_direct_io;           /* Frames completed with Direct I/O */
	int frames_buffered_io;         /* Frames completed with Buffered I/O (fallback) */
	int fallback_count;             /* Number of times fallback was triggered */
	float direct_io_success_rate;   /* Percentage of frames that used Direct I/O */

	/* NFS/SMB optimization (Phase 3) */
	int is_remote_filesystem;       /* 1 if NFS/SMB, 0 if local */
	int skipped_direct_io_attempt;  /* Skip Direct I/O on remote filesystems */
	uint64_t min_frame_time_ns;     /* Minimum frame completion time */
	uint64_t max_frame_time_ns;     /* Maximum frame completion time */
	uint64_t avg_frame_time_ns;     /* Average frame completion time */
	float performance_trend;        /* Performance trend over frames (-1=degrading, 0=stable, 1=improving) */
	uint64_t network_timeout_ns;    /* Timeout for network operations in nanoseconds */
} test_result_t;

#endif
