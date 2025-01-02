#pragma once
#include <memory>
#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> g_view_logger;

namespace Loggers {
/**
 * Initializes the loggers
 */
void init();
} // namespace Loggers
