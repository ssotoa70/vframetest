/*
 * vframetest - Test and benchmark media frame write/read
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

// Enable POSIX timers (clock_gettime, struct timespec, CLOCK_MONOTONIC)
// when compiling on Linux with strict C99 (as in CI).
#if defined(__linux__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

/* Windows mkdir doesn't take a mode argument */
#ifdef _WIN32
#define mkdir(path, mode) mkdir(path)
#endif

#include "profile.h"
#include "frame.h"
#include "tester.h"
#include "histogram.h"
#include "frametest.h"
#include "report.h"
#include "platform.h"

#ifndef NO_TUI
#include "tui.h"
#include "tty.h"
#include "tui_state.h"
#include "tui_input.h"
#include "tui_render.h"
#include "screen.h"
#endif

/* Shared progress state for TUI updates (volatile for thread safety) */
typedef struct tui_progress_t {
	volatile size_t frames_completed;
	volatile size_t frames_succeeded;
	volatile size_t frames_failed;
	volatile uint64_t bytes_written;
	volatile uint64_t last_frame_time_ns;
	volatile io_mode_t last_io_mode;
	volatile int running;
} tui_progress_t;

typedef struct thread_info_t {
	size_t id;
	uint64_t thread;

	const platform_t *platform;
	const opts_t *opts;
	test_result_t res;

	size_t start_frame;
	size_t frames;
	size_t fps;

	/* TUI shared progress pointer (NULL if TUI disabled) */
	tui_progress_t *tui_progress;
} thread_info_t;

void *run_write_test_thread(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;
	test_mode_t mode = TEST_MODE_NORM;
	test_files_t files;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	if (info->opts->reverse)
		mode = TEST_MODE_REVERSE;
	else if (info->opts->random)
		mode = TEST_MODE_RANDOM;

	files = info->opts->single_file ? TEST_FILES_SINGLE :
					  TEST_FILES_MULTIPLE;

	info->res = tester_run_write(info->platform, info->opts->path,
				     info->opts->frm, info->start_frame,
				     info->frames, info->fps, mode, files);

	return NULL;
}

void *run_read_test_thread(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;
	test_mode_t mode = TEST_MODE_NORM;
	test_files_t files;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	if (info->opts->reverse)
		mode = TEST_MODE_REVERSE;
	else if (info->opts->random)
		mode = TEST_MODE_RANDOM;

	files = info->opts->single_file ? TEST_FILES_SINGLE :
					  TEST_FILES_MULTIPLE;

	info->res = tester_run_read(info->platform, info->opts->path,
				    info->opts->frm, info->start_frame,
				    info->frames, info->fps, mode, files);

	return NULL;
}

#ifndef NO_TUI
/* Progress callback for TUI - updates shared progress state using atomics */
static void tui_progress_callback(void *ctx, size_t frames_done,
				  size_t bytes_written, uint64_t frame_time_ns,
				  io_mode_t io_mode, int success)
{
	tui_progress_t *progress = (tui_progress_t *)ctx;
	if (!progress)
		return;

	/* Use atomic operations to avoid race conditions with multiple threads */
	__sync_fetch_and_add(&progress->frames_completed, 1);
	__sync_fetch_and_add(&progress->bytes_written, bytes_written);

	/* These don't need to be precise - just last value seen */
	progress->last_frame_time_ns = frame_time_ns;
	progress->last_io_mode = io_mode;

	if (success)
		__sync_fetch_and_add(&progress->frames_succeeded, 1);
	else
		__sync_fetch_and_add(&progress->frames_failed, 1);
}

/* TUI-enabled write test thread with progress callback */
void *run_write_test_thread_tui(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;
	test_mode_t mode = TEST_MODE_NORM;
	test_files_t files;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	if (info->opts->reverse)
		mode = TEST_MODE_REVERSE;
	else if (info->opts->random)
		mode = TEST_MODE_RANDOM;

	files = info->opts->single_file ? TEST_FILES_SINGLE :
					  TEST_FILES_MULTIPLE;

	info->res = tester_run_write_cb(info->platform, info->opts->path,
					info->opts->frm, info->start_frame,
					info->frames, info->fps, mode, files,
					tui_progress_callback,
					info->tui_progress);

	return NULL;
}

/* TUI-enabled read test thread with progress callback */
void *run_read_test_thread_tui(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;
	test_mode_t mode = TEST_MODE_NORM;
	test_files_t files;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	if (info->opts->reverse)
		mode = TEST_MODE_REVERSE;
	else if (info->opts->random)
		mode = TEST_MODE_RANDOM;

	files = info->opts->single_file ? TEST_FILES_SINGLE :
					  TEST_FILES_MULTIPLE;

	info->res = tester_run_read_cb(info->platform, info->opts->path,
				       info->opts->frm, info->start_frame,
				       info->frames, info->fps, mode, files,
				       tui_progress_callback,
				       info->tui_progress);

	return NULL;
}
#endif /* NO_TUI */

void calculate_frame_range(thread_info_t *threads, const opts_t *opts)
{
	size_t i;
	uint64_t start_frame;
	uint64_t frames_per_thread;
	uint64_t frames_left;
	uint64_t fps_per_thread;
	uint64_t fps_left;

	frames_per_thread = opts->frames / opts->threads;
	frames_left = opts->frames % opts->threads;

	fps_per_thread = opts->fps / opts->threads;
	fps_left = opts->fps % opts->threads;

	start_frame = 0;

	for (i = 0; i < opts->threads; i++) {
		threads[i].start_frame = start_frame;
		threads[i].frames = frames_per_thread;
		threads[i].fps = fps_per_thread;
		if (frames_left) {
			++threads[i].frames;
			--frames_left;
		}
		if (fps_left) {
			++threads[i].fps;
			--fps_left;
		}
		start_frame += threads[i].frames;
	}
}

