/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <m64rr/Types.hpp>
#include <Common.Views/Hotkey.hpp>
#include <map>

struct t_config
{
    /**
     * \brief Synchronization modes the capture manager can abide by
     */
    enum class Sync : int
    {
        /**
         * \brief Video and Audio streams are not kept in sync
         */
        None,

        /**
         * \brief The audio stream dictates the video stream's rate
         */
        Audio,

        /**
         * \brief The video stream dictates the audio stream's rate
         */
        Video,
    };

    enum class PresenterType
    {
        DirectComposition,
        GDI
    };

    enum class EncoderType
    {
        VFW,
        FFmpeg
    };

    enum class StatusbarLayout
    {
        Classic,
        Modern,
        ModernWithReadOnly
    };

    /**
     * \brief Describes how toasts are shown.
     */
    enum class ToastMode : uint8_t
    {
        // Toasts are shown in non-modal windows.
        Window,
        // Toasts are shown in the statusbar.
        Statusbar,
        // Toasts are shown in modal dialogs.
        Dialog
    };

    /// <summary>
    /// The core config.
    /// </summary>
    core_cfg core{};

    /// <summary>
    /// The new version of Mupen64 currently ignored by the update checker.
    /// <para></para>
    /// "" means no ignored version.
    /// </summary>
    std::string ignored_version;

    /// <summary>
    /// The UI theme to use. 0 = Light, 1 = Dark, 2 = System Default.
    /// </summary>
    int32_t theme = 2;

    /// <summary>
    /// The current savestate slot index (0-9).
    /// </summary>
    int32_t st_slot;

    /// <summary>
    /// Whether emulation will pause when the main window loses focus
    /// </summary>
    int32_t is_unfocused_pause_enabled;

    /// <summary>
    /// Whether the statusbar is enabled
    /// </summary>
    int32_t is_statusbar_enabled = 1;

    /// <summary>
    /// Whether the statusbar is allowed to scale its segments down.
    /// </summary>
    int32_t statusbar_scale_down = 1;

    /// <summary>
    /// Whether the statusbar is allowed to scale its segments up.
    /// </summary>
    int32_t statusbar_scale_up;

    /// <summary>
    /// The statusbar layout.
    /// </summary>
    int32_t statusbar_layout = (int32_t)StatusbarLayout::Modern;

    std::string rom_directory = ".\\roms\\";
    std::string plugins_directory = ".\\plugin\\";
    std::string saves_directory = ".\\save\\";
    std::string screenshots_directory = ".\\screenshots\\";
    std::string backups_directory = ".\\backups\\";

    /// <summary>
    /// The recently opened roms' paths
    /// </summary>
    std::vector<std::string> recent_rom_paths;

    /// <summary>
    /// Whether recently opened rom path collection is paused
    /// </summary>
    int32_t is_recent_rom_paths_frozen;

    /// <summary>
    /// The recently opened movies' paths
    /// </summary>
    std::vector<std::string> recent_movie_paths;

    /// <summary>
    /// Whether recently opened movie path collection is paused
    /// </summary>
    int32_t is_recent_movie_paths_frozen;

    /// <summary>
    /// Whether the rom browser will recursively search for roms beginning in the specified directories
    /// </summary>
    int32_t is_rombrowser_recursion_enabled;

    /// <summary>
    /// The strategy to use when capturing video
    /// <para/>
    /// 0 = Use the video plugin's readScreen or read_video (MGE)
    /// 1 = Internal capture of window
    /// 2 = Internal capture of screen cropped to window
    /// 3 = Use the video plugin's readScreen or read_video (MGE) composited with lua scripts
    /// </summary>
    int32_t capture_mode = 3;

    int32_t stop_capture_at_movie_end;

    /// <summary>
    /// The presenter to use for Lua scripts
    /// </summary>
    int32_t presenter_type = (int32_t)PresenterType::DirectComposition;

    /// <summary>
    /// Enables lazy Lua renderer initialization. Greatly speeds up start and stop times for certain scripts.
    /// </summary>
    int32_t lazy_renderer_init = 1;

    /// <summary>
    /// The encoder to use for capturing.
    /// </summary>
    int32_t encoder_type = (int32_t)EncoderType::FFmpeg;

    /// <summary>
    /// The delay (in milliseconds) before capturing the window
    /// <para/>
    /// May be useful when capturing other windows alongside mupen
    /// </summary>
    int32_t capture_delay;

    /// <summary>
    /// FFmpeg options.
    /// </summary>
    std::string ffmpeg_options = "-c:v libx264 -preset veryfast -crf 23 -c:a aac -b:a 128k -vf \"vflip,format=yuv420p\"";

    /// <summary>
    /// FFmpeg binary path
    /// </summary>
    std::string ffmpeg_path = "C:\\ffmpeg\\bin\\ffmpeg.exe";

    /// <summary>
    /// The audio-video synchronization mode
    /// <para/>
    /// 0 - No Sync
    /// 1 - Audio Sync
    /// 2 - Video Sync
    /// </summary>
    int32_t synchronization_mode = 1;

    /// <summary>
    /// When enabled, mupen won't change the working directory to its current path at startup
    /// </summary>
    int32_t keep_default_working_directory;

    /// <summary>
    /// The lua script path
    /// </summary>
    std::string lua_script_path;

    /// <summary>
    /// The recently opened lua scripts' paths
    /// </summary>
    std::vector<std::string> recent_lua_script_paths;

    /// <summary>
    /// Whether recently opened lua script path collection is paused
    /// </summary>
    int32_t is_recent_scripts_frozen;

