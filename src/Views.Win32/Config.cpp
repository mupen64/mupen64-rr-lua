/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "IOUtils.hpp"
#include <Config.hpp>
#include <Messenger.hpp>
#include <ini.h>
#include <action/AppActions.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json;

static t_config get_default_config();

t_config g_config;

#ifdef _M_X64
#define LEGACY_CONFIG_FILE_NAME L"config-x64.ini"
#define CONFIG_FILE_NAME L"config-x64.json"
#else
#define LEGACY_CONFIG_FILE_NAME L"config.ini"
#define CONFIG_FILE_NAME L"config.json"
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

    for (const auto &pair : DIALOG_SILENT_MODE_CHOICES)
    {
        config.silent_mode_dialog_choices[IOUtils::to_wide_string(pair.first)] = std::to_wstring(pair.second);
    }

    return config;
}

#pragma region ini file shenanigans

static std::string ini_cleanup_field(const std::wstring &field_name)
{
    std::string str = IOUtils::to_utf8_string(field_name);

    // We don't want the "core." prefix in the ini file...
    // This isn't too great of an approach though because it can cause silent key collisions but whatever
    if (str.starts_with("core."))
    {
        str.erase(0, 5);
    }

    return str;
}

static void ini_handle_config_value(mINI::INIStructure &ini, const std::wstring &field_name, const int32_t is_reading,
                                    int32_t *value)
{
    const auto key = ini_cleanup_field(field_name);

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

static void ini_handle_config_value(mINI::INIStructure &ini, const std::wstring &field_name, const int32_t is_reading,
                                    double *value)
{
    const auto key = ini_cleanup_field(field_name);

    if (is_reading)
    {
        // keep the default value if the key doesnt exist
        // it will be created upon saving anyway
        if (!ini[FLAT_FIELD_KEY].has(key))
        {
            return;
        }
        *value = std::stod(ini[FLAT_FIELD_KEY][key]);
    }
    else
    {
        ini[FLAT_FIELD_KEY][key] = std::to_string(*value);
    }
}

static void ini_handle_config_value(mINI::INIStructure &ini, const std::wstring &field_name, const int32_t is_reading,
                                    uint64_t *value)
{
    const auto key = ini_cleanup_field(field_name);

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

// !!!
static void ini_handle_config_value(mINI::INIStructure &ini, const std::wstring &field_name, const int32_t is_reading,
                                    std::wstring &value)
{
    const auto key = ini_cleanup_field(field_name);

    // BUG: Leading whitespace seems to be dropped by mINI after a roundtrip!!!

    if (is_reading)
    {
        // keep the default value if the key doesnt exist
        // it will be created upon saving anyway
        if (!ini[FLAT_FIELD_KEY].has(key))
        {
            return;
        }
        value = IOUtils::to_wide_string(ini[FLAT_FIELD_KEY][key]);
    }
    else
    {
        ini[FLAT_FIELD_KEY][key] = IOUtils::to_utf8_string(value);
    }
}

// !!!
static void ini_handle_config_value(mINI::INIStructure &ini, const std::wstring &field_name, const int32_t is_reading,
                                    std::vector<std::wstring> &value)
{
    const auto key = ini_cleanup_field(field_name);

    if (is_reading)
    {
        // if the virtual collection doesn't exist just leave the vector empty, as attempting to read will crash
        if (!ini.has(key))
        {
            return;
        }

        for (size_t i = 0; i < ini[key].size(); i++)
        {
            value.push_back(IOUtils::to_wide_string(ini[key][std::to_string(i)]));
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
            ini[key][std::to_string(i)] = IOUtils::to_utf8_string(value[i]);
        }
    }
}

// !!!
static void ini_handle_config_value(mINI::INIStructure &ini, const std::wstring &field_name, const int32_t is_reading,
                                    std::map<std::wstring, std::wstring> &value)
{
    const auto key = ini_cleanup_field(field_name);

    if (is_reading)
    {
        // if the virtual map doesn't exist just leave the vector empty, as attempting to read will crash
        if (!ini.has(key))
        {
            return;
        }
        auto &map = ini[key];
        for (auto &pair : map)
        {
            value[IOUtils::to_wide_string(pair.first)] = IOUtils::to_wide_string(pair.second);
        }
    }
    else
    {
        // create virtual map:
        // [field_name]
        // value = value
        for (auto &pair : value)
        {
            ini[key][IOUtils::to_utf8_string(pair.first)] = IOUtils::to_utf8_string(pair.second);
        }
    }
}

// !!!
static void ini_handle_config_value(mINI::INIStructure &ini, const std::wstring &field_name, const int32_t is_reading,
                                    std::map<std::wstring, Hotkey::t_hotkey> &value)
{
    const auto key = ini_cleanup_field(field_name);

    // Structure:
    // [action_fullpath]
    // key
    // ctrl
    // shift
    // alt

    const auto prefix = IOUtils::to_utf8_string(std::format(L"{}_", field_name));

    if (is_reading)
    {
        for (const auto &pair : ini)
        {
            if (!pair.first.starts_with(prefix))
            {
                continue;
            }

            const auto action_path = pair.first.substr(prefix.size());

            Hotkey::t_hotkey hotkey = Hotkey::t_hotkey::make_empty();

            const auto key = pair.second.get("key");
            if (!key.empty())
            {
                try
                {
                    hotkey.key = std::stoi(key);
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

            const auto assigned = pair.second.get("assigned");
            if (!alt.empty())
            {
                try
                {
                    hotkey.assigned = std::stoi(assigned);
                }
                catch (...)
                {
                }
            }

            value[IOUtils::to_wide_string(action_path)] = hotkey;
        }
    }
    else
    {
        for (const auto &[action_path, hotkey] : value)
        {
            const auto action_key = prefix + IOUtils::to_utf8_string(action_path);
            ini[action_key]["key"] = std::to_string(hotkey.key);
            ini[action_key]["ctrl"] = std::to_string(hotkey.ctrl);
            ini[action_key]["shift"] = std::to_string(hotkey.shift);
            ini[action_key]["alt"] = std::to_string(hotkey.alt);
            ini[action_key]["assigned"] = std::to_string(hotkey.assigned);
        }
    }
}

static void ini_handle_config_value(mINI::INIStructure &ini, const std::wstring &field_name, const int32_t is_reading,
                                    std::vector<int32_t> &value)
{
    std::vector<std::wstring> string_values;
    for (const auto int_value : value)
    {
        string_values.push_back(std::to_wstring(int_value));
    }

    ini_handle_config_value(ini, field_name, is_reading, string_values);

    if (is_reading)
    {
        for (int i = 0; i < value.size(); ++i)
        {
            value[i] = std::stoi(string_values[i]);
        }
    }
}

static void handle_config_ini(const bool is_reading, mINI::INIStructure &ini)
{
#define _WIDE2(s) L ## s
#define HANDLE_P_VALUE(x) ini_handle_config_value(ini, _WIDE2(#x), is_reading, &g_config.x);
#define HANDLE_VALUE(x) ini_handle_config_value(ini, _WIDE2(#x), is_reading, g_config.x);

    if (is_reading)
    {
        // We need to fill the config with latest default values first, because some "new" fields might not exist in the
        // ini
        g_config = get_default_config();
    }

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

static std::filesystem::path get_legacy_config_path()
{
    return g_main_ctx.app_path / LEGACY_CONFIG_FILE_NAME;
}

/**
 * \brief Migrates old values from the specified config to new ones if possible.
 */
static void migrate_config_ini(t_config &config, const mINI::INIStructure &ini)
{
    const auto migrate_hotkey = [&](const std::string &old_section_name, std::wstring action) {
        action = ActionManager::normalize_filter(action);

        if (!ini.has(old_section_name))
        {
            return;
        }

        auto hotkey = Hotkey::t_hotkey::make_empty();
        const auto &section = ini.get(old_section_name);

        try
        {
            hotkey.key = std::stoi(section.get("key"));
            hotkey.ctrl = std::stoi(section.get("ctrl"));
            hotkey.shift = std::stoi(section.get("shift"));
            hotkey.alt = std::stoi(section.get("alt"));
        }
        catch (...)
        {
        }

        g_view_logger->info(L"[Config] Migrating {} -> {} ({})", IOUtils::to_wide_string(old_section_name), action,
                            hotkey.to_wstring());
        config.hotkeys[action] = hotkey;
        config.inital_hotkeys[action] = hotkey;
    };

    // Migrate pre-1.3.0 hotkeys to their action counterparts.
    migrate_hotkey("Fast-forward", AppActions::FAST_FORWARD);
    migrate_hotkey("GS Button", AppActions::GS_BUTTON);
    migrate_hotkey("Speed down", AppActions::SPEED_DOWN);
    migrate_hotkey("Speed up", AppActions::SPEED_UP);
    migrate_hotkey("Speed reset", AppActions::SPEED_RESET);
    migrate_hotkey("Frame advance", AppActions::FRAME_ADVANCE);
    migrate_hotkey("Multi-Frame advance", AppActions::MULTI_FRAME_ADVANCE);
    migrate_hotkey("Multi-Frame advance increment", AppActions::MULTI_FRAME_ADVANCE_INCREMENT);
    migrate_hotkey("Multi-Frame advance decrement", AppActions::MULTI_FRAME_ADVANCE_DECREMENT);
    migrate_hotkey("Multi-Frame advance reset", AppActions::MULTI_FRAME_ADVANCE_RESET);
    migrate_hotkey("Pause", AppActions::PAUSE);
    migrate_hotkey("Toggle read-only", AppActions::READONLY);
    migrate_hotkey("Toggle movie loop", AppActions::LOOP_MOVIE_PLAYBACK);
    migrate_hotkey("Start movie playback", AppActions::START_MOVIE_PLAYBACK);
    migrate_hotkey("Start movie recording", AppActions::START_MOVIE_RECORDING);
    migrate_hotkey("Continue movie recording", AppActions::CONTINUE_MOVIE_RECORDING);
    migrate_hotkey("Stop movie", AppActions::STOP_MOVIE);
    migrate_hotkey("Create Movie Backup", AppActions::CREATE_MOVIE_BACKUP);
    migrate_hotkey("Take screenshot", AppActions::SCREENSHOT);
    migrate_hotkey("Play latest movie", AppActions::RECENT_MOVIES + L" > Load Recent Item 1");
    migrate_hotkey("Load latest script", AppActions::RECENT_SCRIPTS + L" > Load Recent Item 1");
    migrate_hotkey("New Lua Instance", AppActions::SHOW_INSTANCES);
    migrate_hotkey("Close all Lua Instances", AppActions::CLOSE_ALL);
    migrate_hotkey("Load ROM", AppActions::LOAD_ROM);
    migrate_hotkey("Close ROM", AppActions::CLOSE_ROM);
    migrate_hotkey("Reset ROM", AppActions::RESET_ROM);
    migrate_hotkey("Load Latest ROM", AppActions::RECENT_ROMS + L" > Load Recent Item 1");
    migrate_hotkey("Show Settings", AppActions::SETTINGS);
    migrate_hotkey("Toggle Statusbar", AppActions::STATUSBAR);
    migrate_hotkey("Refresh Rombrowser", AppActions::REFRESH_ROM_LIST);
    migrate_hotkey("Seek to frame", AppActions::SEEK_TO);
    migrate_hotkey("Run", AppActions::COMMAND_PALETTE);
    migrate_hotkey("Open Piano Roll", AppActions::PIANO_ROLL);
    migrate_hotkey("Open Cheats dialog", AppActions::CHEATS);
    migrate_hotkey("Save to current slot", AppActions::SAVE_CURRENT_SLOT);
    migrate_hotkey("Load from current slot", AppActions::LOAD_CURRENT_SLOT);
    migrate_hotkey("Save state as", AppActions::SAVE_STATE_FILE);
    migrate_hotkey("Load state as", AppActions::LOAD_STATE_FILE);
    migrate_hotkey("Undo load state", AppActions::UNDO_LOAD_STATE);
    for (int i = 0; i < 10; ++i)
    {
        migrate_hotkey(std::format("Save to slot {}", i),
                       std::vformat(AppActions::SAVE_SLOT_X, std::make_wformat_args(i)));
        migrate_hotkey(std::format("Load from slot {}", i),
                       std::vformat(AppActions::LOAD_SLOT_X, std::make_wformat_args(i)));
        migrate_hotkey(std::format("Select slot {}", i),
                       std::vformat(AppActions::SELECT_SLOT_X, std::make_wformat_args(i)));
    }

    // Migrate pre-1.3.0-5 rombrowser paths. We just pick the first one :P
    if (ini.has("rombrowser_rom_paths"))
    {
        const auto &section = ini.get("rombrowser_rom_paths");
        if (section.size() > 0)
        {
            const auto path = section.get("0");
            config.rom_directory = IOUtils::to_wide_string(path);

            g_view_logger->info(L"[Config] Migrated rom browser path {}", config.rom_directory);
        }
    }
}

#pragma endregion

#pragma region JSON file parsing

// READING
// ==================

/**
 * @brief Ensures that the JSON matches the format expected by read/write functions.
 */
static void json_ensure_format(json &j)
{
    if (!j.is_object()) j = json::object();

    // setup key namespaces
    if (!j["core"].is_object()) j["core"] = json::object();
    if (!j["frontend"].is_object()) j["frontend"] = json::object();
}

template <class T> static json convert_to_json(const T &value)
{
    return json(value);
}

static json convert_to_json(const std::wstring &value)
{
    return json(IOUtils::to_utf8_string(value));
}

static json convert_to_json(const std::vector<std::wstring> &value)
{
    return value | std::views::transform([](const std::wstring &ws) {
               // convert elements to UTF-8, then wrap in JSON
               return json(IOUtils::to_utf8_string(ws));
           }) |
           std::ranges::to<json::array_t>();
}

static json convert_to_json(const std::map<std::wstring, std::wstring> &value)
{
    return value | std::views::transform([](const std::pair<std::wstring, std::wstring> &ws_pair) {
               // convert elements to UTF-8, wrap the value in JSON
               return std::pair(IOUtils::to_utf8_string(ws_pair.first), json(IOUtils::to_utf8_string(ws_pair.second)));
           }) |
           std::ranges::to<json::object_t>();
}

static json convert_to_json(const std::map<std::wstring, Hotkey::t_hotkey> &value)
{
    return value | std::views::transform([](const std::pair<std::wstring, Hotkey::t_hotkey> &mapping) {
               // translate name
               auto name = IOUtils::to_utf8_string(mapping.first);
               // translate hotkey
               const auto &hotkey = mapping.second;
               auto object = json::object({{"assigned", hotkey.assigned},
                                           {"key", hotkey.key},
                                           {"ctrl", hotkey.ctrl},
                                           {"shift", hotkey.shift},
                                           {"alt", hotkey.alt}});
               return std::pair(std::move(name), std::move(object));
           }) |
           std::ranges::to<json::object_t>();
}

template <class T> static bool convert_from_json(const json &j, T &value)
{
    if (j.is_null()) return false;
    value = j.get<T>();
    return true;
}

static bool convert_from_json(const json &j, std::wstring &value)
{
    if (j.is_null()) return false;
    value = IOUtils::to_wide_string(j.get<std::string>());
    return true;
}

static bool convert_from_json(const json &j, std::vector<std::wstring> &value)
{
    if (!j.is_array()) return false;

    value = j.get_ref<const json::array_t &>() |
            std::views::transform([](const json &str) { return IOUtils::to_wide_string(str.get<std::string>()); }) |
            std::ranges::to<std::vector>();
    return true;
}

static bool convert_from_json(const json &j, std::map<std::wstring, std::wstring> &value)
{
    if (!j.is_object()) return false;

    value = j.get_ref<const json::object_t &>() |
            std::views::transform([](const std::pair<std::string, json> &str_pair) {
                return std::pair(IOUtils::to_wide_string(str_pair.first),
                                 IOUtils::to_wide_string(str_pair.second.get<std::string>()));
            }) |
            std::ranges::to<std::map>();
    return true;
}

static bool convert_from_json(const json &j, std::map<std::wstring, Hotkey::t_hotkey> &value)
{
    if (!j.is_object()) return false;

    value = j.get_ref<const json::object_t &>() |
            std::views::transform([](const std::pair<std::string, json> &str_pair) {
                auto name = IOUtils::to_wide_string(str_pair.first);
                const auto &hotkey_json = str_pair.second;
                auto hotkey = Hotkey::t_hotkey{};

                hotkey.assigned = hotkey_json["assigned"];
                hotkey.key = hotkey_json["key"];
                hotkey.ctrl = hotkey_json["ctrl"];
                hotkey.shift = hotkey_json["shift"];
                hotkey.alt = hotkey_json["alt"];
                return std::pair(std::move(name), hotkey);
            }) |
            std::ranges::to<std::map>();
    return true;
}

static void json_read_file(json &j)
{
    g_config = get_default_config();

#define CORE_VALUE(name) convert_from_json(j["core"][#name], g_config.core.name);
#define FRONTEND_VALUE(name) convert_from_json(j["frontend"][#name], g_config.name);

    CORE_VALUE(total_rerecords)
    CORE_VALUE(total_frames)
    CORE_VALUE(core_type)
    CORE_VALUE(fps_modifier)
    CORE_VALUE(rom_cache_size)
    CORE_VALUE(st_screenshot)
    CORE_VALUE(is_movie_loop_enabled)
    CORE_VALUE(is_reset_recording_enabled)
    CORE_VALUE(seek_savestate_interval)
    CORE_VALUE(seek_savestate_max_count)
    CORE_VALUE(st_undo_load)
    CORE_VALUE(use_summercart)
    CORE_VALUE(wii_vc_emulation)
    CORE_VALUE(rcp_lag_emulation)
    CORE_VALUE(cpu_cf)
    CORE_VALUE(rcp_lag_factor)
    CORE_VALUE(float_exception_emulation)
    CORE_VALUE(c_eq_s_nan_accurate)
    CORE_VALUE(accurate_rdp_completion)
    CORE_VALUE(is_audio_delay_enabled)
    CORE_VALUE(is_compiled_jump_enabled)
    CORE_VALUE(vcr_readonly)
    CORE_VALUE(vcr_backups)
    CORE_VALUE(vcr_write_extended_format)
    CORE_VALUE(wait_at_movie_end)
    CORE_VALUE(max_lag)

    FRONTEND_VALUE(ignored_version)
    FRONTEND_VALUE(theme)
    FRONTEND_VALUE(st_slot)
    FRONTEND_VALUE(is_unfocused_pause_enabled)
    FRONTEND_VALUE(is_statusbar_enabled)
    FRONTEND_VALUE(statusbar_scale_up)
    FRONTEND_VALUE(statusbar_layout)
    FRONTEND_VALUE(rom_directory)
    FRONTEND_VALUE(plugins_directory)
    FRONTEND_VALUE(saves_directory)
    FRONTEND_VALUE(screenshots_directory)
    FRONTEND_VALUE(backups_directory)
    FRONTEND_VALUE(recent_rom_paths)
    FRONTEND_VALUE(is_recent_rom_paths_frozen)
    FRONTEND_VALUE(recent_movie_paths)
    FRONTEND_VALUE(is_recent_movie_paths_frozen)
    FRONTEND_VALUE(is_rombrowser_recursion_enabled)
    FRONTEND_VALUE(capture_mode)
    FRONTEND_VALUE(stop_capture_at_movie_end)
    FRONTEND_VALUE(presenter_type)
    FRONTEND_VALUE(lazy_renderer_init)
    FRONTEND_VALUE(encoder_type)
    FRONTEND_VALUE(capture_delay)
    FRONTEND_VALUE(ffmpeg_options)
    FRONTEND_VALUE(ffmpeg_path)
    FRONTEND_VALUE(synchronization_mode)
    FRONTEND_VALUE(keep_default_working_directory)
    FRONTEND_VALUE(lua_script_path)
    FRONTEND_VALUE(recent_lua_script_paths)
    FRONTEND_VALUE(is_recent_scripts_frozen)
    FRONTEND_VALUE(piano_roll_constrain_edit_to_column)
    FRONTEND_VALUE(piano_roll_undo_stack_size)
    FRONTEND_VALUE(piano_roll_keep_selection_visible)
    FRONTEND_VALUE(piano_roll_keep_playhead_visible)
    FRONTEND_VALUE(selected_video_plugin)
    FRONTEND_VALUE(selected_audio_plugin)
    FRONTEND_VALUE(selected_input_plugin)
    FRONTEND_VALUE(selected_rsp_plugin)
    FRONTEND_VALUE(last_movie_type)
    FRONTEND_VALUE(last_movie_author)
    FRONTEND_VALUE(window_x)
    FRONTEND_VALUE(window_y)
    FRONTEND_VALUE(window_width)
    FRONTEND_VALUE(window_height)
    FRONTEND_VALUE(rombrowser_column_widths)
    FRONTEND_VALUE(rombrowser_sort_ascending)
    FRONTEND_VALUE(rombrowser_sorted_column)
    FRONTEND_VALUE(persistent_folder_paths)
    FRONTEND_VALUE(settings_tab)
    FRONTEND_VALUE(vcr_0_index)
    FRONTEND_VALUE(increment_slot)
    FRONTEND_VALUE(automatic_update_checking)
    FRONTEND_VALUE(silent_mode)
    FRONTEND_VALUE(seeker_value)
    FRONTEND_VALUE(multi_frame_advance_count)
    FRONTEND_VALUE(silent_mode_dialog_choices)
    FRONTEND_VALUE(trusted_lua_paths)
    FRONTEND_VALUE(lua_paths)
    FRONTEND_VALUE(hotkeys)
    FRONTEND_VALUE(inital_hotkeys)

#undef CORE_VALUE
#undef FRONTEND_VALUE
}

static void json_write_file(json &j)
{
#define CORE_VALUE(name) j["core"][#name] = convert_to_json(g_config.core.name);
#define FRONTEND_VALUE(name) j["frontend"][#name] = convert_to_json(g_config.name);

    CORE_VALUE(total_rerecords)
    CORE_VALUE(total_frames)
    CORE_VALUE(core_type)
    CORE_VALUE(fps_modifier)
    CORE_VALUE(rom_cache_size)
    CORE_VALUE(st_screenshot)
    CORE_VALUE(is_movie_loop_enabled)
    CORE_VALUE(is_reset_recording_enabled)
    CORE_VALUE(seek_savestate_interval)
    CORE_VALUE(seek_savestate_max_count)
    CORE_VALUE(st_undo_load)
    CORE_VALUE(use_summercart)
    CORE_VALUE(wii_vc_emulation)
    CORE_VALUE(rcp_lag_emulation)
    CORE_VALUE(cpu_cf)
    CORE_VALUE(rcp_lag_factor)
    CORE_VALUE(float_exception_emulation)
    CORE_VALUE(c_eq_s_nan_accurate)
    CORE_VALUE(accurate_rdp_completion)
    CORE_VALUE(is_audio_delay_enabled)
    CORE_VALUE(is_compiled_jump_enabled)
    CORE_VALUE(vcr_readonly)
    CORE_VALUE(vcr_backups)
    CORE_VALUE(vcr_write_extended_format)
    CORE_VALUE(wait_at_movie_end)
    CORE_VALUE(max_lag)

    FRONTEND_VALUE(ignored_version)
    FRONTEND_VALUE(theme)
    FRONTEND_VALUE(st_slot)
    FRONTEND_VALUE(is_unfocused_pause_enabled)
    FRONTEND_VALUE(is_statusbar_enabled)
    FRONTEND_VALUE(statusbar_scale_up)
    FRONTEND_VALUE(statusbar_layout)
    FRONTEND_VALUE(rom_directory)
    FRONTEND_VALUE(plugins_directory)
    FRONTEND_VALUE(saves_directory)
    FRONTEND_VALUE(screenshots_directory)
    FRONTEND_VALUE(backups_directory)
    FRONTEND_VALUE(recent_rom_paths)
    FRONTEND_VALUE(is_recent_rom_paths_frozen)
    FRONTEND_VALUE(recent_movie_paths)
    FRONTEND_VALUE(is_recent_movie_paths_frozen)
    FRONTEND_VALUE(is_rombrowser_recursion_enabled)
    FRONTEND_VALUE(capture_mode)
    FRONTEND_VALUE(stop_capture_at_movie_end)
    FRONTEND_VALUE(presenter_type)
    FRONTEND_VALUE(lazy_renderer_init)
    FRONTEND_VALUE(encoder_type)
    FRONTEND_VALUE(capture_delay)
    FRONTEND_VALUE(ffmpeg_options)
    FRONTEND_VALUE(ffmpeg_path)
    FRONTEND_VALUE(synchronization_mode)
    FRONTEND_VALUE(keep_default_working_directory)
    FRONTEND_VALUE(lua_script_path)
    FRONTEND_VALUE(recent_lua_script_paths)
    FRONTEND_VALUE(is_recent_scripts_frozen)
    FRONTEND_VALUE(piano_roll_constrain_edit_to_column)
    FRONTEND_VALUE(piano_roll_undo_stack_size)
    FRONTEND_VALUE(piano_roll_keep_selection_visible)
    FRONTEND_VALUE(piano_roll_keep_playhead_visible)
    FRONTEND_VALUE(selected_video_plugin)
    FRONTEND_VALUE(selected_audio_plugin)
    FRONTEND_VALUE(selected_input_plugin)
    FRONTEND_VALUE(selected_rsp_plugin)
    FRONTEND_VALUE(last_movie_type)
    FRONTEND_VALUE(last_movie_author)
    FRONTEND_VALUE(window_x)
    FRONTEND_VALUE(window_y)
    FRONTEND_VALUE(window_width)
    FRONTEND_VALUE(window_height)
    FRONTEND_VALUE(rombrowser_column_widths)
    FRONTEND_VALUE(rombrowser_sort_ascending)
    FRONTEND_VALUE(rombrowser_sorted_column)
    FRONTEND_VALUE(persistent_folder_paths)
    FRONTEND_VALUE(settings_tab)
    FRONTEND_VALUE(vcr_0_index)
    FRONTEND_VALUE(increment_slot)
    FRONTEND_VALUE(automatic_update_checking)
    FRONTEND_VALUE(silent_mode)
    FRONTEND_VALUE(seeker_value)
    FRONTEND_VALUE(multi_frame_advance_count)
    FRONTEND_VALUE(silent_mode_dialog_choices)
    FRONTEND_VALUE(trusted_lua_paths)
    FRONTEND_VALUE(lua_paths)
    FRONTEND_VALUE(hotkeys)
    FRONTEND_VALUE(inital_hotkeys)

#undef CORE_VALUE
#undef FRONTEND_VALUE
}

#pragma endregion

static std::filesystem::path get_config_path()
{
    return IOUtils::config_path() / CONFIG_FILE_NAME;
}

/**
 * \brief Modifies the config to apply value limits and other constraints.
 */
static void config_patch(t_config &cfg)
{
    if (!MonitorFromPoint({cfg.window_x, cfg.window_y}, MONITOR_DEFAULTTONULL))
    {
        cfg.window_x = g_default_config.window_x;
        cfg.window_y = g_default_config.window_y;
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

    for (const auto &pair : DIALOG_SILENT_MODE_CHOICES)
    {
        const auto key = IOUtils::to_wide_string(pair.first);
        if (!cfg.silent_mode_dialog_choices.contains(key))
        {
            cfg.silent_mode_dialog_choices[key] = std::to_wstring(pair.second);
        }
    }

    // HACK: Wine doesn't implement DComp well enough yet, so force GDI
    if (g_main_ctx.wine) cfg.presenter_type = (int32_t)t_config::PresenterType::GDI;

    cfg.core.cpu_cf = std::isfinite(cfg.core.cpu_cf) ? std::max(cfg.core.cpu_cf, 0.0) : 0.0;
    cfg.core.rcp_lag_factor = std::isfinite(cfg.core.rcp_lag_factor) ? std::max(cfg.core.rcp_lag_factor, 0.0) : 0.0;
}

void Config::init()
{
}

void Config::save()
{
    Messenger::broadcast(Messenger::Message::ConfigSaving, nullptr);

    config_patch(g_config);

    json j{};
    json_ensure_format(j);
    json_write_file(j);

    std::ofstream ofs_file(get_config_path());
    ofs_file << std::setw(2) << j;
}

void Config::apply_and_save()
{
    ActionManager::begin_batch_work();
    for (const auto &[action, hotkey] : g_config.hotkeys)
    {
        ActionManager::associate_hotkey(action, hotkey, true);
    }
    ActionManager::end_batch_work();

    save();
}

void Config::load()
{
    if (std::filesystem::exists(get_config_path()))
    {
        json j;
        try
        {
            std::ifstream ifs_file(get_config_path());
            ifs_file >> j;
            json_ensure_format(j);
            json_read_file(j);
        }
        catch (const std::exception &e)
        {
            g_view_logger->info("[CONFIG] Failed to load config, using defaults...");
            g_config = get_default_config();
            save();
        }
    }
    else if (std::filesystem::exists(get_legacy_config_path()))
    {
        // TODO: Remove legacy config support with version 1.6.0
        mINI::INIFile file(get_legacy_config_path().string());
        mINI::INIStructure ini;
        file.read(ini);

        handle_config_ini(true, ini);
        migrate_config_ini(g_config, ini);
    }
    else
    {
        g_view_logger->info("[CONFIG] Default config file does not exist. Generating...");
        g_config = get_default_config();
        save();
    }

    config_patch(g_config);

    Messenger::broadcast(Messenger::Message::ConfigLoaded, nullptr);
}

std::filesystem::path Config::rom_directory()
{
    return IOUtils::exe_path().parent_path() / g_config.rom_directory;
}

std::filesystem::path Config::plugin_directory()
{
    return IOUtils::exe_path().parent_path() / g_config.plugins_directory;
}

std::filesystem::path Config::save_directory()
{
    return IOUtils::exe_path().parent_path() / g_config.saves_directory;
}

std::filesystem::path Config::screenshot_directory()
{
    return IOUtils::exe_path().parent_path() / g_config.screenshots_directory;
}

std::filesystem::path Config::backup_directory()
{
    return IOUtils::exe_path().parent_path() / g_config.backups_directory;
}

std::filesystem::path Config::logs_directory()
{
    return IOUtils::exe_path().parent_path() / L"logs";
}
