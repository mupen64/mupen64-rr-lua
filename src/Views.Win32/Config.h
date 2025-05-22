/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <core_types.h>

struct t_hotkey {
    std::wstring identifier{};
    int32_t key{};
    int32_t ctrl{};
    int32_t shift{};
    int32_t alt{};
    int32_t down_cmd{};
    int32_t up_cmd{};
    /**
     * \brief Gets the wstring representation of the hotkey.
     */
    std::wstring to_wstring();
};

struct t_config {

    enum class PresenterType {
        DirectComposition,
        GDI
    };

    enum class EncoderType {
        VFW,
        FFmpeg
    };

    enum class StatusbarLayout {
        Classic,
        Modern,
        ModernWithReadOnly
    };

#pragma region Hotkeys
    t_hotkey fast_forward_hotkey{};
    t_hotkey gs_hotkey{};
    t_hotkey speed_down_hotkey{};
    t_hotkey speed_up_hotkey{};
    t_hotkey speed_reset_hotkey{};
    t_hotkey frame_advance_hotkey{};
    t_hotkey multi_frame_advance_hotkey{};
    t_hotkey multi_frame_advance_inc_hotkey{};
    t_hotkey multi_frame_advance_dec_hotkey{};
    t_hotkey multi_frame_advance_reset_hotkey{};
    t_hotkey pause_hotkey{};
    t_hotkey toggle_read_only_hotkey{};
    t_hotkey toggle_wait_at_movie_end_hotkey{};
    t_hotkey toggle_movie_loop_hotkey{};
    t_hotkey start_movie_playback_hotkey{};
    t_hotkey start_movie_recording_hotkey{};
    t_hotkey stop_movie_hotkey{};
    t_hotkey create_movie_backup_hotkey{};
    t_hotkey take_screenshot_hotkey{};
    t_hotkey play_latest_movie_hotkey{};
    t_hotkey load_latest_script_hotkey{};
    t_hotkey new_lua_hotkey{};
    t_hotkey close_all_lua_hotkey{};
    t_hotkey load_rom_hotkey{};
    t_hotkey close_rom_hotkey{};
    t_hotkey reset_rom_hotkey{};
    t_hotkey load_latest_rom_hotkey{};
    t_hotkey fullscreen_hotkey{};
    t_hotkey settings_hotkey{};
    t_hotkey toggle_statusbar_hotkey{};
    t_hotkey refresh_rombrowser_hotkey{};
    t_hotkey seek_to_frame_hotkey{};
    t_hotkey run_hotkey{};
    t_hotkey piano_roll_hotkey{};
    t_hotkey cheats_hotkey{};
    t_hotkey save_current_hotkey{};
    t_hotkey load_current_hotkey{};
    t_hotkey save_as_hotkey{};
    t_hotkey load_as_hotkey{};
    t_hotkey undo_load_state_hotkey{};
    t_hotkey save_to_slot_1_hotkey{};
    t_hotkey save_to_slot_2_hotkey{};
    t_hotkey save_to_slot_3_hotkey{};
    t_hotkey save_to_slot_4_hotkey{};
    t_hotkey save_to_slot_5_hotkey{};
    t_hotkey save_to_slot_6_hotkey{};
    t_hotkey save_to_slot_7_hotkey{};
    t_hotkey save_to_slot_8_hotkey{};
    t_hotkey save_to_slot_9_hotkey{};
    t_hotkey save_to_slot_10_hotkey{};
    t_hotkey load_from_slot_1_hotkey{};
    t_hotkey load_from_slot_2_hotkey{};
    t_hotkey load_from_slot_3_hotkey{};
    t_hotkey load_from_slot_4_hotkey{};
    t_hotkey load_from_slot_5_hotkey{};
    t_hotkey load_from_slot_6_hotkey{};
    t_hotkey load_from_slot_7_hotkey{};
    t_hotkey load_from_slot_8_hotkey{};
    t_hotkey load_from_slot_9_hotkey{};
    t_hotkey load_from_slot_10_hotkey{};
    t_hotkey select_slot_1_hotkey{};
    t_hotkey select_slot_2_hotkey{};
    t_hotkey select_slot_3_hotkey{};
    t_hotkey select_slot_4_hotkey{};
    t_hotkey select_slot_5_hotkey{};
    t_hotkey select_slot_6_hotkey{};
    t_hotkey select_slot_7_hotkey{};
    t_hotkey select_slot_8_hotkey{};
    t_hotkey select_slot_9_hotkey{};
    t_hotkey select_slot_10_hotkey{};

#pragma endregion

    /// <summary>
    /// The core config.
    /// </summary>
    core_cfg core{};

    /// <summary>
    /// The new version of Mupen64 currently ignored by the update checker.
    /// <para></para>
    /// L"" means no ignored version.
    /// </summary>
    std::wstring ignored_version;

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

    /// <summary>
    /// Whether plugins discovery is performed asynchronously. Removes potential waiting times in the config dialog.
    /// </summary>
    int32_t plugin_discovery_async = 1;

    /// <summary>
    /// Whether the default plugins directory will be used (otherwise, falls back to <see cref="plugins_directory"/>)
    /// </summary>
    int32_t is_default_plugins_directory_used = 1;

