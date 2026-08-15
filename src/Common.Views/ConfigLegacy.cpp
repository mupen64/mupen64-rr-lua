/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/ConfigLegacy.hpp>
#include <Common.Views/App.hpp>
#include <Common.Views/ActionManager.hpp>
#include <Common.Views/Hotkey.hpp>
#include <nlohmann/json.hpp>

constexpr auto FLAT_FIELD_KEY = "config";

static std::string ini_cleanup_field(std::string field_name)
{
    // We don't want the "core." prefix in the ini file...
    // This isn't too great of an approach though because it can cause silent key collisions but whatever
    if (field_name.starts_with("core.")) field_name.erase(0, 5);

    return field_name;
}

static void ini_handle_config_value(mINI::INIStructure &ini, const std::string &field_name, int32_t *value)
{
    const auto key = ini_cleanup_field(field_name);

    // keep the default value if the key doesnt exist
    // it will be created upon saving anyway
    if (!ini[FLAT_FIELD_KEY].has(key))
    {
        return;
    }
    *value = std::stoi(ini[FLAT_FIELD_KEY][key]);
}

static void ini_handle_config_value(mINI::INIStructure &ini, const std::string &field_name, double *value)
{
    const auto key = ini_cleanup_field(field_name);

    // keep the default value if the key doesnt exist
    // it will be created upon saving anyway
    if (!ini[FLAT_FIELD_KEY].has(key))
    {
        return;
    }
    *value = std::stod(ini[FLAT_FIELD_KEY][key]);
}

static void ini_handle_config_value(mINI::INIStructure &ini, const std::string &field_name, uint64_t *value)
{
    const auto key = ini_cleanup_field(field_name);

    // keep the default value if the key doesnt exist
    // it will be created upon saving anyway
    if (!ini[FLAT_FIELD_KEY].has(key))
    {
        return;
    }
    *value = std::stoull(ini[FLAT_FIELD_KEY][key]);
}

// !!!
static void ini_handle_config_value(mINI::INIStructure &ini, const std::string &field_name, std::string &value)
{
    const auto key = ini_cleanup_field(field_name);

    // BUG: Leading whitespace seems to be dropped by mINI after a roundtrip!!!

    // keep the default value if the key doesnt exist
    // it will be created upon saving anyway
    if (!ini[FLAT_FIELD_KEY].has(key))
    {
        return;
    }
    value = ini[FLAT_FIELD_KEY][key];
}

// !!!
static void ini_handle_config_value(mINI::INIStructure &ini, const std::string &field_name,
                                    std::vector<std::string> &value)
{
    const auto key = ini_cleanup_field(field_name);

    // if the virtual collection doesn't exist just leave the vector empty, as attempting to read will crash
    if (!ini.has(key))
    {
        return;
    }

    for (size_t i = 0; i < ini[key].size(); i++)
    {
        value.push_back(ini[key][std::to_string(i)]);
    }
}

// !!!
static void ini_handle_config_value(mINI::INIStructure &ini, const std::string &field_name,
                                    std::map<std::string, std::string> &value)
{
    const auto key = ini_cleanup_field(field_name);

    // if the virtual map doesn't exist just leave the vector empty, as attempting to read will crash
    if (!ini.has(key))
    {
        return;
    }
    auto &map = ini[key];
    for (auto &pair : map)
    {
        value[pair.first] = pair.second;
    }
}

// !!!
static void ini_handle_config_value(mINI::INIStructure &ini, const std::string &field_name,
                                    std::map<std::string, Hotkey> &value)
{
    // Structure:
    // [action_fullpath]
    // key
    // ctrl
    // shift
    // alt

    const auto prefix = std::format("{}_", field_name);

    for (const auto &pair : ini)
    {
        if (!pair.first.starts_with(prefix))
        {
            continue;
        }

        const auto action_path = ActionManager::normalize_filter(pair.first.substr(prefix.size()));

        const auto assigned = pair.second.get("assigned");
        Hotkey hotkey = assigned == "0" ? Hotkey::make_unassigned() : Hotkey::make_empty();

        const auto key_value = pair.second.get("key");
        if (!key_value.empty() && hotkey.is_assigned())
        {
            try
            {
                // 1.4.x stored Windows virtual-key codes. Use the platform-specific conversion shim, just like the
                // 1.4.x JSON migration does.
                const auto converted = app_json_to_hotkey(nlohmann::json{{"key", std::stoi(key_value)}});
                if (converted)
                {
                    hotkey.trigger = converted->trigger;
                }
            }
            catch (...)
            {
            }
        }

        const auto ctrl = pair.second.get("ctrl");
        if (!ctrl.empty())
        {
            try
            {
                hotkey.ctrl = std::stoi(ctrl);
            }
            catch (...)
            {
            }
        }

        const auto shift = pair.second.get("shift");
        if (!shift.empty())
        {
            try
            {
                hotkey.shift = std::stoi(shift);
            }
            catch (...)
            {
            }
        }

        const auto alt = pair.second.get("alt");
        if (!alt.empty())
        {
            try
            {
                hotkey.alt = std::stoi(alt);
            }
            catch (...)
            {
            }
        }

        value[action_path] = hotkey;
    }
}