int run_test_threads(const platform_t *platform, const char *tst,
		     const opts_t *opts, void *(*tfunc)(void *))
{
	size_t i;
	int res;
	thread_info_t *threads;
	test_result_t tres = { 0 };
	uint64_t start;

	threads = platform->calloc(opts->threads, sizeof(*threads));
	if (!threads)
		return 1;

	calculate_frame_range(threads, opts);

	/* Phase 1: Detect filesystem type and warn if remote */
	tres.filesystem_type = platform_detect_filesystem(opts->path);
	if (tres.filesystem_type != 0) {
		const char *fs_name = (tres.filesystem_type == 1) ? "SMB" :
				      (tres.filesystem_type == 2) ? "NFS" :
								    "Unknown";
		fprintf(stderr,
			"WARNING: Test path is on a remote filesystem (%s)\n"
			"Direct I/O may not be available. Results may not be accurate.\n",
			fs_name);
	}

	start = timing_start();
	for (i = 0; i < opts->threads; i++) {
		int thread_res;

		threads[i].id = i;
		threads[i].platform = platform;
		threads[i].opts = opts;
		thread_res = platform->thread_create(&threads[i].thread, tfunc,
						     (void *)&threads[i]);
		if (thread_res) {
			size_t j;
			void *ret;

			for (j = 0; j < i; j++)
				platform->thread_cancel(threads[j].thread);
			for (j = 0; j < i; j++)
				platform->thread_join(threads[j].thread, &ret);
			platform->free(threads);
			return 1;
		}
	}

	res = 0;
	for (i = 0; i < opts->threads; i++) {
		void *ret;

		if (platform->thread_join(threads[i].thread, &ret))
			res = 1;
		if (ret)
			res = 1;

#if 0
		print_results(&threads[i].res);
#endif
		if (test_result_aggregate(&tres, &threads[i].res))
			res = 1;
		result_free(platform, &threads[i].res);
	}
	tres.time_taken_ns = timing_elapsed(start);
	if (!res) {
		if (opts->json) {
			if (!opts->no_csv_header)
				print_header_json();
			print_results_json(tst, opts, &tres);
			if (!opts->no_csv_header)
				print_footer_json();
		} else if (opts->csv) {
			print_results_csv(tst, opts, &tres);
		} else {
			print_results(tst, opts, &tres);
			/* Phase 1: Always display error/success summary */
			fprintf(stdout, "Frames failed: %d\n",
				tres.frames_failed);
			fprintf(stdout, "Frames succeeded: %d\n",
				tres.frames_succeeded);
			fprintf(stdout, "Success rate: %.2f%%\n",
				tres.success_rate_percent);
			/* Phase 1: Display filesystem type and warnings */
			const char *fs_types[] = { "LOCAL", "SMB", "NFS",
						   "OTHER" };
			int fs_idx = (tres.filesystem_type >= 0 &&
				      tres.filesystem_type <= 3) ?
					     tres.filesystem_type :
					     3;
			fprintf(stdout, "Filesystem: %s\n", fs_types[fs_idx]);
			if (tres.filesystem_type != 0) {
				fprintf(stdout,
					"WARNING: Test path is on a remote filesystem\n");
				fprintf(stdout,
					"Direct I/O may not be available. Results may not be accurate.\n");
			}

			/* Phase 2: Display I/O fallback statistics */
			if (tres.frames_direct_io > 0 ||
			    tres.frames_buffered_io > 0) {
				fprintf(stdout,
					"\n--- Phase 2: I/O Mode Statistics ---\n");
				fprintf(stdout, "Frames with Direct I/O: %d\n",
					tres.frames_direct_io);
				fprintf(stdout,
					"Frames with Buffered I/O (fallback): %d\n",
					tres.frames_buffered_io);
				fprintf(stdout,
					"Direct I/O success rate: %.2f%%\n",
					tres.direct_io_success_rate);
				if (tres.fallback_count > 0) {
					fprintf(stdout, "Fallback events: %d\n",
						tres.fallback_count);
				}
			}

			if (opts->histogram)
				print_histogram(&tres);
		}
	}
	result_free(platform, &tres);
	platform->free(threads);
	return res;
}

#ifndef NO_TUI
/* TUI-enabled test runner with real-time progress updates */
int run_test_threads_tui(const platform_t *platform, const char *tst,
			 const opts_t *opts, void *(*tfunc)(void *))
{
	size_t i;
	int res;
	thread_info_t *threads;
	test_result_t tres = { 0 };
	tui_progress_t progress = { 0 };
	tui_metrics_t metrics;
	uint64_t start;
	uint64_t last_render = 0;
	const uint64_t render_interval_ns = 100000000UL; /* 100ms */

	threads = platform->calloc(opts->threads, sizeof(*threads));
	if (!threads)
		return 1;

	calculate_frame_range(threads, opts);

	/* Detect filesystem type */
	tres.filesystem_type = platform_detect_filesystem(opts->path);

	/* Initialize TUI */
	if (tui_init() != 0) {
		fprintf(stderr,
			"Warning: TUI not supported, falling back to standard output\n");
		platform->free(threads);
		return run_test_threads(platform, tst, opts, tfunc);
	}

	/* Initialize TUI metrics */
	tui_metrics_init(&metrics, opts->profile.name, opts->path,
			 opts->threads, opts->frames, tst,
			 tres.filesystem_type);

	progress.running = 1;

	/* Start timing */
	start = timing_start();

	/* Create worker threads with shared progress */
	for (i = 0; i < opts->threads; i++) {
		int thread_res;

		threads[i].id = i;
		threads[i].platform = platform;
		threads[i].opts = opts;
		threads[i].tui_progress = &progress;

		thread_res = platform->thread_create(&threads[i].thread, tfunc,
						     (void *)&threads[i]);
		if (thread_res) {
			size_t j;
			void *ret;

			for (j = 0; j < i; j++)
				platform->thread_cancel(threads[j].thread);
			for (j = 0; j < i; j++)
				platform->thread_join(threads[j].thread, &ret);
			tui_cleanup();
			platform->free(threads);
			return 1;
		}
	}

	/* TUI update loop - poll progress and render until all threads complete */
	while (progress.running) {
		uint64_t now = timing_start();
		uint64_t elapsed = timing_elapsed(start);

		/* Update metrics from shared progress (read atomically) */
		metrics.frames_completed =
			__sync_fetch_and_add(&progress.frames_completed, 0);
		metrics.frames_succeeded =
			__sync_fetch_and_add(&progress.frames_succeeded, 0);
		metrics.frames_failed =
			__sync_fetch_and_add(&progress.frames_failed, 0);
		metrics.bytes_written =
			__sync_fetch_and_add(&progress.bytes_written, 0);
		metrics.elapsed_ns = elapsed;
		metrics.current_io_mode = progress.last_io_mode;

		/* Update sparkline with latest frame time */
		if (progress.last_frame_time_ns > 0) {
			tui_metrics_update(&metrics,
					   progress.last_frame_time_ns, 0,
					   progress.last_io_mode, 1);
		}

		/* Render at interval */
		if (now - last_render >= render_interval_ns) {
			tui_render(&metrics);
			last_render = now;
		}

		/* Small sleep to avoid busy-waiting */
		platform->usleep(10000); /* 10ms */

		/* Check if we've processed all frames */
		size_t total_done =
			metrics.frames_succeeded + metrics.frames_failed;
		if (total_done >= opts->frames)
			break;
	}

	/* Wait for all threads to complete */
	res = 0;
	for (i = 0; i < opts->threads; i++) {
		void *ret;

		if (platform->thread_join(threads[i].thread, &ret))
			res = 1;
		if (ret)
			res = 1;

		if (test_result_aggregate(&tres, &threads[i].res))
			res = 1;
		result_free(platform, &threads[i].res);
	}

	tres.time_taken_ns = timing_elapsed(start);

	/* Final render */
	metrics.frames_completed = tres.frames_written;
	metrics.frames_succeeded = tres.frames_succeeded;
	metrics.frames_failed = tres.frames_failed;
	metrics.bytes_written = tres.bytes_written;
	metrics.elapsed_ns = tres.time_taken_ns;
	metrics.frames_direct_io = tres.frames_direct_io;
	metrics.frames_buffered_io = tres.frames_buffered_io;

	/* Calculate percentiles from completion data */
	if (tres.completion && tres.frames_written > 0) {
		uint64_t *frame_times = platform->malloc(tres.frames_written *
							 sizeof(uint64_t));
		if (frame_times) {
			for (i = 0; i < tres.frames_written; i++) {
				frame_times[i] = tres.completion[i].frame -
						 tres.completion[i].start;
			}
			tui_percentiles_t percs = tui_calculate_percentiles(
				frame_times, tres.frames_written);
			metrics.latency_p50_ns = percs.p50;
			metrics.latency_p95_ns = percs.p95;
			metrics.latency_p99_ns = percs.p99;
			metrics.latency_min_ns = tres.min_frame_time_ns;
			metrics.latency_max_ns = tres.max_frame_time_ns;
			platform->free(frame_times);
		}
	}

	/* Cleanup TUI and show final summary */
	tui_cleanup();
	tui_render_summary(&metrics, &tres);

	result_free(platform, &tres);
	platform->free(threads);
	return res;
}
#endif /* NO_TUI */