    /// <summary>
    /// Whether the default save directory will be used (otherwise, falls back to <see cref="saves_directory"/>)
    /// </summary>
    int32_t is_default_saves_directory_used = 1;

    /// <summary>
    /// Whether the default screenshot directory will be used (otherwise, falls back to <see cref="screenshots_directory"/>)
    /// </summary>
    int32_t is_default_screenshots_directory_used = 1;

    /// <summary>
    /// Whether the default backups directory will be used (otherwise, falls back to <see cref="backups_directory"/>)
    /// </summary>
    int32_t is_default_backups_directory_used = 1;

    /// <summary>
    /// The plugin directory
    /// </summary>
    std::wstring plugins_directory;

    /// <summary>
    /// The save directory
    /// </summary>
    std::wstring saves_directory;

    /// <summary>
    /// The screenshot directory
    /// </summary>
    std::wstring screenshots_directory;

    /// <summary>
    /// The savestate directory
    /// </summary>
    std::wstring states_path;

    /// <summary>
    /// The movie backups directory
    /// </summary>
    std::wstring backups_directory;

    /// <summary>
    /// The recently opened roms' paths
    /// </summary>
    std::vector<std::wstring> recent_rom_paths;

    /// <summary>
    /// Whether recently opened rom path collection is paused
    /// </summary>
    int32_t is_recent_rom_paths_frozen;

    /// <summary>
    /// The recently opened movies' paths
    /// </summary>
    std::vector<std::wstring> recent_movie_paths;

    /// <summary>
    /// Whether recently opened movie path collection is paused
    /// </summary>
    int32_t is_recent_movie_paths_frozen;

    /// <summary>
    /// Whether the rom browser will recursively search for roms beginning in the specified directories
    /// </summary>
    int32_t is_rombrowser_recursion_enabled;

    /// <summary>
    /// The paths to directories which are searched for roms
    /// </summary>
    std::vector<std::wstring> rombrowser_rom_paths;

    /// <summary>
    /// The strategy to use when capturing video
    /// <para/>
    /// 0 = Use the video plugin's readScreen or read_video (MGE)
    /// 1 = Internal capture of window
    /// 2 = Internal capture of screen cropped to window
    /// 3 = Use the video plugin's readScreen or read_video (MGE) composited with lua scripts
    /// </summary>
    int32_t capture_mode = 3;

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
    int32_t encoder_type = (int32_t)EncoderType::VFW;

    /// <summary>
    /// The delay (in milliseconds) before capturing the window
    /// <para/>
    /// May be useful when capturing other windows alongside mupen
    /// </summary>
    int32_t capture_delay;

    /// <summary>
    /// FFmpeg post-stream option format string which is used when capturing using the FFmpeg encoder type
    /// </summary>
    std::wstring ffmpeg_final_options =
    L"-y -f rawvideo -pixel_format bgr24 -video_size %dx%d -framerate %d -i %s "
    L"-f s16le -sample_rate %d -ac 2 -channel_layout stereo -i %s "
    L"-c:v libx264 -preset veryfast -tune zerolatency -crf 23 -c:a aac -b:a 128k -vf \"vflip\" -f mp4 %s";

    /// <summary>
    /// FFmpeg binary path
    /// </summary>
    std::wstring ffmpeg_path = L"C:\\ffmpeg\\bin\\ffmpeg.exe";

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
    /// Whether a low-latency dispatcher implementation is used. Greatly improves performance when Lua scripts are running. Disable if you DirectInput-based plugins aren't working as expected.
    /// </summary>
    int32_t fast_dispatcher = 1;

    /// <summary>
    /// Whether the plugin discovery process is artificially lengthened.
    /// </summary>
    int32_t plugin_discovery_delayed;

    /// <summary>
    /// The lua script path
    /// </summary>
    std::wstring lua_script_path;

    /// <summary>
    /// The recently opened lua scripts' paths
    /// </summary>
    std::vector<std::wstring> recent_lua_script_paths;

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
    std::wstring selected_video_plugin;

    /// <summary>
    /// The path of the currently selected audio plugin
    /// </summary>
    std::wstring selected_audio_plugin;

    /// <summary>
    /// The path of the currently selected input plugin
    /// </summary>
    std::wstring selected_input_plugin;

    /// <summary>
    /// The path of the currently selected RSP plugin
    /// </summary>
    std::wstring selected_rsp_plugin;

    /// <summary>
    /// The last known value of the record movie dialog's "start type" field
    /// </summary>
    int32_t last_movie_type = 1; // (MOVIE_START_FROM_SNAPSHOT)

    /// <summary>
    /// The last known value of the record movie dialog's "author" field
    /// </summary>
    std::wstring last_movie_author;

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
    std::map<std::wstring, std::wstring> persistent_folder_paths;

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
    std::wstring seeker_value;

    /// <summary>
    /// The multi-frame advance index.
    /// </summary>
    int32_t multi_frame_advance_count = 2;

    /// <summary>
    /// A map of dialog IDs to their default choices for silent mode.
    /// </summary>
    std::map<std::wstring, std::wstring> silent_mode_dialog_choices;
};

extern t_config g_config;
extern const t_config g_default_config;
extern std::vector<t_hotkey*> g_config_hotkeys;

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
     * \brief Restores the config state from the config file.
     */
    void load();
} // namespace Config
