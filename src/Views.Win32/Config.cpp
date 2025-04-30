/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <DialogService.h>
#include <Messenger.h>

#include <ini.h>
#include <Loggers.h>

cfg_view g_config;
std::vector<cfg_hotkey*> g_config_hotkeys;

#ifdef _M_X64
#define CONFIG_FILE_NAME L"config-x64.ini"
#else
#define CONFIG_FILE_NAME L"config.ini"
#endif

constexpr auto FLAT_FIELD_KEY = "config";

const std::unordered_map<std::string, size_t> DIALOG_SILENT_MODE_CHOICES = {
{CORE_DLG_FLOAT_EXCEPTION, 0},
{CORE_DLG_ST_HASH_MISMATCH, 0},
{CORE_DLG_ST_UNFREEZE_WARNING, 0},
{CORE_DLG_ST_NOT_FROM_MOVIE, 0},
{CORE_DLG_VCR_RAWDATA_WARNING, 0},
{CORE_DLG_VCR_WIIVC_WARNING, 0},
{CORE_DLG_VCR_ROM_NAME_WARNING, 0},
{CORE_DLG_VCR_ROM_CCODE_WARNING, 0},
{CORE_DLG_VCR_ROM_CRC_WARNING, 0},
{CORE_DLG_VCR_CHEAT_LOAD_ERROR, 0},

{VIEW_DLG_MOVIE_OVERWRITE_WARNING, 0},
{VIEW_DLG_RESET_SETTINGS, 0},
{VIEW_DLG_RESET_PLUGIN_SETTINGS, 0},
{VIEW_DLG_LAG_EXCEEDED, 0},
{VIEW_DLG_CLOSE_ROM_WARNING, 0},
{VIEW_DLG_HOTKEY_CONFLICT, 0},
{VIEW_DLG_UPDATE_DIALOG, 2},
{VIEW_DLG_PLUGIN_LOAD_ERROR, 0},
{VIEW_DLG_RAMSTART, 0},
};