int run_tests(opts_t *opts)
{
	const platform_t *platform = NULL;

	if (!opts)
		return 1;

	platform = platform_get();
	if (opts->profile.prof == PROF_INVALID && opts->prof != PROF_INVALID) {
		opts->profile = profile_get_by_type(opts->prof);
	}
	if (opts->mode & TEST_EMPTY)
		opts->profile = profile_get_by_name("empty");
	else if (opts->profile.prof == PROF_INVALID && opts->write_size) {
		opts->profile.prof = PROF_CUSTOM;
		opts->profile.name = "custom";

		/* Faking the size */
		opts->profile.width = opts->write_size;
		opts->profile.bytes_per_pixel = 1;
		opts->profile.height = 1;
		opts->profile.header_size = 0;
	}
	if ((opts->mode & TEST_WRITE) && opts->profile.prof == PROF_INVALID) {
		fprintf(stderr, "No test profile found!\n");
		return 1;
	}
	opts->profile.header_size =
		(opts->mode & TEST_EMPTY) ? 0 : opts->header_size;
	if (opts->profile.prof == PROF_INVALID &&
	    opts->stream_prof != PROF_INVALID)
		opts->profile = profile_get_by_type(opts->stream_prof);
	else if (opts->profile.prof == PROF_INVALID && opts->frame_size) {
		opts->profile = profile_get_by_frame_size(
			opts->frame_size, opts->profile.header_size);
		if (opts->profile.prof == PROF_INVALID) {
			opts->profile.prof = PROF_CUSTOM;
			opts->profile.name = "custom";

			/* Faking the size */
			opts->profile.width = opts->frame_size;
			opts->profile.bytes_per_pixel = 1;
			opts->profile.height = 1;
			opts->profile.header_size = 0;
		}
	}
	if (opts->single_file && (opts->mode & TEST_READ) &&
	    opts->profile.prof == PROF_INVALID) {
		fprintf(stderr,
			"Frame size (-z) is required for streaming test\n");
		return 1;
	}

	if (opts->mode & TEST_WRITE)
		opts->frm = frame_gen(platform, opts->profile);
	else if (opts->mode & TEST_READ) {
		if (opts->single_file || opts->profile.prof != PROF_INVALID)
			opts->frm = frame_gen(platform, opts->profile);
		if (!opts->frm) {
			opts->frm = tester_get_frame_read(
				platform, opts->path,
				opts->profile.header_size);
		}
		if (!opts->frm) {
			fprintf(stderr, "Can't allocate frame\n");
			return 1;
		}
		opts->profile = opts->frm->profile;
	}
	if (!opts->csv && !opts->json && !opts->tui)
		printf("Profile: %s\n", opts->profile.name);

	if (opts->csv && !opts->no_csv_header)
		print_header_csv(opts);

	/* Select appropriate test runner based on TUI mode */
#ifndef NO_TUI
	if (opts->tui) {
		/* TUI mode - use real-time dashboard */
		if (opts->mode & TEST_WRITE) {
			if (!opts->frm) {
				fprintf(stderr, "Can't allocate frame\n");
				return 1;
			}
			run_test_threads_tui(platform, "write", opts,
					     &run_write_test_thread_tui);
		}
		if (opts->mode & TEST_READ) {
			run_test_threads_tui(platform, "read", opts,
					     &run_read_test_thread_tui);
		}
	} else
#endif
	{
		/* Standard mode */
		if (opts->mode & TEST_WRITE) {
			if (!opts->frm) {
				fprintf(stderr, "Can't allocate frame\n");
				return 1;
			}
			run_test_threads(platform, "write", opts,
					 &run_write_test_thread);
		}
		if (opts->mode & TEST_READ) {
			run_test_threads(platform, "read", opts,
					 &run_read_test_thread);
		}
	}
	frame_destroy(platform, opts->frm);

	return 0;
}