static void ini_handle_config_value(mINI::INIStructure &ini, const std::string &field_name, std::vector<int32_t> &value)
{
    std::vector<std::string> string_values;
    for (const auto int_value : value)
    {
        string_values.push_back(std::to_string(int_value));
    }

    ini_handle_config_value(ini, field_name, string_values);

    for (int i = 0; i < value.size(); ++i)
    {
        value[i] = std::stoi(string_values[i]);
    }
}

void Config::Legacy::handle_config_ini(mINI::INIStructure &ini)
{
#define HANDLE_P_VALUE(x) ini_handle_config_value(ini, #x, &g_config.x);
#define HANDLE_VALUE(x) ini_handle_config_value(ini, #x, g_config.x);

    // We need to fill the config with latest default values first, because some "new" fields might not exist in the
    // ini
    g_config = Config::default_config();

    HANDLE_VALUE(ignored_version)
    HANDLE_P_VALUE(theme)
    HANDLE_P_VALUE(core.total_rerecords)
    HANDLE_P_VALUE(core.total_frames)
    HANDLE_P_VALUE(core.core_type)
    HANDLE_P_VALUE(core.fps_modifier)
    HANDLE_P_VALUE(st_slot)
    HANDLE_P_VALUE(core.rom_cache_size)
    HANDLE_P_VALUE(core.st_screenshot)
    HANDLE_P_VALUE(core.is_movie_loop_enabled)
    HANDLE_P_VALUE(is_unfocused_pause_enabled)
    HANDLE_P_VALUE(is_statusbar_enabled)
    HANDLE_P_VALUE(statusbar_scale_up)
    HANDLE_P_VALUE(statusbar_layout)
    HANDLE_VALUE(rom_directory)
    HANDLE_VALUE(plugins_directory)
    HANDLE_VALUE(saves_directory)
    HANDLE_VALUE(screenshots_directory)
    HANDLE_VALUE(backups_directory)
    HANDLE_VALUE(recent_rom_paths)
    HANDLE_P_VALUE(is_recent_rom_paths_frozen)
    HANDLE_VALUE(recent_movie_paths)
    HANDLE_P_VALUE(is_recent_movie_paths_frozen)
    HANDLE_P_VALUE(is_rombrowser_recursion_enabled)
    HANDLE_P_VALUE(core.is_reset_recording_enabled)
    HANDLE_P_VALUE(capture_mode)
    HANDLE_P_VALUE(stop_capture_at_movie_end)
    HANDLE_P_VALUE(presenter_type)
    HANDLE_P_VALUE(lazy_renderer_init)
    HANDLE_P_VALUE(encoder_type)
    HANDLE_P_VALUE(capture_delay)
    HANDLE_VALUE(ffmpeg_options)
    HANDLE_VALUE(ffmpeg_path)
    HANDLE_P_VALUE(synchronization_mode)
    HANDLE_P_VALUE(keep_default_working_directory)
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
    HANDLE_P_VALUE(core.rcp_lag_emulation)
    HANDLE_P_VALUE(core.cpu_cf)
    HANDLE_P_VALUE(core.rcp_lag_factor)
    HANDLE_P_VALUE(core.float_exception_emulation)
    HANDLE_P_VALUE(core.c_eq_s_nan_accurate)
    HANDLE_P_VALUE(core.accurate_rdp_completion)
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
    HANDLE_P_VALUE(rombrowser_sort_ascending)
    HANDLE_P_VALUE(rombrowser_sorted_column)
    HANDLE_VALUE(persistent_folder_paths)
    HANDLE_P_VALUE(settings_tab)
    HANDLE_P_VALUE(vcr_0_index)
    HANDLE_P_VALUE(increment_slot)
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
    HANDLE_VALUE(lua_paths)
    HANDLE_VALUE(hotkeys)
    HANDLE_VALUE(inital_hotkeys)
}

/**
 * \brief Migrates old values from the specified config to new ones if possible.
 */
void Config::Legacy::migrate_config_ini(t_config &config, const mINI::INIStructure &ini)
{
    // no migrations currently
}