cfg_view get_default_config()
{
    cfg_view config = {};

    config.fast_forward_hotkey = {
    .identifier = L"Fast-forward",
    .key = VK_TAB,
    .down_cmd = IDM_FASTFORWARD_ON,
    .up_cmd = IDM_FASTFORWARD_OFF};

    config.gs_hotkey = {
    .identifier = L"GS Button",
    .key = 'G',
    .down_cmd = IDM_GS_ON,
    .up_cmd = IDM_GS_OFF};

    config.speed_down_hotkey = {
    .identifier = L"Speed down",
    .key = VK_OEM_MINUS,
    .down_cmd = IDM_SPEED_DOWN,
    };

    config.speed_up_hotkey = {
    .identifier = L"Speed up",
    .key = VK_OEM_PLUS,
    .down_cmd = IDM_SPEED_UP,
    };

    config.speed_reset_hotkey = {
    .identifier = L"Speed reset",
    .key = VK_OEM_PLUS,
    .ctrl = 1,
    .down_cmd = IDM_SPEED_RESET,
    };

    config.frame_advance_hotkey = {
    .identifier = L"Frame advance",
    .key = VK_OEM_5,
    .down_cmd = IDM_FRAMEADVANCE,
    };

    config.multi_frame_advance_hotkey = {
    .identifier = L"Multi-Frame advance",
    .key = VK_OEM_5,
    .ctrl = 1,
    .down_cmd = IDM_MULTI_FRAME_ADVANCE,
    };

    config.multi_frame_advance_inc_hotkey = {
    .identifier = L"Multi-Frame advance increment",
    .key = 'E',
    .ctrl = 1,
    .down_cmd = IDM_MULTI_FRAME_ADVANCE_INC,
    };

    config.multi_frame_advance_dec_hotkey = {
    .identifier = L"Multi-Frame advance decrement",
    .key = 'Q',
    .ctrl = 1,
    .down_cmd = IDM_MULTI_FRAME_ADVANCE_DEC,
    };

    config.multi_frame_advance_reset_hotkey = {
    .identifier = L"Multi-Frame advance reset",
    .key = 'E',
    .ctrl = 1,
    .shift = 1,
    .down_cmd = IDM_MULTI_FRAME_ADVANCE_RESET,
    };

    config.pause_hotkey = {
    .identifier = L"Pause",
    .key = VK_PAUSE,
    .down_cmd = IDM_PAUSE,
    };

    config.toggle_read_only_hotkey = {
    .identifier = L"Toggle read-only",
    .key = 'R',
    .shift = true,
    .down_cmd = IDM_VCR_READONLY,
    };

    config.toggle_movie_loop_hotkey = {
    .identifier = L"Toggle movie loop",
    .key = 'L',
    .shift = true,
    .down_cmd = IDM_LOOP_MOVIE,
    };

    config.start_movie_playback_hotkey = {
    .identifier = L"Start movie playback",
    .key = 'P',
    .ctrl = true,
    .shift = true,
    .down_cmd = IDM_START_MOVIE_PLAYBACK,
    };

    config.start_movie_recording_hotkey = {
    .identifier = L"Start movie recording",
    .key = 'R',
    .ctrl = true,
    .shift = true,
    .down_cmd = IDM_START_MOVIE_RECORDING,
    };

    config.stop_movie_hotkey = {
    .identifier = L"Stop movie",
    .key = 'C',
    .ctrl = true,
    .shift = true,
    .down_cmd = IDM_STOP_MOVIE,
    };

    config.create_movie_backup_hotkey = {
    .identifier = L"Create Movie Backup",
    .key = 'B',
    .ctrl = true,
    .shift = true,
    .down_cmd = IDM_CREATE_MOVIE_BACKUP,
    };

    config.take_screenshot_hotkey = {
    .identifier = L"Take screenshot",
    .key = VK_F12,
    .down_cmd = IDM_SCREENSHOT,
    };

    config.play_latest_movie_hotkey = {
    .identifier = L"Play latest movie",
    .key = 'T',
    .ctrl = true,
    .shift = true,
    .down_cmd = IDM_PLAY_LATEST_MOVIE,
    };

    config.load_latest_script_hotkey = {
    .identifier = L"Load latest script",
    .key = 'K',
    .ctrl = true,
    .shift = true,
    .down_cmd = IDM_LOAD_LATEST_LUA,
    };

    config.new_lua_hotkey = {
    .identifier = L"New Lua Instance",
    .key = 'N',
    .ctrl = true,
    .down_cmd = IDM_LOAD_LUA,
    };

    config.close_all_lua_hotkey = {
    .identifier = L"Close all Lua Instances",
    .key = 'W',
    .ctrl = true,
    .shift = true,
    .down_cmd = IDM_CLOSE_ALL_LUA,
    };

    config.load_rom_hotkey = {
    .identifier = L"Load ROM",
    .key = 'O',
    .ctrl = true,
    .down_cmd = IDM_LOAD_ROM,
    };

    config.close_rom_hotkey = {
    .identifier = L"Close ROM",
    .key = 'W',
    .ctrl = true,
    .down_cmd = IDM_CLOSE_ROM,
    };

    config.reset_rom_hotkey = {
    .identifier = L"Reset ROM",
    .key = 'R',
    .ctrl = true,
    .down_cmd = IDM_RESET_ROM,
    };

    config.load_latest_rom_hotkey = {
    .identifier = L"Load Latest ROM",
    .key = 'O',
    .ctrl = true,
    .shift = true,
    .down_cmd = IDM_LOAD_LATEST_ROM,
    };

    config.fullscreen_hotkey = {
    .identifier = L"Toggle Fullscreen",
    .key = VK_RETURN,
    .alt = true,
    .down_cmd = IDM_FULLSCREEN,
    };

    config.settings_hotkey = {
    .identifier = L"Show Settings",
    .key = 'S',
    .ctrl = true,
    .down_cmd = IDM_SETTINGS,
    };

    config.toggle_statusbar_hotkey = {
    .identifier = L"Toggle Statusbar",
    .key = 'S',
    .alt = true,
    .down_cmd = IDM_STATUSBAR,
    };

    config.refresh_rombrowser_hotkey = {
    .identifier = L"Refresh Rombrowser",
    .key = VK_F5,
    .ctrl = true,
    .down_cmd = IDM_REFRESH_ROMBROWSER,
    };

    config.seek_to_frame_hotkey = {
    .identifier = L"Seek to frame",
    .key = 'G',
    .ctrl = true,
    .down_cmd = IDM_SEEKER,
    };

    config.run_hotkey = {
    .identifier = L"Run",
    .key = 'P',
    .ctrl = true,
    .down_cmd = IDM_RUNNER,
    };

    config.piano_roll_hotkey = {
    .identifier = L"Open Piano Roll",
    .key = 'U',
    .ctrl = true,
    .down_cmd = IDM_PIANO_ROLL,
    };

    config.cheats_hotkey = {
    .identifier = L"Open Cheats dialog",
    .key = 'K',
    .ctrl = true,
    .down_cmd = IDM_CHEATS,
    };

    config.save_current_hotkey = {
    .identifier = L"Save to current slot",
    .key = 'I',
    .down_cmd = IDM_SAVE_SLOT,
    };

    config.load_current_hotkey = {
    .identifier = L"Load from current slot",
    .key = 'P',
    .down_cmd = IDM_LOAD_SLOT,
    };

    config.save_as_hotkey = {
    .identifier = L"Save state as",
    .key = 'N',
    .ctrl = true,
    .down_cmd = IDM_SAVE_STATE_AS,
    };

    config.load_as_hotkey = {
    .identifier = L"Load state as",
    .key = 'M',
    .ctrl = true,
    .down_cmd = IDM_LOAD_STATE_AS,
    };

    config.undo_load_state_hotkey = {
    .identifier = L"Undo load state",
    .key = 'Z',
    .ctrl = true,
    .down_cmd = IDM_UNDO_LOAD_STATE,
    };

    config.save_to_slot_1_hotkey = {
    .identifier = L"Save to slot 1",
    .key = '1',
    .shift = true,
    .down_cmd = ID_SAVE_1,
    };

    config.save_to_slot_2_hotkey = {
    .identifier = L"Save to slot 2",
    .key = '2',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 1,
    };

    config.save_to_slot_3_hotkey = {
    .identifier = L"Save to slot 3",
    .key = '3',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 2,
    };

    config.save_to_slot_4_hotkey = {
    .identifier = L"Save to slot 4",
    .key = '4',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 3,
    };

    config.save_to_slot_5_hotkey = {
    .identifier = L"Save to slot 5",
    .key = '5',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 4,
    };

    config.save_to_slot_6_hotkey = {
    .identifier = L"Save to slot 6",
    .key = '6',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 5,
    };

    config.save_to_slot_7_hotkey = {
    .identifier = L"Save to slot 7",
    .key = '7',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 6,
    };

    config.save_to_slot_8_hotkey = {
    .identifier = L"Save to slot 8",
    .key = '8',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 7,
    };

    config.save_to_slot_9_hotkey = {
    .identifier = L"Save to slot 9",
    .key = '9',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 8,
    };

    config.save_to_slot_10_hotkey = {
    .identifier = L"Save to slot 10",
    .key = '0',
    .shift = true,
    .down_cmd = ID_SAVE_1 + 9,
    };

    config.load_from_slot_1_hotkey = {
    .identifier = L"Load from slot 1",
    .key = VK_F1,
    .down_cmd = ID_LOAD_1,
    };

    config.load_from_slot_2_hotkey = {
    .identifier = L"Load from slot 2",
    .key = VK_F2,
    .down_cmd = ID_LOAD_1 + 1,
    };

    config.load_from_slot_3_hotkey = {
    .identifier = L"Load from slot 3",
    .key = VK_F3,
    .down_cmd = ID_LOAD_1 + 2,
    };

    config.load_from_slot_4_hotkey = {
    .identifier = L"Load from slot 4",
    .key = VK_F4,
    .down_cmd = ID_LOAD_1 + 3,
    };

    config.load_from_slot_5_hotkey = {
    .identifier = L"Load from slot 5",
    .key = VK_F5,
    .down_cmd = ID_LOAD_1 + 4,
    };

    config.load_from_slot_6_hotkey = {
    .identifier = L"Load from slot 6",
    .key = VK_F6,
    .down_cmd = ID_LOAD_1 + 5,
    };

    config.load_from_slot_7_hotkey = {
    .identifier = L"Load from slot 7",
    .key = VK_F7,
    .down_cmd = ID_LOAD_1 + 6,
    };

    config.load_from_slot_8_hotkey = {
    .identifier = L"Load from slot 8",
    .key = VK_F8,
    .down_cmd = ID_LOAD_1 + 7,
    };

    config.load_from_slot_9_hotkey = {
    .identifier = L"Load from slot 9",
    .key = VK_F9,
    .down_cmd = ID_LOAD_1 + 8,
    };

    config.load_from_slot_10_hotkey = {
    .identifier = L"Load from slot 10",
    .key = VK_F10,
    .down_cmd = ID_LOAD_1 + 9,
    };

    config.select_slot_1_hotkey = {
    .identifier = L"Select slot 1",
    .key = '1',
    .down_cmd = IDM_SELECT_1,
    };

    config.select_slot_2_hotkey = {
    .identifier = L"Select slot 2",
    .key = '2',
    .down_cmd = IDM_SELECT_1 + 1,
    };

    config.select_slot_3_hotkey = {
    .identifier = L"Select slot 3",
    .key = '3',
    .down_cmd = IDM_SELECT_1 + 2,
    };

    config.select_slot_4_hotkey = {
    .identifier = L"Select slot 4",
    .key = '4',
    .down_cmd = IDM_SELECT_1 + 3,
    };

    config.select_slot_5_hotkey = {
    .identifier = L"Select slot 5",
    .key = '5',
    .down_cmd = IDM_SELECT_1 + 4,
    };

    config.select_slot_6_hotkey = {
    .identifier = L"Select slot 6",
    .key = '6',
    .down_cmd = IDM_SELECT_1 + 5,
    };

    config.select_slot_7_hotkey = {
    .identifier = L"Select slot 7",
    .key = '7',
    .down_cmd = IDM_SELECT_1 + 6,
    };

    config.select_slot_8_hotkey = {
    .identifier = L"Select slot 8",
    .key = '8',
    .down_cmd = IDM_SELECT_1 + 7,
    };

    config.select_slot_9_hotkey = {
    .identifier = L"Select slot 9",
    .key = '9',
    .down_cmd = IDM_SELECT_1 + 8,
    };

    config.select_slot_10_hotkey = {
    .identifier = L"Select slot 10",
    .key = '0',
    .down_cmd = IDM_SELECT_1 + 9,
    };

    for (const auto& pair : DIALOG_SILENT_MODE_CHOICES)
    {
        config.silent_mode_dialog_choices[string_to_wstring(pair.first)] = std::to_wstring(pair.second);
    }

    return config;
}

