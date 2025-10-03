/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ViewPlugin.h>

struct plugin_funcs
{
    core_plugin_extended_funcs video_extended_funcs;
    ROMOPEN video_rom_open;
    ROMCLOSED video_rom_closed;
    CLOSEDLL video_close_dll;
    PROCESSDLIST video_process_dlist;
    PROCESSRDPLIST video_process_rdp_list;
    SHOWCFB video_show_cfb;
    VISTATUSCHANGED video_vi_status_changed;
    VIWIDTHCHANGED video_vi_width_changed;
    GETVIDEOSIZE video_get_video_size;
    FBREAD video_fb_read;
    FBWRITE video_fb_write;
    FBGETFRAMEBUFFERINFO video_fb_get_frame_buffer_info;
    CHANGEWINDOW video_change_window;
    UPDATESCREEN video_update_screen;
    READSCREEN video_read_screen;
    DLLCRTFREE video_dll_crt_free;
    MOVESCREEN video_move_screen;
    CAPTURESCREEN video_capture_screen;
    READVIDEO video_read_video;

    core_plugin_extended_funcs audio_extended_funcs;
    ROMOPEN audio_rom_open;
    ROMCLOSED audio_rom_closed;
    CLOSEDLL audio_close_dll_audio;
    AIDACRATECHANGED audio_ai_dacrate_changed;
    AILENCHANGED audio_ai_len_changed;
    AIREADLENGTH audio_ai_read_length;
    PROCESSALIST audio_process_alist;
    AIUPDATE audio_ai_update;
    CLOSEDLL input_close_dll;
    ROMCLOSED input_rom_closed;
    ROMOPEN input_rom_open;

    core_plugin_extended_funcs input_extended_funcs;
    CONTROLLERCOMMAND input_controller_command;
    GETKEYS input_get_keys;
    SETKEYS input_set_keys;
    READCONTROLLER input_read_controller;
    KEYDOWN input_key_down;
    KEYUP input_key_up;

    core_plugin_extended_funcs rsp_extended_funcs;
    CLOSEDLL rsp_close_dll;
    ROMCLOSED rsp_rom_closed;
    DORSPCYCLES rsp_do_rsp_cycles;
};

class Plugin
{
  public:
    /**
     * \brief Tries to create a plugin from the given path
     * \param path The path to a plugin
     * \return The operation status along with a pointer to the plugin. The pointer will be invalid if the first pair
     * element isn't an empty string.
     */
    static std::pair<std::wstring, std::unique_ptr<Plugin>> create(std::filesystem::path path);

    Plugin() = default;
    ~Plugin();

    /**
     * \brief Opens the plugin configuration dialog.
     * \param hwnd The parent window handle.
     */
    void config(HWND hwnd);

    /**
     * \brief Opens the plugin test dialog
     * \param hwnd The parent window handle.
     */
    void test(HWND hwnd);

    /**
     * \brief Opens the plugin about dialog
     * \param hwnd The parent window handle.
     */
    void about(HWND hwnd);

    /**
     * \brief Loads the plugin's exported functions into the globals and calls the initiate function.
     */
    void initiate();

    /**
     * \brief Gets the plugin's path
     */
    auto path() const { return m_path; }

    /**
     * \brief Gets the plugin's name
     */
    auto name() const { return m_name; }

    /**
     * \brief Gets the plugin's type
     */
    auto type() const { return m_type; }

    /**
     * \brief Gets the plugin's version
     */
    auto version() const { return m_version; }

  private:
    std::filesystem::path m_path;
    std::string m_name;
    core_plugin_type m_type;
    uint16_t m_version;
    HMODULE m_module;
};

/**
 * Represents the result of a plugin discovery operation.
 */
typedef struct
{
    /**
     * The discovered plugins matching the plugin API surface.
     */
    std::vector<std::unique_ptr<Plugin>> plugins;

    /**
     * Vector of discovered plugins and their results.
     */
    std::vector<std::pair<std::filesystem::path, std::wstring>> results;

} t_plugin_discovery_result;

extern plugin_funcs g_plugin_funcs;

/// <summary>
/// Initializes dummy info used by per-plugin functions
/// </summary>
void setup_dummy_info();

/**
 * \brief A module providing utility functions related to plugins.
 */
namespace PluginUtil
{
/**
 * \brief Discovers plugins in the given directory.
 * \param directory The directory to search for plugins in.
 * \return The plugin discovery result.
 */
t_plugin_discovery_result discover_plugins(const std::filesystem::path &directory);

/**
 * \brief Gets the extended function set for video plugins.
 */
core_plugin_extended_funcs video_extended_funcs();
/**
 * \brief Gets the extended function set for audio plugins.
 */
core_plugin_extended_funcs audio_extended_funcs();
/**
 * \brief Gets the extended function set for input plugins.
 */
core_plugin_extended_funcs input_extended_funcs();
/**
 * \brief Gets the extended function set for RSP plugins.
 */
core_plugin_extended_funcs rsp_extended_funcs();

/**
 * \return Whether MGE functionality is currently available.
 */
bool mge_available();

/**
 * \brief Prepares and starts the currently loaded plugins to be used by the core.
 */
void start_plugins();

/**
 * \brief Stops and unloads the currently loaded plugins.
 */
void stop_plugins();

} // namespace PluginUtil
