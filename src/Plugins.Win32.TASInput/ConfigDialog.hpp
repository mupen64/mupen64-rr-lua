/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <SDL3/SDL_events.h>

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