const cfg_view g_default_config = get_default_config();

static std::string process_field_name(const std::wstring& field_name)
{
    std::string str = wstring_to_string(field_name);

    // We don't want the "core." prefix in the ini file...
    // This isn't too great of an approach though because it can cause silent key collisions but whatever
    if (str.starts_with("core."))
    {
        str.erase(0, 5);
    }

    return str;
}

void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, int32_t is_reading, cfg_hotkey* hotkey)
{
    const auto key = process_field_name(field_name);

    if (is_reading)
    {
        if (!ini.has(key))
        {
            return;
        }

        hotkey->key = std::stoi(ini[key]["key"]);
        hotkey->ctrl = std::stoi(ini[key]["ctrl"]);
        hotkey->shift = std::stoi(ini[key]["shift"]);
        hotkey->alt = std::stoi(ini[key]["alt"]);
    }
    else
    {
        ini[key]["key"] = std::to_string(hotkey->key);
        ini[key]["ctrl"] = std::to_string(hotkey->ctrl);
        ini[key]["shift"] = std::to_string(hotkey->shift);
        ini[key]["alt"] = std::to_string(hotkey->alt);
    }
}

void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, int32_t* value)
{
    const auto key = process_field_name(field_name);

    if (is_reading)
    {
        // keep the default value if the key doesnt exist
        // it will be created upon saving anyway
        if (!ini[FLAT_FIELD_KEY].has(key))
        {
            return;
        }
        *value = std::stoi(ini[FLAT_FIELD_KEY][key]);
    }
    else
    {
        ini[FLAT_FIELD_KEY][key] = std::to_string(*value);
    }
}

