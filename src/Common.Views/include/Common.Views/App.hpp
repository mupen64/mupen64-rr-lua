/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <Common.Views/IDialogService.hpp>
#include <Common.Views/Hotkey.hpp>
#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Exposes view functionality that is outside the responsibility of the shared module (e.g. logger, paths).
 */

extern std::shared_ptr<spdlog::logger> g_view_logger;
extern IDialogService *g_dialog_service;

#ifdef _WIN32
extern HWND g_main_hwnd;
#endif

/**
 * \brief Called when a runtime assertion fails.
 * \param message The assertion message.
 */
void app_runtime_assert_fail(std::string_view message);

/**
 * \return A map of default dialog choices for silent mode.
 */
std::unordered_map<std::string, size_t> get_silent_mode_dialog_choices();

// Temporary shim for 1.4.0-x -> 1.5.0 hotkey conversion
std::optional<Hotkey> app_json_to_hotkey(const nlohmann::basic_json<> &hotkey_json);
