/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"

extern "C"
{
#include <libavutil/log.h>
}

#include "Loggers.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/wincolor_sink.h>

std::shared_ptr<spdlog::logger> g_core_logger;
std::shared_ptr<spdlog::logger> g_view_logger;
std::shared_ptr<spdlog::logger> g_video_logger;
std::shared_ptr<spdlog::logger> g_audio_logger;
std::shared_ptr<spdlog::logger> g_input_logger;
std::shared_ptr<spdlog::logger> g_rsp_logger;
std::shared_ptr<spdlog::logger> g_ffmpeg_logger;

static std::filesystem::path get_log_path()
{
    return g_main_ctx.app_path / L"logs" / L"mupen.log";
}

static void ffmpeg_log_callback(void *obj, int level, const char *fmt, va_list vl)
{
    if (level < 0)
        return;

    spdlog::level::level_enum spdlog_level = spdlog::level::trace;
    switch (level) {
        case AV_LOG_PANIC:
        case AV_LOG_FATAL:
            spdlog_level = spdlog::level::critical;
            break;
        case AV_LOG_ERROR:
            spdlog_level = spdlog::level::err;
            break;
        case AV_LOG_WARNING:
            spdlog_level = spdlog::level::warn;
            break;
        case AV_LOG_INFO:
            spdlog_level = spdlog::level::info;
            break;
        case AV_LOG_VERBOSE:
            spdlog_level = spdlog::level::debug;
            break;
        case AV_LOG_TRACE:
            spdlog_level = spdlog::level::trace;
            break;
        default:
            break;
    }

    std::string item_name = "???";
    if (obj != nullptr) {
        AVClass* av_class = *(AVClass**) obj;
        if (av_class->item_name != nullptr)
            item_name = av_class->item_name(obj);
        else
            item_name = av_class->class_name;
    }

    int msg_len = vsnprintf(nullptr, 0, fmt, vl);
    if (msg_len < 0)
        return;
    auto msg_buffer = std::make_unique<char[]>(msg_len);
    int write_len = vsnprintf(msg_buffer.get(), msg_len, fmt, vl);
    if (write_len != msg_len)
        return;

    g_ffmpeg_logger->log(spdlog_level, "({}) {}", item_name, msg_buffer.get());
}

void Loggers::init()
{
    const auto log_path = get_log_path();
    HANDLE h_file = CreateFile(log_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (h_file != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER size{};
        GetFileSizeEx(h_file, &size);

        // Clear log file if bigger than 50MB
        if (size.QuadPart > 50ull * 1024ull * 1024ull)
        {
            SetFilePointerEx(h_file, {.QuadPart = 0}, nullptr, FILE_BEGIN);
            SetEndOfFile(h_file);
        }

        CloseHandle(h_file);
    }

#ifdef _DEBUG
    spdlog::sinks_init_list sink_list = {std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string()),
                                         std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>()};
#else
    spdlog::sinks_init_list sink_list = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string()),
    };
#endif

    g_core_logger = std::make_shared<spdlog::logger>("COR", sink_list);
    g_view_logger = std::make_shared<spdlog::logger>("VIW", sink_list);
    g_video_logger = std::make_shared<spdlog::logger>("VID", sink_list);
    g_audio_logger = std::make_shared<spdlog::logger>("AUD", sink_list);
    g_input_logger = std::make_shared<spdlog::logger>("INP", sink_list);
    g_rsp_logger = std::make_shared<spdlog::logger>("RSP", sink_list);
    g_ffmpeg_logger = std::make_shared<spdlog::logger>("FFM", sink_list);

    const auto LOGGERS = {g_core_logger,  g_view_logger, g_video_logger, g_audio_logger,
                          g_input_logger, g_rsp_logger,  g_ffmpeg_logger};

    for (auto &logger : LOGGERS)
    {
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::err);
    }

    // link FFmpeg log callback
    av_log_set_callback(ffmpeg_log_callback);
}