void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, uint64_t* value)
{
    const auto key = process_field_name(field_name);

    if (is_reading)
    {
        // keep the default value if the key doesnt exist
        // it will be created upon saving anyway
        if (!ini[FLAT_FIELD_KEY].has(key))
        {
            return;
        }
        *value = std::stoull(ini[FLAT_FIELD_KEY][key]);
    }
    else
    {
        ini[FLAT_FIELD_KEY][key] = std::to_string(*value);
    }
}

void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::wstring& value)
{
    const auto key = process_field_name(field_name);

    // BUG: Leading whitespace seems to be dropped by mINI after a roundtrip!!!

    if (is_reading)
    {
        // keep the default value if the key doesnt exist
        // it will be created upon saving anyway
        if (!ini[FLAT_FIELD_KEY].has(key))
        {
            return;
        }
        value = string_to_wstring(ini[FLAT_FIELD_KEY][key]);
    }
    else
    {
        ini[FLAT_FIELD_KEY][key] = wstring_to_string(value);
    }
}

void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::vector<std::wstring>& value)
{
    const auto key = process_field_name(field_name);

    if (is_reading)
    {
        // if the virtual collection doesn't exist just leave the vector empty, as attempting to read will crash
        if (!ini.has(key))
        {
            return;
        }

        for (size_t i = 0; i < ini[key].size(); i++)
        {
            value.push_back(string_to_wstring(ini[key][std::to_string(i)]));
        }
    }
    else
    {
        // create virtual collection:
        // dump under key field_name with i
        // [field_name]
        // 0 = a.m64
        // 1 = b.m64
        for (size_t i = 0; i < value.size(); i++)
        {
            ini[key][std::to_string(i)] = wstring_to_string(value[i]);
        }
    }
}

