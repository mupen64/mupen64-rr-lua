/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "Loggers.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/wincolor_sink.h>

std::shared_ptr<spdlog::logger> g_core_logger;
std::shared_ptr<spdlog::logger> g_view_logger;
std::shared_ptr<spdlog::logger> g_video_logger;
std::shared_ptr<spdlog::logger> g_audio_logger;
std::shared_ptr<spdlog::logger> g_input_logger;
std::shared_ptr<spdlog::logger> g_rsp_logger;

void Loggers::init()
{
    HANDLE h_file = CreateFile(L"mupen.log", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

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
    spdlog::sinks_init_list sink_list = {
    std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log"),
    std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>()};
#else
    spdlog::sinks_init_list sink_list = {
    std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log"),
    };
#endif

    g_core_logger = std::make_shared<spdlog::logger>("COR", sink_list);
    g_view_logger = std::make_shared<spdlog::logger>("VIW", sink_list);
    g_video_logger = std::make_shared<spdlog::logger>("VID", sink_list);
    g_audio_logger = std::make_shared<spdlog::logger>("AUD", sink_list);
    g_input_logger = std::make_shared<spdlog::logger>("INP", sink_list);
    g_rsp_logger = std::make_shared<spdlog::logger>("RSP", sink_list);

    const auto LOGGERS = {
    g_core_logger,
    g_view_logger,
    g_video_logger,
    g_audio_logger,
    g_input_logger,
    g_rsp_logger,
    };

    for (auto& logger : LOGGERS)
    {
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::err);
    }
}
