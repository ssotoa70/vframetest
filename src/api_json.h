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

#ifndef VFRAMETEST_API_JSON_H
#define VFRAMETEST_API_JSON_H

#include <stddef.h>
#include "api_data.h"

/* JSON serialization functions - return allocated strings that must be freed */

/* Convert status to JSON */
char* api_json_status(const api_status_t *status);

/* Convert metrics to JSON */
char* api_json_metrics(const api_metrics_t *metrics);

/* Convert I/O modes to JSON */
char* api_json_io_modes(const api_io_modes_t *io_modes);

/* Convert filesystem to JSON */
char* api_json_filesystem(const api_filesystem_t *filesystem);

/* Convert frame history to JSON */
char* api_json_history(const api_history_t *history);

/* Convert summary to JSON */
char* api_json_summary(const api_summary_t *summary);

/* Create error JSON response */
char* api_json_error(int http_status, const char *message);

/* Create success response with data */
char* api_json_response(int http_status, const char *data);

#endif /* VFRAMETEST_API_JSON_H */