    /// <summary>
    /// Whether piano roll edits are constrained to the column they started on
    /// </summary>
    int32_t piano_roll_constrain_edit_to_column;

    /// <summary>
    /// Maximum size of the undo/redo stack.
    /// </summary>
    int32_t piano_roll_undo_stack_size = 100;

    /// <summary>
    /// Whether the piano roll will try to keep the selection visible when the frame changes
    /// </summary>
    int32_t piano_roll_keep_selection_visible;

    /// <summary>
    /// Whether the piano roll will try to keep the playhead visible when the frame changes
    /// </summary>
    int32_t piano_roll_keep_playhead_visible;

    /// <summary>
    /// The path of the currently selected video plugin
    /// </summary>
    std::string selected_video_plugin;

    /// <summary>
    /// The path of the currently selected audio plugin
    /// </summary>
    std::string selected_audio_plugin;

    /// <summary>
    /// The path of the currently selected input plugin
    /// </summary>
    std::string selected_input_plugin;

    /// <summary>
    /// The path of the currently selected RSP plugin
    /// </summary>
    std::string selected_rsp_plugin;

    /// <summary>
    /// The last known value of the record movie dialog's "start type" field
    /// </summary>
    int32_t last_movie_type = 1; // (MOVIE_START_FROM_SNAPSHOT)

    /// <summary>
    /// The last known value of the record movie dialog's "author" field
    /// </summary>
    std::string last_movie_author;

    /// <summary>
    /// The main window's X position
    /// </summary>
    int32_t window_x = ((int)0x80000000);

    /// <summary>
    /// The main window's Y position
    /// </summary>
    int32_t window_y = ((int)0x80000000);

    /// <summary>
    /// The main window's width
    /// </summary>
    int32_t window_width = 640;

    /// <summary>
    /// The main window's height
    /// </summary>
    int32_t window_height = 480;

    // The mode in which toasts are displayed.
    int32_t toast_mode = (int32_t)ToastMode::Window;

    /// <summary>
    /// The width of rombrowser columns by index
    /// </summary>
    std::vector<std::int32_t> rombrowser_column_widths = {24, 240, 240, 120};

    /// <summary>
    /// The index of the currently sorted column, or -1 if none is sorted
    /// </summary>
    int32_t rombrowser_sorted_column;

    /// <summary>
    /// Whether the selected column is sorted in an ascending order
    /// </summary>
    int32_t rombrowser_sort_ascending = 1;

    /// <summary>
    /// A map of persistent path dialog IDs and the respective value
    /// </summary>
    std::map<std::string, std::string> persistent_folder_paths;

    /// <summary>
    /// The last selected settings tab's index.
    /// </summary>
    int32_t settings_tab;

    /// <summary>
    /// Whether VCR displays frame information relative to frame 0, not 1
    /// </summary>
    int32_t vcr_0_index;

    /// <summary>
    /// Increments the current slot when saving savestate to slot
    /// </summary>
    int32_t increment_slot;

    /// <summary>
    /// Whether automatic update checking is enabled.
    /// </summary>
    int32_t automatic_update_checking = 1;

    /// <summary>
    /// Whether mupen will avoid showing modals and other elements which require user interaction
    /// </summary>
    int32_t silent_mode = 0;

    /// <summary>
    /// The current seeker input value
    /// </summary>
    std::string seeker_value;

    /// <summary>
    /// The multi-frame advance index.
    /// </summary>
    int32_t multi_frame_advance_count = -1;

    /// <summary>
    /// A map of dialog IDs to their default choices for silent mode.
    /// </summary>
    std::map<std::string, std::string> silent_mode_dialog_choices;

    /// <summary>
    /// A map of trusted Lua script paths. If a Lua script path is present in this map, it will be trusted.
    /// </summary>
    std::map<std::string, std::string> trusted_lua_paths;

    /// <summary>
    /// The Lua Dialog's saved paths.
    /// </summary>
    std::vector<std::string> lua_paths;

    /// <summary>
    /// A map of fully-qualified action paths to a hotkey assigned to them.
    /// </summary>
    std::map<std::string, Hotkey> hotkeys;

    /// <summary>
    /// A map of fully-qualified action paths to the hotkey which was assigned to them the first time the action was
    /// assigned a hotkey.
    /// </summary>
    std::map<std::string, Hotkey> inital_hotkeys;

    bool operator==(const t_config &) const = default;
};

extern t_config g_config;

namespace Config
{

/**
 * \brief Initializes the subsystem.
 */
void init();

/**
 * \brief Saves the current config state to the config file.
 */
void save();

/**
 * \brief Applies the current config state and saves it to the config file.
 */
void apply_and_save();

/**
 * \brief Restores the config state from the config file.
 */
void load();

/**
 * \brief Gets the default config.
 */
const t_config &default_config();

/**
 * \brief Gets the path to the ROM directory based on the current configuration.
 */
std::filesystem::path rom_directory();

/**
 * \brief Gets the path to the plugin directory based on the current configuration.
 */
std::filesystem::path plugin_directory();

/**
 * \brief Gets the path to the save directory based on the current configuration.
 */
std::filesystem::path save_directory();

/**
 * \brief Gets the path to the screenshot directory based on the current configuration.
 */
std::filesystem::path screenshot_directory();

/**
 * \brief Gets the path to the backup directory based on the current configuration.
 */
std::filesystem::path backup_directory();

/**
 * \brief Gets the path to the logs directory. This is not based on the current configuration, but it's here for
 * convenience.
 */
std::filesystem::path logs_directory();

} // namespace Config