int opt_parse_frame_size_helper(opts_t *opt, const char *arg,
				enum ProfileType *prof, size_t *sz)
{
	char *endp = NULL;

	if (!strcmp(arg, "sd") || !strcmp(arg, "SD")) {
		*prof = PROF_SD;
		return 0;
	} else if (!strcmp(arg, "hd") || !strcmp(arg, "HD")) {
		*prof = PROF_HD;
		return 0;
	} else if (!strcmp(arg, "fullhd") || !strcmp(arg, "FULLHD")) {
		*prof = PROF_FULLHD;
		return 0;
	} else if (!strcmp(arg, "2k") || !strcmp(arg, "2K")) {
		*prof = PROF_2K;
		return 0;
	} else if (!strcmp(arg, "4k") || !strcmp(arg, "4K")) {
		*prof = PROF_4K;
		return 0;
	} else if (!strcmp(arg, "8k") || !strcmp(arg, "8K")) {
		*prof = PROF_8K;
		return 0;
	}

	*sz = strtoll(arg, &endp, 10);
	if (!endp || *endp != 0 || !*sz)
		return 1;

	return 0;
}

int opt_parse_write(opts_t *opt, const char *arg)
{
	return opt_parse_frame_size_helper(opt, arg, &opt->prof,
					   &opt->write_size);
}

int opt_parse_profile(opts_t *opt, const char *arg)
{
	profile_t prof;

	if (!arg)
		return 1;

	/* Try to get profile by name first */
	prof = profile_get_by_name(arg);
	if (prof.prof != PROF_INVALID) {
		opt->profile = prof;
		return 0;
	}

	/* Try to parse as custom resolution (WIDTHxHEIGHTxBITS) */
	prof = profile_parse_custom(arg);
	if (prof.width > 0 && prof.height > 0) {
		opt->profile = prof;
		return 0;
	}

	return 1;
}

static inline int parse_arg_size_t(const char *arg, size_t *res, int zero_ok)
{
	char *endp = NULL;
	size_t val;

	if (!arg || !res)
		return 1;

	val = strtoll(arg, &endp, 10);
	if (!endp || *endp != 0)
		return 1;
	if (!zero_ok && !val)
		return 1;

	*res = val;

	return 0;
}

int opt_parse_threads(opts_t *opt, const char *arg)
{
	return parse_arg_size_t(arg, &opt->threads, 0);
}

int opt_parse_num_frames(opts_t *opt, const char *arg)
{
	return parse_arg_size_t(arg, &opt->frames, 0);
}

int opt_parse_limit_fps(opts_t *opt, const char *arg)
{
	return parse_arg_size_t(arg, &opt->fps, 0);
}

int opt_parse_header_size(opts_t *opt, const char *arg)
{
	return parse_arg_size_t(arg, &opt->header_size, 1);
}

int opt_parse_frame_size(opts_t *opt, const char *arg)
{
	return opt_parse_frame_size_helper(opt, arg, &opt->stream_prof,
					   &opt->frame_size);
}

void list_profiles(void)
{
	size_t cnt = profile_count();
	size_t i;

	printf("Profiles:\n");
	for (i = 1; i < cnt; i++) {
		profile_t prof = profile_get_by_index(i);

		printf("  %s\n", prof.name);
		printf("     %zux%zu, %zu bits, %zuB header\n", prof.width,
		       prof.height, prof.bytes_per_pixel * 8, prof.header_size);
	}
}

struct long_opt_desc {
	const char *name;
	const char *desc;
};
static struct option long_opts[] = {
	{ "write", required_argument, 0, 'w' },
	{ "read", no_argument, 0, 'r' },
	{ "empty", no_argument, 0, 'e' },
	{ "streaming", no_argument, 0, 's' },
	{ "frame-size", no_argument, 0, 'z' },
	{ "list-profiles", no_argument, 0, 'l' },
	{ "threads", required_argument, 0, 't' },
	{ "num-frames", required_argument, 0, 'n' },
	{ "fps", required_argument, 0, 'f' },
	{ "reverse", no_argument, 0, 'v' },
	{ "random", no_argument, 0, 'm' },
	{ "csv", no_argument, 0, 'c' },
	{ "json", no_argument, 0, 'j' },
	{ "no-csv-header", no_argument, 0, 0 },
	{ "header", required_argument, 0, 0 },
	{ "times", no_argument, 0, 0 },
	{ "frametimes", no_argument, 0, 0 },
	{ "histogram", no_argument, 0, 0 },
	{ "tui", no_argument, 0, 0 },
	{ "interactive", no_argument, 0, 'i' },
	{ "history-size", required_argument, 0, 0 },
	{ "version", no_argument, 0, 'V' },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 },
};
static size_t long_opts_cnt = sizeof(long_opts) / sizeof(long_opts[0]);
static struct long_opt_desc long_opt_descs[] = {
	{ "write", "Perform write tests, size/profile as parameter" },
	{ "read", "Perform read tests" },
	{ "empty", "Perform write tests with empty frames" },
	{ "streaming", "Perform streaming test to a single file" },
	{ "frame-size",
	  "Specify frame size for reading, required for streaming" },
	{ "list-profiles", "List available profiles" },
	{ "threads", "Use number of threads (default 1)" },
	{ "num-frames", "Write number of frames (default 1800)" },
	{ "fps", "Limit frame rate to frames per second" },
	{ "reverse", "Access files in reverse order" },
	{ "random", "Access files in random order" },
	{ "csv", "Output results in CSV format" },
	{ "json", "Output results in JSON format" },
	{ "no-csv-header", "Do not print CSV header" },
	{ "header", "Frame header size (default 64k)" },
	{ "times", "Show breakdown of completion times (open/io/close)" },
	{ "frametimes", "Show detailed timings of every frames in CSV format" },
	{ "histogram", "Show histogram of completion times at the end" },
	{ "tui", "Show real-time TUI dashboard during test" },
	{ "interactive", "Launch interactive TTY mode with config menu" },
	{ "history-size",
	  "Frame history depth for interactive mode (default 10000)" },
	{ "version", "Display version information" },
	{ "help", "Display this help" },
	{ 0, 0 },
};

#define XSTRING(x) #x
#define VERSION_STRING(a, b, c) XSTRING(a) "." XSTRING(b) "." XSTRING(c)
void version(void)
{
	fprintf(stderr, "vframetest %s\n", VERSION_STRING(MAJOR, MINOR, PATCH));
}
#undef XSTRING
#undef VERSION_STRING