void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::map<std::wstring, std::wstring>& value)
{
    const auto key = process_field_name(field_name);

    if (is_reading)
    {
        // if the virtual map doesn't exist just leave the vector empty, as attempting to read will crash
        if (!ini.has(key))
        {
            return;
        }
        auto& map = ini[key];
        for (auto& pair : map)
        {
            value[string_to_wstring(pair.first)] = string_to_wstring(pair.second);
        }
    }
    else
    {
        // create virtual map:
        // [field_name]
        // value = value
        for (auto& pair : value)
        {
            ini[key][wstring_to_string(pair.first)] = wstring_to_string(pair.second);
        }
    }
}

void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::vector<int32_t>& value)
{
    std::vector<std::wstring> string_values;
    for (const auto int_value : value)
    {
        string_values.push_back(std::to_wstring(int_value));
    }

    handle_config_value(ini, field_name, is_reading, string_values);

    if (is_reading)
    {
        for (int i = 0; i < value.size(); ++i)
        {
            value[i] = std::stoi(string_values[i]);
        }
    }
}

const auto first_offset = offsetof(cfg_view, fast_forward_hotkey);
const auto last_offset = offsetof(cfg_view, select_slot_10_hotkey);

std::vector<cfg_hotkey*> collect_hotkeys(const cfg_view* config)
{
    // NOTE:
    // last_offset should contain the offset of the last hotkey
    // this also requires that the hotkeys are laid out contiguously, or else the pointer arithmetic fails
    // i recommend inserting your new hotkeys before the savestate hotkeys... pretty please
    std::vector<cfg_hotkey*> vec;
    for (size_t i = 0; i < ((last_offset - first_offset) / sizeof(cfg_hotkey)) + 1; i++)
    {
        auto hotkey = &(((cfg_hotkey*)config)[i]);
        vec.push_back(hotkey);
    }

    return vec;
}

