#pragma once
#include <spdlog/logger.h>

/*
 *	Exposes an spdlog logger registry.
 *
 *	Must be implemented in the view layer.
 */

/**
 * The logger for the core layer.
 */
extern std::shared_ptr<spdlog::logger> g_core_logger;

/**
 * The logger for the shared layer.
 */
extern std::shared_ptr<spdlog::logger> g_shared_logger;