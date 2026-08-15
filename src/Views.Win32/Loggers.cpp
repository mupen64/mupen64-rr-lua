/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "Loggers.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/wincolor_sink.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

std::shared_ptr<spdlog::logger> g_core_logger;
std::shared_ptr<spdlog::logger> g_view_logger;
std::shared_ptr<spdlog::logger> g_video_logger;
std::shared_ptr<spdlog::logger> g_audio_logger;
std::shared_ptr<spdlog::logger> g_input_logger;
std::shared_ptr<spdlog::logger> g_rsp_logger;

static constexpr std::size_t MAX_LOG_FILES = 20;

static std::filesystem::path get_log_path()
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    std::tm local_time{};
    localtime_s(&local_time, &time);

    std::ostringstream filename;
    filename << "mupen-" << std::put_time(&local_time, "%Y%m%d-%H%M%S") << "-" << std::setfill('0')
             << std::setw(3) << milliseconds << ".log";

    return IOUtils::exe_path().parent_path() / "logs" / filename.str();
}

static void remove_old_logs(const std::filesystem::path &log_directory)
{
    std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> logs;
    std::error_code error;

    for (const auto &entry : std::filesystem::directory_iterator(log_directory, error))
    {
        if (error)
            break;

        const auto filename = entry.path().filename().string();
        if (!entry.is_regular_file() || entry.path().extension() != ".log" || filename.rfind("mupen-", 0) != 0)
            continue;

        const auto last_write_time = entry.last_write_time(error);
        if (!error)
            logs.emplace_back(last_write_time, entry.path());
    }

    std::sort(logs.begin(), logs.end(),
              [](const auto &left, const auto &right) { return left.first > right.first; });

    const auto logs_to_keep = MAX_LOG_FILES > 0 ? MAX_LOG_FILES - 1 : 0;
    for (std::size_t index = logs_to_keep; index < logs.size(); ++index)
        std::filesystem::remove(logs[index].second, error);
}

void Loggers::init()
{
    const auto log_path = get_log_path();
    std::filesystem::create_directories(log_path.parent_path());
    remove_old_logs(log_path.parent_path());

#ifdef _DEBUG
    spdlog::sinks_init_list sink_list = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string()),
        std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(),
    };
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

    const auto LOGGERS = {
        g_core_logger, g_view_logger, g_video_logger, g_audio_logger, g_input_logger, g_rsp_logger,
    };

    for (auto &logger : LOGGERS)
    {
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::err);
    }
}
