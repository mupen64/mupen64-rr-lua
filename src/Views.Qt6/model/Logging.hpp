/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MODEL_LOGGING_HPP_INCLUDED
#define MODEL_LOGGING_HPP_INCLUDED

#include <spdlog/common.h>
#include <spdlog/logger.h>

namespace Mupen {
  spdlog::logger& core_log();
  spdlog::logger& video_log();
  spdlog::logger& audio_log();
  spdlog::logger& input_log();
  spdlog::logger& rsp_log();
  spdlog::logger& view_log();
}

#endif