static void handle_config_ini(const bool is_reading, mINI::INIStructure& ini)
{
#define HANDLE_P_VALUE(x) handle_config_value(ini, L#x, is_reading, &g_config.x);
#define HANDLE_VALUE(x) handle_config_value(ini, L#x, is_reading, g_config.x);

    if (is_reading)
    {
        // We need to fill the config with latest default values first, because some "new" fields might not exist in the ini
        g_config = get_default_config();
    }

    for (auto& hotkey : g_config_hotkeys)
    {
        handle_config_value(ini, hotkey->identifier, is_reading, hotkey);
    }

    HANDLE_VALUE(ignored_version)
    HANDLE_P_VALUE(core.total_rerecords)
    HANDLE_P_VALUE(core.total_frames)
    HANDLE_P_VALUE(core.core_type)
    HANDLE_P_VALUE(core.fps_modifier)
    HANDLE_P_VALUE(core.frame_skip_frequency)
    HANDLE_P_VALUE(st_slot)
    HANDLE_P_VALUE(core.fastforward_silent)
    HANDLE_P_VALUE(core.skip_rendering_lag)
    HANDLE_P_VALUE(core.rom_cache_size)
    HANDLE_P_VALUE(core.st_screenshot)
    HANDLE_P_VALUE(core.is_movie_loop_enabled)
    HANDLE_P_VALUE(core.counter_factor)
    HANDLE_P_VALUE(is_unfocused_pause_enabled)
    HANDLE_P_VALUE(is_statusbar_enabled)
    HANDLE_P_VALUE(statusbar_scale_up)
    HANDLE_P_VALUE(statusbar_layout)
    HANDLE_P_VALUE(plugin_discovery_async)
    HANDLE_P_VALUE(is_default_plugins_directory_used)
    HANDLE_P_VALUE(is_default_saves_directory_used)
    HANDLE_P_VALUE(is_default_screenshots_directory_used)
    HANDLE_P_VALUE(is_default_backups_directory_used)
    HANDLE_VALUE(plugins_directory)
    HANDLE_VALUE(saves_directory)
    HANDLE_VALUE(screenshots_directory)
    HANDLE_VALUE(states_path)
    HANDLE_VALUE(backups_directory)
    HANDLE_VALUE(recent_rom_paths)
    HANDLE_P_VALUE(is_recent_rom_paths_frozen)
    HANDLE_VALUE(recent_movie_paths)
    HANDLE_P_VALUE(is_recent_movie_paths_frozen)
    HANDLE_P_VALUE(is_rombrowser_recursion_enabled)
    HANDLE_P_VALUE(core.is_reset_recording_enabled)
    HANDLE_P_VALUE(capture_mode)
    HANDLE_P_VALUE(presenter_type)
    HANDLE_P_VALUE(lazy_renderer_init)
    HANDLE_P_VALUE(encoder_type)
    HANDLE_P_VALUE(capture_delay)
    HANDLE_VALUE(ffmpeg_final_options)
    HANDLE_VALUE(ffmpeg_path)
    HANDLE_P_VALUE(synchronization_mode)
    HANDLE_P_VALUE(keep_default_working_directory)
    HANDLE_P_VALUE(fast_dispatcher)
    HANDLE_P_VALUE(plugin_discovery_delayed)
    HANDLE_VALUE(lua_script_path)
    HANDLE_VALUE(recent_lua_script_paths)
    HANDLE_P_VALUE(is_recent_scripts_frozen)
    HANDLE_P_VALUE(core.seek_savestate_interval)
    HANDLE_P_VALUE(core.seek_savestate_max_count)
    HANDLE_P_VALUE(piano_roll_constrain_edit_to_column)
    HANDLE_P_VALUE(piano_roll_undo_stack_size)
    HANDLE_P_VALUE(piano_roll_keep_selection_visible)
    HANDLE_P_VALUE(piano_roll_keep_playhead_visible)
    HANDLE_P_VALUE(core.st_undo_load)
    HANDLE_P_VALUE(core.use_summercart)
    HANDLE_P_VALUE(core.wii_vc_emulation)
    HANDLE_P_VALUE(core.float_exception_emulation)
    HANDLE_P_VALUE(core.is_audio_delay_enabled)
    HANDLE_P_VALUE(core.is_compiled_jump_enabled)
    HANDLE_VALUE(selected_video_plugin)
    HANDLE_VALUE(selected_audio_plugin)
    HANDLE_VALUE(selected_input_plugin)
    HANDLE_VALUE(selected_rsp_plugin)
    HANDLE_P_VALUE(last_movie_type)
    HANDLE_VALUE(last_movie_author)
    HANDLE_P_VALUE(window_x)
    HANDLE_P_VALUE(window_y)
    HANDLE_P_VALUE(window_width)
    HANDLE_P_VALUE(window_height)
    HANDLE_VALUE(rombrowser_column_widths)
    HANDLE_VALUE(rombrowser_rom_paths)
    HANDLE_P_VALUE(rombrowser_sort_ascending)
    HANDLE_P_VALUE(rombrowser_sorted_column)
    HANDLE_VALUE(persistent_folder_paths)
    HANDLE_P_VALUE(settings_tab)
    HANDLE_P_VALUE(vcr_0_index)
    HANDLE_P_VALUE(increment_slot)
    HANDLE_P_VALUE(core.pause_at_frame)
    HANDLE_P_VALUE(core.pause_at_last_frame)
    HANDLE_P_VALUE(core.vcr_readonly)
    HANDLE_P_VALUE(core.vcr_backups)
    HANDLE_P_VALUE(core.vcr_write_extended_format)
    HANDLE_P_VALUE(automatic_update_checking)
    HANDLE_P_VALUE(silent_mode)
    HANDLE_P_VALUE(core.max_lag)
    HANDLE_VALUE(seeker_value)
    HANDLE_P_VALUE(multi_frame_advance_count)
    HANDLE_VALUE(silent_mode_dialog_choices)
}

