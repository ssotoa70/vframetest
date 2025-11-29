/*
 * tui_input.h - Keyboard input handling for interactive TUI
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

#ifndef FRAMETEST_TUI_INPUT_H
#define FRAMETEST_TUI_INPUT_H

#include "tui_state.h"

/*
 * Process a single key press and update state accordingly
 * Returns 1 if the key was handled, 0 if ignored
 */
int tui_input_handle_key(tui_app_state_t *state, int key);

/*
 * Process input with timeout
 * Reads keys and processes them, blocking up to timeout_ms
 * Returns 1 if quit was requested, 0 otherwise
 */
int tui_input_process(tui_app_state_t *state, int timeout_ms);

#endif /* FRAMETEST_TUI_INPUT_H */