#define DESC_POS 30
void usage(const char *name)
{
	size_t i;

	version();
	fprintf(stderr, "Usage: %s [options] path\n", name);
	fprintf(stderr, "Options:\n");
	for (i = 0; i < long_opts_cnt; i++) {
		int p = 8;

		if (!long_opts[i].name)
			break;
		if (long_opts[i].val)
			fprintf(stderr, "    -%c, ", long_opts[i].val);
		else
			fprintf(stderr, "        ");

		p += strlen(long_opts[i].name);
		fprintf(stderr, "--%s", long_opts[i].name);

		if (p < DESC_POS)
			p = DESC_POS - p;
		else
			p = 1;
		fprintf(stderr, "%*s%s\n", p, " ", long_opt_descs[i].desc);
	}
}
#undef DESC_POS

/* ─────────────────────────────────────────────────────────────────────────────
 * Interactive TTY mode (Unix only)
 * ───────────────────────────────────────────────────────────────────────────── */

#ifndef NO_TUI
/* Create test directory if it doesn't exist */
static int ensure_test_directory(const char *path)
{
	struct stat st;

	if (stat(path, &st) == 0) {
		/* Path exists - check if it's a directory */
		if (S_ISDIR(st.st_mode)) {
			return 0; /* Directory exists */
		}
		return -1; /* Exists but not a directory */
	}

	/* Directory doesn't exist - create it */
	if (mkdir(path, 0755) == 0) {
		return 0;
	}

	return -1;
}

/* Count test files and calculate total size */
static int count_test_files(const char *path, size_t *total_bytes)
{
	DIR *dir;
	struct dirent *entry;
	char filepath[PATH_MAX];
	struct stat st;
	int count = 0;

	if (total_bytes)
		*total_bytes = 0;

	dir = opendir(path);
	if (!dir) {
		return -1;
	}

	while ((entry = readdir(dir)) != NULL) {
		/* Only count frame*.tst files */
		if (strncmp(entry->d_name, "frame", 5) == 0 &&
		    strstr(entry->d_name, ".tst") != NULL) {
			count++;
			if (total_bytes) {
				snprintf(filepath, sizeof(filepath), "%s/%s",
					 path, entry->d_name);
				if (stat(filepath, &st) == 0) {
					*total_bytes += st.st_size;
				}
			}
		}
	}

	closedir(dir);
	return count;
}

/* Clean up test files from directory (removes frame*.tst files only) */
static int cleanup_test_files(const char *path)
{
	DIR *dir;
	struct dirent *entry;
	char filepath[PATH_MAX];
	int count = 0;

	dir = opendir(path);
	if (!dir) {
		return -1;
	}

	while ((entry = readdir(dir)) != NULL) {
		/* Only remove frame*.tst files */
		if (strncmp(entry->d_name, "frame", 5) == 0 &&
		    strstr(entry->d_name, ".tst") != NULL) {
			snprintf(filepath, sizeof(filepath), "%s/%s", path,
				 entry->d_name);
			if (unlink(filepath) == 0) {
				count++;
			}
		}
	}

	closedir(dir);
	return count;
}

/* Open dashboard HTML report in browser (no server needed) */
static int open_dashboard(const char *report_path)
{
#if defined(__APPLE__)
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "open '%s' 2>/dev/null", report_path);
	return system(cmd);
#elif defined(_WIN32)
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", report_path);
	return system(cmd);
#else
	/* Linux */
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "xdg-open '%s' 2>/dev/null", report_path);
	int ret = system(cmd);
	if (ret != 0) {
		snprintf(cmd, sizeof(cmd), "sensible-browser '%s' 2>/dev/null",
			 report_path);
		ret = system(cmd);
	}
	return ret;
#endif
}

/* Compare function for qsort */
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