std::filesystem::path get_config_path()
{
    return g_app_path / CONFIG_FILE_NAME;
}

/// Modifies the config to apply value limits and other constraints
void config_apply_limits()
{
    // handle edge case: closing while minimized produces bogus values for position
    if (g_config.window_x < -10'000 || g_config.window_y < -10'000)
    {
        g_config.window_x = g_default_config.window_x;
        g_config.window_y = g_default_config.window_y;
        g_config.window_width = g_default_config.window_width;
        g_config.window_height = g_default_config.window_height;
    }

    if (g_config.rombrowser_column_widths.size() < 4)
    {
        // something's malformed, fuck off and use default values
        g_config.rombrowser_column_widths = g_default_config.rombrowser_column_widths;
    }

    // Causes too many issues
    if (g_config.core.seek_savestate_interval == 1)
    {
        g_config.core.seek_savestate_interval = 2;
    }

    g_config.settings_tab = std::min(std::max(g_config.settings_tab, 0), 2);

    for (const auto& pair : DIALOG_SILENT_MODE_CHOICES)
    {
        const auto key = string_to_wstring(pair.first);
        if (!g_config.silent_mode_dialog_choices.contains(key))
        {
            g_config.silent_mode_dialog_choices[key] = std::to_wstring(pair.second);
        }
    }
}

void save_config()
{
    Messenger::broadcast(Messenger::Message::ConfigSaving, nullptr);
    config_apply_limits();

    std::remove(get_config_path().string().c_str());

    mINI::INIFile file(get_config_path().string());
    mINI::INIStructure ini;

    handle_config_ini(false, ini);

    file.write(ini, true);
}

void config_load()
{
    if (!std::filesystem::exists(get_config_path()))
    {
        g_view_logger->info("[CONFIG] Default config file does not exist. Generating...");
        g_config = get_default_config();
        save_config();
    }

    mINI::INIFile file(get_config_path().string());
    mINI::INIStructure ini;
    file.read(ini);

    handle_config_ini(true, ini);

    config_apply_limits();

    Messenger::broadcast(Messenger::Message::ConfigLoaded, nullptr);
}

void config_init()
{
    g_config_hotkeys = collect_hotkeys(&g_config);
}
