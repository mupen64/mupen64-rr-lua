/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <components/Dispatcher.h>

#define CURRENT_VERSION L"1.3.0-7"

#define WM_FOCUS_MAIN_WINDOW (WM_USER + 17)
#define WM_EXECUTE_DISPATCHER (WM_USER + 18)
#define WM_INVALIDATE_LUA (WM_USER + 23)

#define VIEW_DLG_MOVIE_OVERWRITE_WARNING "VIEW_DLG_MOVIE_OVERWRITE_WARNING"
#define VIEW_DLG_RESET_SETTINGS "VIEW_DLG_RESET_SETTINGS"
#define VIEW_DLG_RESET_PLUGIN_SETTINGS "VIEW_DLG_RESET_PLUGIN_SETTINGS"
#define VIEW_DLG_LAG_EXCEEDED "VIEW_DLG_LAG_EXCEEDED"
#define VIEW_DLG_CLOSE_ROM_WARNING "VIEW_DLG_CLOSE_ROM_WARNING"
#define VIEW_DLG_HOTKEY_CONFLICT "VIEW_DLG_HOTKEY_CONFLICT"
#define VIEW_DLG_UPDATE_DIALOG "VIEW_DLG_UPDATE_DIALOG"
#define VIEW_DLG_PLUGIN_LOAD_ERROR "VIEW_DLG_PLUGIN_LOAD_ERROR"
#define VIEW_DLG_RAMSTART "VIEW_DLG_RAMSTART"
#define VIEW_DLG_ABOUT "VIEW_DLG_ABOUT"

struct t_main_context
{
    core_params core{};
    core_ctx *core_ctx{};
    // PlatformService io_service{};
    bool frame_changed{};
    int last_wheel_delta{};
    HWND hwnd{};
    HINSTANCE hinst{};
    std::shared_ptr<Dispatcher> dispatcher{};
    bool paused_before_menu{};
    bool in_menu_loop{};
    bool fullscreen{};
    bool fast_forward{};
    std::filesystem::path app_path{};
};

/**
 * \brief Pauses the emulation during the object's lifetime, resuming it if previously paused upon being destroyed
 */
struct BetterEmulationLock
{
  private:
    bool was_paused;

  public:
    BetterEmulationLock();
    ~BetterEmulationLock();
};

struct t_window_info
{
    long width;
    long height;
    long statusbar_height;
};

extern t_main_context g_main_ctx;

static bool task_is_playback(const core_vcr_task task)
{
    return task == task_playback || task == task_start_playback_from_reset || task == task_start_playback_from_snapshot;
}

static bool vcr_is_task_recording(const core_vcr_task task)
{
    return task == task_recording || task == task_start_recording_from_reset ||
           task == task_start_recording_from_snapshot;
}

/**
 * \return The friendly name of the emulator.
 * \param simple If true, returns a simplified name with only the app name and version number.
 */
std::wstring get_mupen_name(bool simple = false);

/**
 * \return Information about the current window size.
 */
t_window_info get_window_info();

/**
 * \brief Demands user confirmation for an exit action
 * \return Whether the action is allowed
 * \remarks If the user has chosen to not use modals, this function will return true by default
 */
bool confirm_user_exit();

/**
 * \brief Whether the current execution is on the UI thread
 */
bool is_on_gui_thread();

/**
 * Shows an error dialog for a core result. If the result indicates no error, no work is done.
 * \param result The result to show an error dialog for.
 * \param hwnd The parent window handle for the spawned dialog. If null, the main window is used.
 * \returns Whether the function was able to show an error dialog.
 */
bool show_error_dialog_for_result(core_result result, void *hwnd = nullptr);

/**
 * \brief Sets the current working directory to the application path.
 */
void set_cwd();

/**
 * \brief Gets the path for a savestate with the given slot number.
 * \param slot The savestate slot number.
 * \return The path to the savestate file.
 */
std::filesystem::path get_st_with_slot_path(size_t slot);
