/*
 * tui_render.h - Simple TUI rendering using screen buffer
 *
 * This file is part of vframetest.
 * Copyright (c) 2023-2025 Tuxera Inc.
 */

#ifndef FRAMETEST_TUI_RENDER_H
#define FRAMETEST_TUI_RENDER_H

#include "screen.h"
#include "tui_state.h"
#include "tui.h"

/* Main render function */
void tui_render_screen(tui_app_state_t *state, tui_metrics_t *metrics);

#endif
