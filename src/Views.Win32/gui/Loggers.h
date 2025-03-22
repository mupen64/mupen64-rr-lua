/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

extern std::shared_ptr<spdlog::logger> g_view_logger;
extern std::shared_ptr<spdlog::logger> g_core_logger;

namespace Loggers
{
    /**
     * Initializes the loggers
     */
    void init();
} // namespace Loggers
