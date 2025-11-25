/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Logging.hpp"
#include <boost/dll.hpp>
#include <spdlog/logger.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/ansicolor_sink.h>

namespace
{
struct CoreLoggers
{
    spdlog::logger core;
    spdlog::logger video;
    spdlog::logger audio;
    spdlog::logger input;
    spdlog::logger rsp;
    spdlog::logger view;
};
} // namespace

namespace Mupen
{
static const spdlog::sinks_init_list &sink_init_list()
{
    static spdlog::sinks_init_list sinks{
        std::make_shared<spdlog::sinks::basic_file_sink_mt>([]() {
            auto file_path = boost::dll::program_location().parent_path() / "mupen.log";
            return file_path.string();
        }()),
#ifdef _DEBUG
        // ANSI escape codes work on recent Windows
        std::make_shared<spdlog::sinks::ansicolor_stderr_sink_mt>(),
#endif
    };
    return sinks;
}

static CoreLoggers &core_loggers_all()
{
    static CoreLoggers loggers = []() {
        CoreLoggers result = CoreLoggers{
            .core = spdlog::logger("core ", sink_init_list()),
            .video = spdlog::logger("video", sink_init_list()),
            .audio = spdlog::logger("audio", sink_init_list()),
            .input = spdlog::logger("input", sink_init_list()),
            .rsp = spdlog::logger("rsp  ", sink_init_list()),
            .view = spdlog::logger("view ", sink_init_list()),
        };
        spdlog::cfg::load_env_levels();
        return result;
    }();
    return loggers;
}

spdlog::logger &core_log()
{
    return core_loggers_all().core;
}
spdlog::logger &video_log()
{
    return core_loggers_all().video;
}
spdlog::logger &audio_log()
{
    return core_loggers_all().audio;
}
spdlog::logger &input_log()
{
    return core_loggers_all().input;
}
spdlog::logger &rsp_log()
{
    return core_loggers_all().rsp;
}
spdlog::logger &view_log()
{
    return core_loggers_all().view;
}
} // namespace Mupen