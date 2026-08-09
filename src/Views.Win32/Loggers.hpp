/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <memory>

#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> g_view_logger;
extern std::shared_ptr<spdlog::logger> g_core_logger;
extern std::shared_ptr<spdlog::logger> g_video_logger;
extern std::shared_ptr<spdlog::logger> g_audio_logger;
extern std::shared_ptr<spdlog::logger> g_input_logger;
extern std::shared_ptr<spdlog::logger> g_rsp_logger;
extern std::shared_ptr<spdlog::logger> g_ffmpeg_logger;

namespace Loggers
{
/**
 * Initializes the loggers
 */
void init();
} // namespace Loggers
