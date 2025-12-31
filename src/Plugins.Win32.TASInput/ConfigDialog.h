/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace ConfigDialog
{
/**
 * \brief Shows the configuration dialog.
 * \param parent The parent window handle.
 */
void show(HWND parent);

/**
 * \brief Notifies of an SDL event.
 * \param e The SDL event.
 */
void on_sdl_event(const SDL_Event &e);

} // namespace ConfigDialog
