/*
 * This file is part of vframetest.
 *
 * Copyright (c) 2023-2025 Tuxera Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Define version macros for TUI test compilation */
#ifndef MAJOR
#define MAJOR 25
#endif
#ifndef MINOR
#define MINOR 13
#endif
#ifndef PATCH
#define PATCH 0
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unittest.h"
#include "../src/tui.h"
#include "../src/tui.c"

void test_setup(void **state)
{
	(void)state;
}

void test_teardown(void **state)
{
	(void)state;
}

int test_percentiles_basic(void **state)
{
	uint64_t times[100];
	tui_percentiles_t percs;
	(void)state;

	/* Fill with sequential values 1-100 */
	for (int i = 0; i < 100; i++) {
		times[i] = (uint64_t)(i + 1) * 1000000; /* 1ms to 100ms */
	}

	percs = tui_calculate_percentiles(times, 100);

	/* P50 should be around 50ms */
	TEST_ASSERT(percs.p50 >= 49000000 && percs.p50 <= 51000000);

	/* P95 should be around 95ms */
	TEST_ASSERT(percs.p95 >= 94000000 && percs.p95 <= 96000000);

	/* P99 should be around 99ms */
	TEST_ASSERT(percs.p99 >= 98000000 && percs.p99 <= 100000000);

	return 0;
}

int test_percentiles_empty(void **state)
{
	tui_percentiles_t percs;
	(void)state;

	percs = tui_calculate_percentiles(NULL, 0);

	TEST_ASSERT(percs.p50 == 0);
	TEST_ASSERT(percs.p95 == 0);
	TEST_ASSERT(percs.p99 == 0);

	return 0;
}

int test_metrics_init(void **state)
{
	tui_metrics_t metrics;
	(void)state;

	tui_metrics_init(&metrics, "FULLHD-24bit", "/tmp/test", 4, 1000,
			 "write", FILESYSTEM_LOCAL);

	TEST_ASSERT_EQ_STR(metrics.profile_name, "FULLHD-24bit");
	TEST_ASSERT_EQ_STR(metrics.target_path, "/tmp/test");
	TEST_ASSERT(metrics.thread_count == 4);
	TEST_ASSERT(metrics.frames_total == 1000);
	TEST_ASSERT_EQ_STR(metrics.test_type, "write");
	TEST_ASSERT(metrics.fs_type == FILESYSTEM_LOCAL);
	TEST_ASSERT(metrics.frames_completed == 0);
	TEST_ASSERT(metrics.frames_succeeded == 0);
	TEST_ASSERT(metrics.frames_failed == 0);

	return 0;
}

int test_metrics_update(void **state)
{
	tui_metrics_t metrics;
	(void)state;

	tui_metrics_init(&metrics, "HD", "/tmp/test", 2, 100, "write",
			 FILESYSTEM_LOCAL);

	/* Simulate a few frame completions */
	tui_metrics_update(&metrics, 5000000, 1000000, IO_MODE_DIRECT, 1);
	tui_metrics_update(&metrics, 6000000, 1000000, IO_MODE_DIRECT, 1);
	tui_metrics_update(&metrics, 4000000, 1000000, IO_MODE_DIRECT, 1);

	TEST_ASSERT(metrics.frames_completed == 3);
	TEST_ASSERT(metrics.frames_succeeded == 3);
	TEST_ASSERT(metrics.frames_failed == 0);
	TEST_ASSERT(metrics.bytes_written == 3000000);
	TEST_ASSERT(metrics.frames_direct_io == 3);
	TEST_ASSERT(metrics.frames_buffered_io == 0);
	TEST_ASSERT(metrics.latency_min_ns == 4000000);
	TEST_ASSERT(metrics.latency_max_ns == 6000000);

	return 0;
}

int test_metrics_failures(void **state)
{
	tui_metrics_t metrics;
	(void)state;

	tui_metrics_init(&metrics, "HD", "/tmp/test", 2, 100, "write",
			 FILESYSTEM_LOCAL);

	tui_metrics_update(&metrics, 5000000, 1000000, IO_MODE_DIRECT, 1);
	tui_metrics_update(&metrics, 0, 0, IO_MODE_UNKNOWN, 0); /* failure */
	tui_metrics_update(&metrics, 6000000, 1000000, IO_MODE_BUFFERED, 1);

	TEST_ASSERT(metrics.frames_completed == 3);
	TEST_ASSERT(metrics.frames_succeeded == 2);
	TEST_ASSERT(metrics.frames_failed == 1);
	TEST_ASSERT(metrics.frames_direct_io == 1);
	TEST_ASSERT(metrics.frames_buffered_io == 1);

	return 0;
}

int test_tui_detection(void **state)
{
	(void)state;

	/* In a test environment (non-interactive), TUI should not be supported */
	/* This test verifies the detection logic exists and doesn't crash */
	int supported = tui_is_supported();

	/* We can't assert the value because it depends on environment,
	 * but we verify the function doesn't crash */
	(void)supported;

	return 0;
}

int test_tui(void)
{
	TEST_INIT();

	TESTF(percentiles_basic, test_setup, test_teardown);
	TESTF(percentiles_empty, test_setup, test_teardown);
	TESTF(metrics_init, test_setup, test_teardown);
	TESTF(metrics_update, test_setup, test_teardown);
	TESTF(metrics_failures, test_setup, test_teardown);
	TESTF(tui_detection, test_setup, test_teardown);

	TEST_END();
}

TEST_MAIN(tui)
