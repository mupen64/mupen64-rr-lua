/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <spdlog/spdlog.h>
#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Exposes view functionality that is outside the responsibility of the shared module (e.g. logger, paths).
 */

extern std::shared_ptr<spdlog::logger> g_view_logger;

#ifdef _WIN32
extern HWND g_main_hwnd;
#endif
