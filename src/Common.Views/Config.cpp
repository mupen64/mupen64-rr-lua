/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/App.hpp>
#include <nlohmann/json.hpp>
#include <IOUtils.hpp>
#include <Common.Views/Config.hpp>
#include <Common.Views/Messages.hpp>
#include <m64rr/API.hpp>
#include <Common.Views/ActionManager.hpp>

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

static std::unordered_map<std::string, size_t> get_merged_silent_mode_dialog_choices()
{
    const std::unordered_map<std::string, size_t> BASE_CHOICES = {
        {CORE_DLG_FLOAT_EXCEPTION, 0},      {CORE_DLG_ST_HASH_MISMATCH, 0},      {CORE_DLG_ST_UNFREEZE_WARNING, 0},
        {CORE_DLG_ST_NOT_FROM_MOVIE, 0},    {CORE_DLG_VCR_RAWDATA_WARNING, 0},   {CORE_DLG_VCR_GENERAL_SYNC_WARNING, 0},
        {CORE_DLG_VCR_ROM_NAME_WARNING, 0}, {CORE_DLG_VCR_ROM_CCODE_WARNING, 0}, {CORE_DLG_VCR_ROM_CRC_WARNING, 0},
        {CORE_DLG_VCR_CHEAT_LOAD_ERROR, 0},
    };

    std::unordered_map<std::string, size_t> choices;
    for (const auto &pair : BASE_CHOICES) choices.insert(pair);
    for (const auto &pair : get_silent_mode_dialog_choices()) choices.insert(pair);
    return choices;
}

const t_config g_default_config = get_default_config();

static t_config get_default_config()
{
    t_config config = {};

    for (const auto &pair : get_merged_silent_mode_dialog_choices())
    {
        config.silent_mode_dialog_choices[IOUtils::to_wide_string(pair.first)] = std::to_wstring(pair.second);
    }

    return config;
}

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

static json convert_to_json(const std::map<std::wstring, Hotkey> &value)
{
    return value | std::views::transform([](const std::pair<std::wstring, Hotkey> &mapping) {
               // translate name
               auto name = IOUtils::to_utf8_string(mapping.first);
               // translate hotkey
               const auto &hotkey = mapping.second;
               auto object = json::object(
                   {{"trigger", hotkey.trigger}, {"ctrl", hotkey.ctrl}, {"shift", hotkey.shift}, {"alt", hotkey.alt}});
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

static bool convert_from_json(const json &j, std::map<std::wstring, Hotkey> &value)
{
    if (!j.is_object()) return false;

    value = j.get_ref<const json::object_t &>() |
            std::views::transform([](const std::pair<std::string, json> &str_pair) {
                auto name = IOUtils::to_wide_string(str_pair.first);
                const auto &hotkey_json = str_pair.second;
                auto hotkey = Hotkey{};

                // TODO: Remove in 1.6.0, this is just temporary
                const auto view_converted = app_json_to_hotkey(hotkey_json);
                if (view_converted)
                    hotkey = *view_converted;
                else
                    hotkey.trigger = hotkey_json["trigger"];

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
    CORE_VALUE(st_lz4)
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
    CORE_VALUE(st_lz4)
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

    for (const auto &pair : get_merged_silent_mode_dialog_choices())
    {
        const auto key = IOUtils::to_wide_string(pair.first);
        if (!cfg.silent_mode_dialog_choices.contains(key))
        {
            cfg.silent_mode_dialog_choices[key] = std::to_wstring(pair.second);
        }
    }

    Messenger::broadcast<Messenger::Message::ConfigNeedsPatching>(cfg);

    cfg.core.cpu_cf = std::isfinite(cfg.core.cpu_cf) ? std::max(cfg.core.cpu_cf, 0.0) : 0.0;
    cfg.core.rcp_lag_factor = std::isfinite(cfg.core.rcp_lag_factor) ? std::max(cfg.core.rcp_lag_factor, 0.0) : 0.0;
}

void Config::init()
{
}

void Config::save()
{
    Messenger::broadcast<Messenger::Message::ConfigSaving>();

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
        const auto uaction = IOUtils::to_utf8_string(action);
        ActionManager::associate_hotkey(uaction, hotkey, true);
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
    else
    {
        g_view_logger->info("[CONFIG] Default config file does not exist. Generating...");
        g_config = get_default_config();
        save();
    }

    config_patch(g_config);

    Messenger::broadcast<Messenger::Message::ConfigLoaded>();
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
