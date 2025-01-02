#include "Loggers.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/wincolor_sink.h>
#include <vector>

#include "shared/services/LoggingService.h"

std::shared_ptr<spdlog::logger> g_core_logger;
std::shared_ptr<spdlog::logger> g_shared_logger;
std::shared_ptr<spdlog::logger> g_view_logger;

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
    std::vector<spdlog::sink_ptr> core_sinks = {std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(), std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log")};
    std::vector<spdlog::sink_ptr> shared_sinks = {std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(), std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log")};
    std::vector<spdlog::sink_ptr> view_sinks = {std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(), std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log")};
    g_core_logger = std::make_shared<spdlog::logger>("Core", begin(core_sinks), end(core_sinks));
    g_shared_logger = std::make_shared<spdlog::logger>("Shared", begin(shared_sinks), end(shared_sinks));
    g_view_logger = std::make_shared<spdlog::logger>("View", begin(view_sinks), end(view_sinks));
#else
    g_core_logger = spdlog::basic_logger_mt("Core", "mupen.log");
    g_shared_logger = spdlog::basic_logger_mt("Shared", "mupen.log");
    g_view_logger = spdlog::basic_logger_mt("View", "mupen.log");
#endif
    g_core_logger->set_level(spdlog::level::trace);
    g_shared_logger->set_level(spdlog::level::trace);
    g_view_logger->set_level(spdlog::level::trace);
    g_core_logger->flush_on(spdlog::level::err);
    g_shared_logger->flush_on(spdlog::level::err);
    g_view_logger->flush_on(spdlog::level::err);
}
