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

#ifndef VFRAMETEST_API_CSV_H
#define VFRAMETEST_API_CSV_H

#include <stddef.h>
#include "api_data.h"

/* CSV export functions - return allocated strings that must be freed */

/* Export metrics to CSV format */
char* api_csv_metrics(const api_metrics_t *metrics);

/* Export frame history to CSV */
char* api_csv_history(const api_history_t *history);

/* Export summary to CSV */
char* api_csv_summary(const api_summary_t *summary);

#endif /* VFRAMETEST_API_CSV_H */
