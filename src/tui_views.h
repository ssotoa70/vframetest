/*
 * tui_views.h - View implementations for interactive TUI
 *
 * This file is part of vframetest.
 *
 * Copyright (c) 2023-2025 Tuxera Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef FRAMETEST_TUI_VIEWS_H
#define FRAMETEST_TUI_VIEWS_H

#include "tui_state.h"
#include "tui.h"

/* Sparkline size constant (must match tui.h) */
#ifndef TUI_SPARKLINE_SIZE
#define TUI_SPARKLINE_SIZE 20
#endif

/*
 * Render the current view
 * Renders the appropriate view based on state->current_view
 */
void tui_views_render(tui_app_state_t *state, tui_metrics_t *metrics);

/*
 * Render the dashboard view
 */
void tui_view_dashboard(tui_app_state_t *state, tui_metrics_t *metrics);

/*
 * Render the history view
 */
void tui_view_history(tui_app_state_t *state, tui_metrics_t *metrics);

/*
 * Render the latency view
 */
void tui_view_latency(tui_app_state_t *state, tui_metrics_t *metrics);

/*
 * Render the config view (test configuration menu)
 */
void tui_view_config(tui_app_state_t *state, tui_metrics_t *metrics);

/*
 * Render help overlay on top of current view
 */
void tui_view_help_overlay(tui_app_state_t *state);

/*
 * Render the tab bar at top of screen
 */
void tui_render_tab_bar(tui_app_state_t *state, int width);

/*
 * Render the status/key bar at bottom of screen
 */
void tui_render_status_bar(tui_app_state_t *state, int width);

#endif /* FRAMETEST_TUI_VIEWS_H */
