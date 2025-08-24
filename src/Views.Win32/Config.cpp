/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <Messenger.h>
#include <ini.h>

static t_config get_default_config();

t_config g_config;

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

const t_config g_default_config = get_default_config();

static t_config get_default_config()
{
    t_config config = {};

    for (const auto& pair : DIALOG_SILENT_MODE_CHOICES)
    {
        config.silent_mode_dialog_choices[io_service.string_to_wstring(pair.first)] = std::to_wstring(pair.second);
    }

    return config;
}

static std::string process_field_name(const std::wstring& field_name)
{
    std::string str = io_service.wstring_to_string(field_name);

    // We don't want the "core." prefix in the ini file...
    // This isn't too great of an approach though because it can cause silent key collisions but whatever
    if (str.starts_with("core."))
    {
        str.erase(0, 5);
    }

    return str;
}

static void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, int32_t* value)
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

static void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, uint64_t* value)
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

static void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::wstring& value)
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
        value = io_service.string_to_wstring(ini[FLAT_FIELD_KEY][key]);
    }
    else
    {
        ini[FLAT_FIELD_KEY][key] = io_service.wstring_to_string(value);
    }
}

static void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::vector<std::wstring>& value)
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
            value.push_back(io_service.string_to_wstring(ini[key][std::to_string(i)]));
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
            ini[key][std::to_string(i)] = io_service.wstring_to_string(value[i]);
        }
    }
}

static void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::map<std::wstring, std::wstring>& value)
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
            value[io_service.string_to_wstring(pair.first)] = io_service.string_to_wstring(pair.second);
        }
    }
    else
    {
        // create virtual map:
        // [field_name]
        // value = value
        for (auto& pair : value)
        {
            ini[key][io_service.wstring_to_string(pair.first)] = io_service.wstring_to_string(pair.second);
        }
    }
}

static void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::map<std::wstring, Hotkey::t_hotkey>& value)
{
    const auto key = process_field_name(field_name);

    // Structure:
    // [action_fullpath]
    // key
    // ctrl
    // shift
    // alt

    const auto prefix = io_service.wstring_to_string(std::format(L"{}_", field_name));

    if (is_reading)
    {
        for (const auto& pair : ini)
        {
            if (!pair.first.starts_with(prefix))
            {
                continue;
            }

            const auto action_path = pair.first.substr(prefix.size());

            Hotkey::t_hotkey hotkey{};

            const auto key = pair.second.get("key");
            if (!key.empty())
            {
                hotkey.key = std::stoi(key);
            }

            const auto ctrl = pair.second.get("ctrl");
            if (!ctrl.empty())
            {
                hotkey.ctrl = std::stoi(ctrl);
            }

            const auto shift = pair.second.get("shift");
            if (!shift.empty())
            {
                hotkey.shift = std::stoi(shift);
            }

            const auto alt = pair.second.get("alt");
            if (!alt.empty())
            {
                hotkey.alt = std::stoi(alt);
            }

            value[io_service.string_to_wstring(action_path)] = hotkey;
        }
    }
    else
    {
        for (const auto& [action_path, hotkey] : value)
        {
            const auto action_key = prefix + io_service.wstring_to_string(action_path);
            ini[action_key]["key"] = std::to_string(hotkey.key);
            ini[action_key]["ctrl"] = std::to_string(hotkey.ctrl);
            ini[action_key]["shift"] = std::to_string(hotkey.shift);
            ini[action_key]["alt"] = std::to_string(hotkey.alt);
        }
    }
}

static void handle_config_value(mINI::INIStructure& ini, const std::wstring& field_name, const int32_t is_reading, std::vector<int32_t>& value)
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