/* Generate self-contained HTML report with embedded test data */
static int generate_html_report(const char *output_path,
				const char *template_path,
				tui_app_state_t *state, tui_metrics_t *metrics,
				const opts_t *test_opts)
{
	FILE *tmpl = fopen(template_path, "r");
	FILE *out = fopen(output_path, "w");
	if (!tmpl || !out) {
		if (tmpl)
			fclose(tmpl);
		if (out)
			fclose(out);
		return -1;
	}

	size_t frame_count = tui_history_count(state);

	/* Calculate stats */
	uint64_t *durations = NULL;
	size_t valid_count = 0;
	uint64_t total_ns = 0, min_ns = UINT64_MAX, max_ns = 0;

	if (frame_count > 0) {
		durations = malloc(frame_count * sizeof(uint64_t));
		if (durations) {
			for (size_t i = 0; i < frame_count; i++) {
				const tui_frame_record_t *f =
					tui_history_get(state, i);
				if (f && f->duration_ns > 0) {
					durations[valid_count++] =
						f->duration_ns;
					total_ns += f->duration_ns;
					if (f->duration_ns < min_ns)
						min_ns = f->duration_ns;
					if (f->duration_ns > max_ns)
						max_ns = f->duration_ns;
				}
			}
			if (valid_count > 0)
				qsort(durations, valid_count, sizeof(uint64_t),
				      compare_uint64);
		}
	}

	uint64_t p50_ns = 0, p95_ns = 0, p99_ns = 0;
	if (durations && valid_count > 0) {
		p50_ns = durations[(size_t)(valid_count * 0.50)];
		p95_ns = durations[(size_t)(valid_count * 0.95)];
		p99_ns = durations[valid_count > 1 ?
					   (size_t)(valid_count * 0.99) :
					   valid_count - 1];
	}

	double avg_ms = valid_count > 0 ? (double)total_ns / valid_count / 1e6 :
					  0;
	double elapsed_sec = (double)metrics->elapsed_ns / 1e9;
	double throughput =
		elapsed_sec > 0 ?
			((double)metrics->bytes_written / (1024.0 * 1024.0)) /
				elapsed_sec :
			0;

	/* Read template and inject data after <head> */
	char line[4096];
	int injected = 0;

	while (fgets(line, sizeof(line), tmpl)) {
		fputs(line, out);

		/* Inject data script after opening <head> tag */
		if (!injected && strstr(line, "<head>")) {
			fprintf(out, "<script>window.VFRAMETEST_DATA = {\n");
			fprintf(out,
				"  \"config\": {\"profile\": \"%s\", \"path\": \"%s\", \"threads\": %zu, \"frames\": %zu, \"filesystem\": \"%s\"},\n",
				test_opts->profile.name, test_opts->path,
				test_opts->threads, test_opts->frames,
				metrics->fs_type == FILESYSTEM_SMB ? "SMB" :
				metrics->fs_type == FILESYSTEM_NFS ? "NFS" :
								     "LOCAL");
			fprintf(out,
				"  \"summary\": {\"total_frames\": %zu, \"frames_succeeded\": %zu, \"frames_failed\": %zu, \"throughput_mibs\": %.2f, \"duration_sec\": %.2f, \"io_mode\": \"%s\"},\n",
				metrics->frames_completed,
				metrics->frames_succeeded,
				metrics->frames_failed, throughput, elapsed_sec,
				metrics->current_io_mode == IO_MODE_DIRECT ?
					"Direct" :
					"Buffered");
			fprintf(out,
				"  \"latency\": {\"min_ms\": %.4f, \"max_ms\": %.4f, \"avg_ms\": %.4f, \"p50_ms\": %.4f, \"p95_ms\": %.4f, \"p99_ms\": %.4f},\n",
				min_ns == UINT64_MAX ? 0.0 :
						       (double)min_ns / 1e6,
				(double)max_ns / 1e6, avg_ms,
				(double)p50_ns / 1e6, (double)p95_ns / 1e6,
				(double)p99_ns / 1e6);

			/* Frames array */
			fprintf(out, "  \"frames\": [");
			for (size_t i = 0; i < frame_count; i++) {
				const tui_frame_record_t *f =
					tui_history_get(state, i);
				if (!f)
					continue;
				fprintf(out,
					"%s{\"frame_num\":%zu,\"duration_ms\":%.4f,\"bytes\":%zu,\"io_mode\":\"%s\",\"success\":%s,\"thread\":%d}",
					i > 0 ? "," : "", f->frame_num,
					(double)f->duration_ns / 1e6, f->bytes,
					f->io_mode == IO_MODE_DIRECT ?
						"direct" :
						"buffered",
					f->success ? "true" : "false",
					f->thread_id);
			}
			fprintf(out, "],\n");

			/* Throughput samples */
			fprintf(out, "  \"throughput_samples\": [");
			if (frame_count > 0) {
				size_t window_size = frame_count / 50;
				if (window_size < 1)
					window_size = 1;
				int first = 1;
				for (size_t start = 0; start < frame_count;
				     start += window_size) {
					size_t end = start + window_size;
					if (end > frame_count)
						end = frame_count;
					size_t window_bytes = 0;
					uint64_t window_duration_ns = 0;
					size_t window_frame_start = 0;
					for (size_t j = start; j < end; j++) {
						const tui_frame_record_t *f =
							tui_history_get(state,
									j);
						if (f) {
							window_bytes +=
								f->bytes;
							window_duration_ns +=
								f->duration_ns;
							if (j == start)
								window_frame_start =
									f->frame_num;
						}
					}
					double wtp =
						window_duration_ns > 0 ?
							((double)window_bytes /
							 (1024.0 * 1024.0)) /
								((double)window_duration_ns /
								 1e9) :
							0;
					fprintf(out,
						"%s{\"frame\":%zu,\"throughput_mibs\":%.2f}",
						first ? "" : ",",
						window_frame_start, wtp);
					first = 0;
				}
			}

			time_t now = time(NULL);
			char timestamp[64];
			strftime(timestamp, sizeof(timestamp),
				 "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
			fprintf(out,
				"],\n  \"timestamp\": \"%s\"\n};</script>\n",
				timestamp);
			injected = 1;
		}
	}

	if (durations)
		free(durations);
	fclose(tmpl);
	fclose(out);
	return 0;
}

/* Show cleanup prompt and wait for user response */
static int show_cleanup_prompt(tui_app_state_t *state, int file_count,
			       size_t total_bytes)
{
	screen_t scr;
	int width = state->term_width > 80 ? 80 : state->term_width;
	int height = state->term_height > 24 ? 24 : state->term_height;

	screen_init(&scr, width, height);

	/* Draw box */
	screen_box(&scr, 0, 0, width, height);

	/* Title */
	int row = 2;
	screen_move(&scr, row++, (width - 20) / 2);
	screen_print(&scr, "=== Test Complete ===");
	row++;

	/* File info */
	char buf[128];
	double size_gb = (double)total_bytes / (1024.0 * 1024.0 * 1024.0);
	snprintf(buf, sizeof(buf), "%d test files (%.1f GB)", file_count,
		 size_gb);
	screen_move(&scr, row++, (width - strlen(buf)) / 2);
	screen_print(&scr, buf);
	row++;

	/* Prompt */
	screen_move(&scr, row++, (width - 30) / 2);
	screen_print(&scr, "Delete test files?");
	row++;

	screen_move(&scr, row++, (width - 30) / 2);
	screen_print(&scr, "[Y] Yes   [N] No   [Q] Quit");

	screen_render(&scr);

	/* Wait for response */
	while (1) {
		char c = 0;
		if (read(STDIN_FILENO, &c, 1) == 1) {
			if (c == 'y' || c == 'Y') {
				return 1; /* Delete files */
			} else if (c == 'n' || c == 'N') {
				return 0; /* Keep files */
			} else if (c == 'q' || c == 'Q') {
				return -1; /* Quit */
			}
		}
	}
}

static int run_interactive(opts_t *opts)
{
	tui_app_state_t state;
	tui_metrics_t metrics;
	int result = 0;

	/* Check terminal support */
	if (!tty_is_supported()) {
		fprintf(stderr,
			"ERROR: Interactive mode requires a terminal\n");
		return 1;
	}

	/* Initialize state */
	size_t history_size = opts->history_size > 0 ? opts->history_size :
						       TUI_HISTORY_DEFAULT;
	if (tui_state_init(&state, history_size) != 0) {
		fprintf(stderr,
			"ERROR: Failed to initialize interactive state\n");
		return 1;
	}

	/* Initialize metrics (empty for now) */
	memset(&metrics, 0, sizeof(metrics));

	/* Initialize terminal */
	if (tty_init() != 0) {
		fprintf(stderr, "ERROR: Failed to initialize terminal\n");
		tui_state_cleanup(&state);
		return 1;
	}

	/* Main event loop */
	while (!tui_should_quit(&state)) {
		/* Check for resize */
		if (tty_was_resized()) {
			tty_size_t size = tty_get_size();
			tui_state_set_size(&state, size.width, size.height);
		}

		/* Render if needed */
		if (state.needs_redraw) {
			tui_render_screen(&state,
					  state.run_state != TUI_STATE_IDLE ?
						  &metrics :
						  NULL);
		}

		/* Process input (100ms timeout for responsive UI) */
		tui_input_process(&state, 100);

		/* Check if test was requested */
		if (tui_config_test_requested(&state)) {
			/* Convert TUI config to opts */
			opts_t test_opts;
			memset(&test_opts, 0, sizeof(test_opts));

			/* Set path */
			test_opts.path = state.config.path;

			/* Validate path */
			if (!test_opts.path || !test_opts.path[0]) {
				state.run_state = TUI_STATE_IDLE;
				state.current_view = TUI_VIEW_CONFIG;
				state.needs_redraw = 1;
				continue;
			}

			/* Create test directory if it doesn't exist */
			if (ensure_test_directory(test_opts.path) != 0) {
				state.run_state = TUI_STATE_IDLE;
				state.current_view = TUI_VIEW_CONFIG;
				state.needs_redraw = 1;
				continue;
			}

			/* Set profile based on TUI selection */
			switch (state.config.profile) {
			case TUI_PROFILE_SD:
				test_opts.profile =
					profile_get_by_name("SD-24bit");
				break;
			case TUI_PROFILE_HD:
				test_opts.profile =
					profile_get_by_name("HD-24bit");
				break;
			case TUI_PROFILE_FULLHD:
				test_opts.profile =
					profile_get_by_name("FULLHD-24bit");
				break;
			case TUI_PROFILE_2K:
				test_opts.profile =
					profile_get_by_name("2K-24bit");
				break;
			case TUI_PROFILE_4K:
				test_opts.profile =
					profile_get_by_name("4K-24bit");
				break;
			case TUI_PROFILE_8K:
				test_opts.profile =
					profile_get_by_name("8K-24bit");
				break;
			default:
				test_opts.profile =
					profile_get_by_name("FULLHD-24bit");
			}

			/* Set other parameters */
			test_opts.threads = state.config.threads > 0 ?
						    state.config.threads :
						    1;
			test_opts.frames = state.config.frames > 0 ?
						   state.config.frames :
						   100;
			test_opts.fps = state.config.fps;
			test_opts.header_size = state.config.header_size;

			/* Set test mode */
			switch (state.config.test_type) {
			case TUI_TEST_WRITE:
				test_opts.mode = TEST_WRITE;
				break;
			case TUI_TEST_READ:
				test_opts.mode = TEST_READ;
				break;
			case TUI_TEST_EMPTY:
				test_opts.mode = TEST_EMPTY;
				break;
			default:
				test_opts.mode = TEST_WRITE;
			}

			/* Create frame for test */
			const platform_t *platform = platform_get();
			test_opts.profile.header_size = test_opts.header_size;
			frame_t *frm = frame_gen(platform, test_opts.profile);
			if (!frm) {
				state.run_state = TUI_STATE_IDLE;
				state.needs_redraw = 1;
				continue;
			}

			/* Initialize metrics for display */
			tui_metrics_init(
				&metrics, test_opts.profile.name,
				test_opts.path, test_opts.threads,
				test_opts.frames,
				test_opts.mode == TEST_WRITE ? "write" : "read",
				FILESYSTEM_LOCAL);

			/* Reset state for new test (clears history from previous runs) */
			tui_state_reset_for_test(&state);

			/* Switch to dashboard and mark as running */
			state.run_state = TUI_STATE_RUNNING;
			state.current_view = TUI_VIEW_DASHBOARD;
			state.needs_redraw = 1;

			/* Set up progress tracking */
			tui_progress_t progress = { 0 };

			/* Start worker threads */
			size_t frames_per_thread =
				test_opts.frames / test_opts.threads;
			thread_info_t *threads = platform->malloc(
				sizeof(*threads) * test_opts.threads);
			if (!threads) {
				frame_destroy(platform, frm);
				state.run_state = TUI_STATE_IDLE;
				state.needs_redraw = 1;
				continue;
			}

			uint64_t start_time = timing_start();

			test_opts.frm = frm;

			for (size_t i = 0; i < test_opts.threads; i++) {
				threads[i].platform = platform;
				threads[i].opts = &test_opts;
				threads[i].start_frame = i * frames_per_thread;
				threads[i].frames =
					(i == test_opts.threads - 1) ?
						test_opts.frames -
							threads[i].start_frame :
						frames_per_thread;
				threads[i].fps = test_opts.fps;
				threads[i].tui_progress = &progress;
				memset(&threads[i].res, 0,
				       sizeof(threads[i].res));

				if (platform->thread_create(
					    &threads[i].thread,
					    run_write_test_thread_tui,
					    &threads[i])) {
					/* Thread creation failed */
					threads[i].thread = 0;
				}
			}

			/* Main loop - update display while test runs */
			size_t last_frame_count = 0;
			size_t frame_bytes =
				frm->size; /* Cache frame size for history */
			while (state.run_state == TUI_STATE_RUNNING) {
				/* Update timing metrics (frame counts come from tui_metrics_update) */
				size_t current_frames =
					progress.frames_completed;
				metrics.elapsed_ns = timing_elapsed(start_time);

				if (progress.last_frame_time_ns > 0) {
					/* Update latency stats */
					metrics.current_io_mode =
						progress.last_io_mode;

					/* Add ALL new frames to history since last check */
					while (last_frame_count <
					       current_frames) {
						last_frame_count++;
						tui_frame_record_t rec = {
							.frame_num =
								last_frame_count,
							.start_ns =
								timing_start(),
							.duration_ns =
								progress.last_frame_time_ns,
							.bytes = frame_bytes,
							.io_mode =
								progress.last_io_mode,
							.success = 1,
							.thread_id =
								(int)(last_frame_count %
								      test_opts.threads)
						};
						tui_history_add(&state, &rec);

						/* Update metrics with latency min/max/sparkline */
						tui_metrics_update(
							&metrics,
							progress.last_frame_time_ns,
							frame_bytes,
							progress.last_io_mode,
							1);
					}
				}

				/* Check if done (use progress counter as authoritative source) */
				if (current_frames >= test_opts.frames) {
					state.run_state = TUI_STATE_COMPLETED;
				}

				/* Render */
				tui_render_screen(&state, &metrics);

				/* Process input (allow quit/pause) */
				tui_input_process(&state, 50);

				/* Check for quit */
				if (tui_should_quit(&state)) {
					break;
				}
			}

			/* Wait for threads to finish */
			for (size_t i = 0; i < test_opts.threads; i++) {
				if (threads[i].thread) {
					platform->thread_join(threads[i].thread,
							      NULL);
					result_free(platform, &threads[i].res);
				}
			}

			platform->free(threads);
			frame_destroy(platform, frm);

			/* Final update */
			metrics.elapsed_ns = timing_elapsed(start_time);
			state.needs_redraw = 1;

			/* Generate self-contained HTML report */
			const char *report_path =
				"report-dashboard/report.html";
			generate_html_report(report_path,
					     "report-dashboard/index.html",
					     &state, &metrics, &test_opts);

			/* Open dashboard in browser if enabled */
			if (state.config.open_dashboard) {
				open_dashboard(report_path);
			}

			/* Handle cleanup based on config */
			if (state.config.auto_cleanup) {
				/* Auto-cleanup enabled - delete files silently */
				cleanup_test_files(test_opts.path);
			} else {
				/* Prompt user for cleanup */
				size_t total_bytes = 0;
				int file_count = count_test_files(
					test_opts.path, &total_bytes);
				if (file_count > 0) {
					int response = show_cleanup_prompt(
						&state, file_count,
						total_bytes);
					if (response == 1) {
						cleanup_test_files(
							test_opts.path);
					} else if (response == -1) {
						/* User chose quit */
						state.run_state =
							TUI_STATE_QUITTING;
					}
				}
				state.needs_redraw = 1;
			}
		}
	}

	/* Cleanup */
	tty_cleanup();
	tui_state_cleanup(&state);

	return result;
}
#endif /* NO_TUI */

int main(int argc, char **argv)
{
	opts_t opts = { 0 };
	int c = 0;
	int opt_index = 0;

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	srand((unsigned)(ts.tv_sec ^ ts.tv_nsec ^ getpid()));
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	opts.threads = 1;
	opts.frames = 1800;
	opts.header_size = 65536;
	while (1) {
		c = getopt_long(argc, argv, "irw:elt:n:f:s:z:vmhVc", long_opts,
				&opt_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (!strcmp(long_opts[opt_index].name, "no-csv-header"))
				opts.no_csv_header = 1;
			if (!strcmp(long_opts[opt_index].name, "histogram"))
				opts.histogram = 1;
			if (!strcmp(long_opts[opt_index].name, "tui"))
				opts.tui = 1;
			if (!strcmp(long_opts[opt_index].name, "times"))
				opts.times = 1;
			if (!strcmp(long_opts[opt_index].name, "frametimes"))
				opts.frametimes = 1;
			if (!strcmp(long_opts[opt_index].name, "header")) {
				if (opt_parse_header_size(&opts, optarg))
					goto invalid_long;
			}
			if (!strcmp(long_opts[opt_index].name, "interactive"))
				opts.interactive = 1;
			if (!strcmp(long_opts[opt_index].name,
				    "history-size")) {
				opts.history_size = (size_t)atol(optarg);
			}
			break;
		case 'i':
			opts.interactive = 1;
			break;
		case 'h':
			usage(argv[0]);
			return 1;
		case 'c':
			opts.csv = 1;
			break;
		case 'j':
			opts.json = 1;
			break;
		case 'v':
			opts.reverse = 1;
			break;
		case 'm':
			opts.random = 1;
			break;
		case 'w':
			if (opt_parse_write(&opts, optarg)) {
				if (opt_parse_profile(&opts, optarg)) {
					/* Could not parse profile, just skip */
				}
			}
			opts.mode |= TEST_WRITE;
			break;
		case 'e':
			opts.mode |= TEST_WRITE;
			opts.mode |= TEST_EMPTY;
			break;
		case 'r':
			opts.mode |= TEST_READ;
			break;
		case 's':
			opts.single_file = 1;
			opts.path = optarg;
			break;
		case 't':
			if (opt_parse_threads(&opts, optarg))
				goto invalid_short;
			break;
		case 'n':
			if (opt_parse_num_frames(&opts, optarg))
				goto invalid_short;
			break;
		case 'f':
			if (opt_parse_limit_fps(&opts, optarg))
				goto invalid_short;
			break;
		case 'z':
			if (opt_parse_frame_size(&opts, optarg))
				goto invalid_short;
			break;
		case 'l':
			list_profiles();
			return 0;
		case 'V':
			version();
			return 0;
		default:
			printf("Invalid option: %c\n", c);
			return 1;
		}
	}
	if (optind < argc) {
		int i;

		for (i = optind; i < argc; i++) {
			if (!opts.path)
				opts.path = argv[i];
			else {
				printf("Unknown option: %s\n", argv[i]);
				return 1;
			}
		}
	}
	if (opts.random && opts.reverse) {
		printf("ERROR: --random and --reverse are mutually exclusive, "
		       "please define only one.\n");
		usage(argv[0]);
		return 1;
	}
	/* Interactive mode - launch config menu */
	if (opts.interactive) {
#ifndef NO_TUI
		return run_interactive(&opts);
#else
		fprintf(stderr,
			"ERROR: Interactive mode not available (TUI disabled)\n");
		return 1;
#endif
	}

	if (!opts.path) {
		usage(argv[0]);
		return 1;
	}

	/* Validate path exists and is accessible */
	struct stat path_stat;
	if (stat(opts.path, &path_stat) != 0) {
		fprintf(stderr, "ERROR: Cannot access path '%s': %s\n",
			opts.path, strerror(errno));
		return 1;
	}
	if (!opts.single_file && !S_ISDIR(path_stat.st_mode)) {
		fprintf(stderr, "ERROR: Path '%s' is not a directory\n",
			opts.path);
		return 1;
	}

	return run_tests(&opts);

invalid_long:
	fprintf(stderr, "Invalid argument for option --%s: %s\n",
		long_opts[opt_index].name, optarg);
	return 1;

invalid_short:
	fprintf(stderr,
		"Invalid argument for option "
		"-%c: %s\n",
		c, optarg);
	return 1;
}
