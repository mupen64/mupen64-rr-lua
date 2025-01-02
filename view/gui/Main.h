/***************************************************************************
                                                  main_win.h  -  description
                                                         -------------------
        copyright            : (C) 2003 by ShadowPrince
        email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Windows.h>
#include <core/r4300/vcr.h>
#include <filesystem>
#include <shared/Config.hpp>
#include <spdlog/logger.h>
#include <string>
#include <view/gui/features/Dispatcher.h>

#ifdef _DEBUG
#define BUILD_TARGET_INFO L"-debug"
#else
#define BUILD_TARGET_INFO L""
#endif

#ifdef UNICODE
#define CHARSET_INFO L""
#else
#define CHARSET_INFO L"-a"
#endif

#ifdef _M_X64
#define ARCH_INFO L"-x64"
#else
#define ARCH_INFO L""
#endif

#define MUPEN_VERSION                                                          \
  L"Mupen 64 1.1.9-6" ARCH_INFO CHARSET_INFO BUILD_TARGET_INFO

#define WM_FOCUS_MAIN_WINDOW (WM_USER + 17)
#define WM_EXECUTE_DISPATCHER (WM_USER + 18)

extern BOOL CALLBACK CfgDlgProc(HWND hwnd, UINT Message, WPARAM wParam,
                                LPARAM lParam);

extern DWORD g_ui_thread_id;

extern int g_last_wheel_delta;

extern HWND g_main_hwnd;
extern HINSTANCE g_app_instance;

extern HWND g_hwnd_plug;
extern DWORD start_rom_id;

extern std::filesystem::path g_app_path;
extern std::shared_ptr<Dispatcher> g_main_window_dispatcher;

/**
 * \brief Whether the statusbar needs to be updated with new input information
 */
extern bool is_primary_statusbar_invalidated;

/**
 * The view ff flag. Combined with other flags to determine the core
 * fast-forward value.
 */
extern bool g_fast_forward;

/**
 * \brief Pauses the emulation during the object's lifetime, resuming it if
 * previously paused upon being destroyed
 */
struct BetterEmulationLock {
private:
  bool was_paused;

public:
  BetterEmulationLock();
  ~BetterEmulationLock();
};

typedef struct {
  long width;
  long height;
  long statusbar_height;
} t_window_info;

extern t_window_info window_info;

t_window_info get_window_info();

/**
 * \brief Gets the string representation of a hotkey
 * \param hotkey The hotkey to convert
 * \return The hotkey as a string
 */
std::wstring hotkey_to_string(const t_hotkey *hotkey);

/**
 * \brief Demands user confirmation for an exit action
 * \return Whether the action is allowed
 * \remarks If the user has chosen to not use modals, this function will return
 * true by default
 */
bool confirm_user_exit();

/**
 * \brief Whether the current execution is on the UI thread
 */
bool is_on_gui_thread();

/**
 * Shows an error dialog for a core result. If the result indicates no error, no
 * work is done. \param result The result to show an error dialog for. \param
 * hwnd The parent window handle for the spawned dialog. If null, the main
 * window is used. \returns Whether the function was able to show an error
 * dialog.
 */
bool show_error_dialog_for_result(CoreResult result, void *hwnd = nullptr);

/**
 * Represents the result of a plugin discovery operation.
 */
typedef struct {
  /**
   * The discovered plugins matching the plugin API surface.
   */
  std::vector<std::unique_ptr<Plugin>> plugins;

  /**
   * Vector of discovered plugins and their results.
   */
  std::vector<std::pair<std::filesystem::path, std::wstring>> results;

} t_plugin_discovery_result;

/**
 * Performs a plugin discovery operation.
 */
t_plugin_discovery_result do_plugin_discovery();