static void handle_config_ini(const bool is_reading, mINI::INIStructure& ini)
{
#define HANDLE_P_VALUE(x) handle_config_value(ini, L#x, is_reading, &g_config.x);
#define HANDLE_VALUE(x) handle_config_value(ini, L#x, is_reading, g_config.x);

    if (is_reading)
    {
        // We need to fill the config with latest default values first, because some "new" fields might not exist in the ini
        g_config = get_default_config();
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
    HANDLE_P_VALUE(core.c_eq_s_nan_accurate)
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
    HANDLE_P_VALUE(core.wait_at_movie_end)
    HANDLE_P_VALUE(automatic_update_checking)
    HANDLE_P_VALUE(silent_mode)
    HANDLE_P_VALUE(core.max_lag)
    HANDLE_VALUE(seeker_value)
    HANDLE_P_VALUE(multi_frame_advance_count)
    HANDLE_VALUE(silent_mode_dialog_choices)
    HANDLE_VALUE(trusted_lua_paths)
    HANDLE_VALUE(hotkeys)
    HANDLE_VALUE(inital_hotkeys)
}

static std::filesystem::path get_config_path()
{
    return g_main_wnd.app_path / CONFIG_FILE_NAME;
}

/**
 * \brief Modifies the config to apply value limits and other constraints.
 */
static void config_patch(t_config& cfg)
{
    // handle edge case: closing while minimized produces bogus values for position
    if (cfg.window_x < -10'000 || cfg.window_y < -10'000)
    {
        cfg.window_x = g_default_config.window_x;
        cfg.window_y = g_default_config.window_y;
        cfg.window_width = g_default_config.window_width;
        cfg.window_height = g_default_config.window_height;
    }

    if (cfg.rombrowser_column_widths.size() < 4)
    {
        // something's malformed, fuck off and use default values
        cfg.rombrowser_column_widths = g_default_config.rombrowser_column_widths;
    }

    // Causes too many issues
    if (cfg.core.seek_savestate_interval == 1)
    {
        cfg.core.seek_savestate_interval = 2;
    }

    cfg.settings_tab = std::min(std::max(cfg.settings_tab, 0), 2);

    for (const auto& pair : DIALOG_SILENT_MODE_CHOICES)
    {
        const auto key = io_service.string_to_wstring(pair.first);
        if (!cfg.silent_mode_dialog_choices.contains(key))
        {
            cfg.silent_mode_dialog_choices[key] = std::to_wstring(pair.second);
        }
    }
}

void Config::init()
{
}

void Config::save()
{
    Messenger::broadcast(Messenger::Message::ConfigSaving, nullptr);

    config_patch(g_config);

    std::remove(get_config_path().string().c_str());

    mINI::INIFile file(get_config_path().string());
    mINI::INIStructure ini;

    handle_config_ini(false, ini);

    file.write(ini, true);
}

void Config::load()
{
    if (!std::filesystem::exists(get_config_path()))
    {
        g_view_logger->info("[CONFIG] Default config file does not exist. Generating...");
        g_config = get_default_config();
        save();
    }

    mINI::INIFile file(get_config_path().string());
    mINI::INIStructure ini;
    file.read(ini);

    handle_config_ini(true, ini);

    config_patch(g_config);

    Messenger::broadcast(Messenger::Message::ConfigLoaded, nullptr);
}

std::filesystem::path Config::plugin_directory()
{
    if (g_config.is_default_plugins_directory_used)
    {
        return g_main_wnd.app_path / L"plugin\\";
    }
    return g_config.plugins_directory;
}

std::filesystem::path Config::save_directory()
{
    if (g_config.is_default_saves_directory_used)
    {
        return g_main_wnd.app_path / L"save\\";
    }
    return g_config.saves_directory;
}

std::filesystem::path Config::screenshot_directory()
{
    if (g_config.is_default_screenshots_directory_used)
    {
        return g_main_wnd.app_path / L"screenshots\\";
    }
    return g_config.screenshots_directory;
}

std::filesystem::path Config::backup_directory()
{
    if (g_config.is_default_backups_directory_used)
    {
        return "backups\\";
    }
    return g_config.backups_directory;
}
