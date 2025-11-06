/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <DialogService.h>
#include <Messenger.h>
#include <ThreadPool.h>
#include <Plugin.h>
#include <capture/EncodingManager.h>
#include <components/AppActions.h>
#include <components/CLI.h>
#include <components/Cheats.h>
#include <components/CommandPalette.h>
#include <components/Compare.h>
#include <components/ConfigDialog.h>
#include <components/CoreDbg.h>
#include <components/FilePicker.h>
#include <components/LuaDialog.h>
#include <components/MovieDialog.h>
#include <components/PianoRoll.h>
#include <components/RecentItems.h>
#include <components/RomBrowser.h>
#include <components/Seeker.h>
#include <components/Statusbar.h>
#include <components/UpdateChecker.h>

bool confirm_user_exit()
{
    BetterEmulationLock lock;

    if (g_config.silent_mode)
    {
        return true;
    }

    std::wstring final_message;
    std::vector<std::pair<bool, std::wstring>> messages = {
        {g_main_ctx.core_ctx->vcr_get_task() == task_recording, L"Movie recording"},
        {EncodingManager::is_capturing(), L"Capture"},
        {g_main_ctx.core_ctx->tl_active(), L"Trace logging"}};

    std::vector<std::wstring> active_messages;
    for (const auto &[is_active, msg] : messages)
    {
        if (!is_active)
        {
            continue;
        }

        active_messages.push_back(msg);
    }

    if (active_messages.empty())
    {
        return true;
    }

    for (size_t i = 0; i < active_messages.size(); ++i)
    {
        final_message += active_messages[i];
        if (i < active_messages.size() - 1)
        {
            final_message += L", ";
        }
    }
    final_message += L" is running. Are you sure you want to close the ROM?";

    const bool result =
        DialogService::show_ask_dialog(VIEW_DLG_CLOSE_ROM_WARNING, final_message.c_str(), L"Close ROM", true);

    return result;
}

void AppActions::update_core_fast_forward()
{
    g_main_ctx.core_ctx->vr_set_fast_forward(g_main_ctx.fast_forward || g_main_ctx.core_ctx->vcr_is_seeking() ||
                                             CLI::wants_fast_forward() || Compare::active());
}

void AppActions::load_rom_from_path(const std::wstring &path)
{
    ThreadPool::submit_task([=] {
        const auto result = g_main_ctx.core_ctx->vr_start_rom(path);
        show_error_dialog_for_result(result);
    });
}

#pragma region Action Callbacks

static void stub()
{
    DialogService::show_dialog(L"ActionManager::stub", L"Stub", fsvc_error);
}

#pragma region File

static void load_rom()
{
    BetterEmulationLock lock;

    const auto path = FilePicker::show_open_dialog(L"o_rom", g_main_ctx.hwnd,
                                                   L"*.n64;*.z64;*.v64;*.rom;*.bin;*.zip;*.usa;*.eur;*.jap");

    if (path.empty())
    {
        return;
    }

    AppActions::load_rom_from_path(path);
}

static void load_recent_rom(size_t i)
{
    if (g_config.recent_rom_paths.size() <= i)
    {
        return;
    }

    const auto path = g_config.recent_rom_paths[i];

    AppActions::load_rom_from_path(path);
}

static void close_rom()
{
    if (!confirm_user_exit()) return;

    ThreadPool::submit_task(
        [] {
            const auto result = g_main_ctx.core_ctx->vr_close_rom(true);
            show_error_dialog_for_result(result);
        },
        ASYNC_KEY_CLOSE_ROM);
}

static void reset_rom()
{
    const bool reset_will_continue_recording =
        g_config.core.is_reset_recording_enabled && g_main_ctx.core_ctx->vcr_get_task() == task_recording;

    if (!reset_will_continue_recording && !confirm_user_exit()) return;

    ThreadPool::submit_task(
        [] {
            const auto result = g_main_ctx.core_ctx->vr_reset_rom(false, true);
            show_error_dialog_for_result(result);
        },
        ASYNC_KEY_RESET_ROM);
}

static void refresh_rombrowser()
{
    if (!g_main_ctx.core_ctx->vr_get_launched())
    {
        RomBrowser::build();
    }
}

static void exit_app()
{
    DestroyWindow(g_main_ctx.hwnd);
}

#pragma endregion

#pragma region Emulation

static void pause_emu()
{
    // FIXME: While this is a beautiful and clean solution, there has to be a better way to handle this
    if (g_main_ctx.in_menu_loop)
    {
        if (g_main_ctx.paused_before_menu)
        {
            g_main_ctx.core_ctx->vr_resume_emu();
            g_main_ctx.paused_before_menu = false;
            return;
        }
        g_main_ctx.paused_before_menu = true;
        g_main_ctx.core_ctx->vr_pause_emu();
    }
    else
    {
        if (g_main_ctx.core_ctx->vr_get_paused())
        {
            g_main_ctx.core_ctx->vr_resume_emu();
            return;
        }
        g_main_ctx.core_ctx->vr_pause_emu();
    }
}

static void increment_speed(const int value)
{
    g_config.core.fps_modifier = clamp(g_config.core.fps_modifier + value, 5, 1000);
    g_main_ctx.core_ctx->vr_on_speed_modifier_changed();
    Messenger::broadcast(Messenger::Message::SpeedModifierChanged, g_config.core.fps_modifier);
}

static void speed_down()
{
    increment_speed(-5);
}

static void speed_up()
{
    increment_speed(5);
}

static void speed_reset()
{
    g_config.core.fps_modifier = 100;
    g_main_ctx.core_ctx->vr_on_speed_modifier_changed();
    Messenger::broadcast(Messenger::Message::SpeedModifierChanged, g_config.core.fps_modifier);
}

static void frame_advance()
{
    g_main_ctx.fast_forward = false;
    AppActions::update_core_fast_forward();

    g_main_ctx.core_ctx->vr_frame_advance(1);
    g_main_ctx.core_ctx->vr_resume_emu();
}

static void multi_frame_advance()
{
    if (g_config.multi_frame_advance_count > 0)
    {
        g_main_ctx.core_ctx->vr_frame_advance(g_config.multi_frame_advance_count);
    }
    else
    {
        ThreadPool::submit_task([] {
            const auto result =
                g_main_ctx.core_ctx->vcr_begin_seek(std::to_string(g_config.multi_frame_advance_count), true);
            show_error_dialog_for_result(result);
        });
    }
    g_main_ctx.core_ctx->vr_resume_emu();
}

static void fastforward_enable()
{
    g_main_ctx.fast_forward = true;
    Messenger::broadcast(Messenger::Message::FastForwardNeedsUpdate, nullptr);
}

static void fastforward_disable()
{
    g_main_ctx.fast_forward = false;
    Messenger::broadcast(Messenger::Message::FastForwardNeedsUpdate, nullptr);
}

static bool fastforward_active()
{
    return g_main_ctx.fast_forward;
}

static void gs_button_enable()
{
    g_main_ctx.core_ctx->vr_set_gs_button(true);
    ActionManager::notify_active_changed(AppActions::GS_BUTTON);
}

static void gs_button_disable()
{
    g_main_ctx.core_ctx->vr_set_gs_button(false);
    ActionManager::notify_active_changed(AppActions::GS_BUTTON);
}

static bool gs_button_active()
{
    if (!g_main_ctx.core_ctx->vr_get_core_executing())
    {
        return false;
    }
    return g_main_ctx.core_ctx->vr_get_gs_button();
}

static void save_slot()
{
    g_main_ctx.core_ctx->vr_wait_increment();
    if (g_config.increment_slot)
    {
        g_config.st_slot >= 9 ? g_config.st_slot = 0 : g_config.st_slot++;
        Messenger::broadcast(Messenger::Message::SlotChanged, (size_t)g_config.st_slot);
    }
    ThreadPool::submit_task([=] {
        g_main_ctx.core_ctx->vr_wait_decrement();
        g_main_ctx.core_ctx->st_do_file(get_st_with_slot_path(g_config.st_slot), core_st_job_save, nullptr, false);
    });
}

static void load_slot()
{
    g_main_ctx.core_ctx->vr_wait_increment();
    ThreadPool::submit_task([=] {
        g_main_ctx.core_ctx->vr_wait_decrement();
        g_main_ctx.core_ctx->st_do_file(get_st_with_slot_path(g_config.st_slot), core_st_job_load, nullptr, false);
    });
}

static void save_state_as()
{
    BetterEmulationLock lock;

    const auto path = FilePicker::show_save_dialog(L"s_savestate", g_main_ctx.hwnd, L"*.st;*.savestate");
    if (path.empty())
    {
        return;
    }

    g_main_ctx.core_ctx->vr_wait_increment();
    ThreadPool::submit_task([=] {
        g_main_ctx.core_ctx->vr_wait_decrement();
        (void)g_main_ctx.core_ctx->st_do_file(path, core_st_job_save, nullptr, false);
    });
}

static void load_state_as()
{
    BetterEmulationLock lock;

    const auto path = FilePicker::show_open_dialog(
        L"o_state", g_main_ctx.hwnd,
        L"*.st;*.savestate;*.st0;*.st1;*.st2;*.st3;*.st4;*.st5;*.st6;*.st7;*.st8;*.st9,*.st10");
    if (path.empty())
    {
        return;
    }

    g_main_ctx.core_ctx->vr_wait_increment();
    ThreadPool::submit_task([=] {
        g_main_ctx.core_ctx->vr_wait_decrement();
        (void)g_main_ctx.core_ctx->st_do_file(path, core_st_job_load, nullptr, false);
    });
}

static void undo_load_state()
{
    g_main_ctx.core_ctx->vr_wait_increment();
    ThreadPool::submit_task([=] {
        g_main_ctx.core_ctx->vr_wait_decrement();

        std::vector<uint8_t> buf{};
        g_main_ctx.core_ctx->st_get_undo_savestate(buf);

        if (buf.empty())
        {
            Statusbar::post(L"No load to undo");
            return;
        }

        (void)g_main_ctx.core_ctx->st_do_memory(
            buf, core_st_job_load,
            [](const core_st_callback_info &info, auto) {
                if (info.result == Res_Ok)
                {
                    Statusbar::post(L"Undid load");
                    return;
                }

                if (info.result == Res_Cancelled)
                {
                    return;
                }

                Statusbar::post(L"Failed to undo load");
            },
            false);
    });
}

static void multi_frame_advance_increment()
{
    g_config.multi_frame_advance_count++;
    if (g_config.multi_frame_advance_count == 0)
    {
        g_config.multi_frame_advance_count++;
    }
    Messenger::broadcast(Messenger::Message::MultiFrameAdvanceCountChanged, std::nullopt);
}

static void multi_frame_advance_decrement()
{
    g_config.multi_frame_advance_count--;
    if (g_config.multi_frame_advance_count == 0)
    {
        g_config.multi_frame_advance_count--;
    }
    Messenger::broadcast(Messenger::Message::MultiFrameAdvanceCountChanged, std::nullopt);
}

static void multi_frame_advance_reset()
{
    g_config.multi_frame_advance_count = g_default_config.multi_frame_advance_count;
    Messenger::broadcast(Messenger::Message::MultiFrameAdvanceCountChanged, std::nullopt);
}

static void set_save_slot(const size_t slot)
{
    g_config.st_slot = slot;
    Messenger::broadcast(Messenger::Message::SlotChanged, static_cast<size_t>(g_config.st_slot));
}

#pragma endregion

#pragma region Options

static void toggle_fullscreen()
{
    g_plugin_funcs.video_change_window();
    g_main_ctx.fullscreen ^= true;
    ActionManager::notify_active_changed(AppActions::FULL_SCREEN);
}

static bool fullscreen_active()
{
    return g_main_ctx.fullscreen;
}

static void show_plugin_settings_dialog(const std::unique_ptr<Plugin> &plugin)
{
    BetterEmulationLock lock;

    if (plugin != nullptr)
    {
        plugin->config(g_main_ctx.hwnd);
    }
}

static void show_video_plugin_settings()
{
    show_plugin_settings_dialog(Plugin::create(g_config.selected_video_plugin).second);
}

static void show_audio_plugin_settings()
{
    show_plugin_settings_dialog(Plugin::create(g_config.selected_audio_plugin).second);
}

static void show_input_plugin_settings()
{
    show_plugin_settings_dialog(Plugin::create(g_config.selected_input_plugin).second);
}

static void show_rsp_plugin_settings()
{
    show_plugin_settings_dialog(Plugin::create(g_config.selected_rsp_plugin).second);
}

static void toggle_statusbar()
{
    g_config.is_statusbar_enabled ^= true;
    Messenger::broadcast(Messenger::Message::StatusbarVisibilityChanged, (bool)g_config.is_statusbar_enabled);
}

static void show_settings_dialog()
{
    BetterEmulationLock lock;
    ConfigDialog::show_app_settings();
}

#pragma endregion

#pragma region Movie

static void start_movie_recording()
{
    BetterEmulationLock lock;

    auto movie_dialog_result = MovieDialog::show(false);

    if (movie_dialog_result.path.empty())
    {
        return;
    }

    g_main_ctx.core_ctx->vr_wait_increment();
    g_main_ctx.core.submit_task([=] {
        auto vcr_result =
            g_main_ctx.core_ctx->vcr_start_record(movie_dialog_result.path, movie_dialog_result.start_flag,
                                                  IOUtils::to_utf8_string(movie_dialog_result.author),
                                                  IOUtils::to_utf8_string(movie_dialog_result.description));
        g_main_ctx.core_ctx->vr_wait_decrement();
        if (!show_error_dialog_for_result(vcr_result))
        {
            g_config.last_movie_author = movie_dialog_result.author;
            Statusbar::post(L"Recording replay");
        }
    });
}

static void continue_movie_recording()
{
    g_main_ctx.core_ctx->vr_wait_increment();
    g_main_ctx.core.submit_task([] {
        const auto result = g_main_ctx.core_ctx->vcr_continue_recording();
        g_main_ctx.core_ctx->vr_wait_decrement();
        show_error_dialog_for_result(result);
    });
}

static void start_movie_playback()
{
    BetterEmulationLock lock;

    auto result = MovieDialog::show(true);

    if (result.path.empty())
    {
        return;
    }

    g_main_ctx.core_ctx->vcr_replace_author_info(result.path, IOUtils::to_utf8_string(result.author),
                                                 IOUtils::to_utf8_string(result.description));

    g_config.core.pause_at_frame = result.pause_at;
    g_config.core.pause_at_last_frame = result.pause_at_last;

    ThreadPool::submit_task([result] {
        auto vcr_result = g_main_ctx.core_ctx->vcr_start_playback(result.path);
        show_error_dialog_for_result(vcr_result);
    });
}

static void stop_movie()
{
    g_main_ctx.core_ctx->vr_wait_increment();
    g_main_ctx.core.submit_task([] {
        g_main_ctx.core_ctx->vcr_stop_all();
        g_main_ctx.core_ctx->vr_wait_decrement();
    });
}

static void create_movie_backup()
{
    const auto result = g_main_ctx.core_ctx->vcr_write_backup();
    show_error_dialog_for_result(result);
}

static void load_recent_movie(size_t i)
{
    if (g_config.recent_movie_paths.size() <= i)
    {
        return;
    }

    const auto path = g_config.recent_movie_paths[i];

    g_config.core.vcr_readonly = true;
    Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)g_config.core.vcr_readonly);
    ThreadPool::submit_task(
        [=] {
            const auto result = g_main_ctx.core_ctx->vcr_start_playback(path);
            show_error_dialog_for_result(result);
        },
        ASYNC_KEY_PLAY_MOVIE);
}

static void toggle_movie_loop()
{
    g_config.core.is_movie_loop_enabled ^= true;
    Messenger::broadcast(Messenger::Message::MovieLoopChanged, (bool)g_config.core.is_movie_loop_enabled);
}

static void toggle_readonly()
{
    g_config.core.vcr_readonly ^= true;
    Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)g_config.core.vcr_readonly);
}

static void toggle_wait_at_movie_end()
{
    g_config.core.wait_at_movie_end ^= true;
    ActionManager::notify_active_changed(AppActions::WAIT_AT_MOVIE_END);
}

#pragma endregion

#pragma region Utilities

static void show_ram_start()
{
    BetterEmulationLock lock;

    wchar_t ram_start[20] = {0};
    wsprintfW(ram_start, L"0x%p", static_cast<void *>(g_main_ctx.core_ctx->rdram));

    wchar_t proc_name[MAX_PATH] = {0};
    GetModuleFileName(NULL, proc_name, MAX_PATH);

    const auto str = std::format(L"The RAM start is {}.\r\nHow would you like to proceed?", ram_start);

    const auto result = DialogService::show_multiple_choice_dialog(
        VIEW_DLG_RAMSTART, {L"Copy STROOP config line", L"Close"}, str.c_str(), L"Core Information", fsvc_information);

    if (result == 0)
    {
        auto exe_filename = IOUtils::exe_path_cached().stem();

        const auto stroop_line = std::format(L"<Emulator name=\"Mupen 5.0 RR\" processName=\"{}\" ramStart=\"{}\" "
                                             L"endianness=\"little\" autoDetect=\"true\"/>",
                                             exe_filename.c_str(), ram_start);
        copy_to_clipboard(g_main_ctx.hwnd, stroop_line);
    }
}

static void show_statistics()
{
    BetterEmulationLock lock;

    auto str = std::format(L"Total playtime: {}\r\nTotal rerecords: {}",
                           format_duration(g_config.core.total_frames / 30), g_config.core.total_rerecords);

    MessageBox(g_main_ctx.hwnd, str.c_str(), L"Statistics", MB_ICONINFORMATION);
}

static void stop_tracelog()
{
    if (g_main_ctx.core_ctx->tl_active())
    {
        g_main_ctx.core_ctx->tl_stop();
    }
}

static void start_tracelog()
{
    stop_tracelog();

    auto path = FilePicker::show_save_dialog(L"s_tracelog", g_main_ctx.hwnd, L"*.log");

    if (path.empty())
    {
        return;
    }

    auto result = MessageBox(g_main_ctx.hwnd, L"Should the trace log be generated in a binary format?", L"Trace Logger",
                             MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

    g_main_ctx.core_ctx->tl_start(path, result == IDYES, false);
}

static void show_debugger()
{
    CoreDbg::show();
}

static void show_command_palette()
{
    BetterEmulationLock lock;
    CommandPalette::show();
}

static void show_cheat_dialog()
{
    BetterEmulationLock lock;
    Cheats::show();
}

static void show_seek_dialog()
{
    BetterEmulationLock lock;
    Seeker::show();
}

static void show_piano_roll()
{
    PianoRoll::show();
}

static void screenshot()
{
    g_plugin_funcs.video_capture_screen(Config::screenshot_directory().string().data());
}

static void start_capture(const bool ask_preset)
{
    if (!g_main_ctx.core_ctx->vr_get_launched())
    {
        return;
    }

    BetterEmulationLock lock;

    auto path = FilePicker::show_save_dialog(L"s_capture", g_main_ctx.hwnd, L"*.avi");
    if (path.empty())
    {
        return;
    }

    EncodingManager::start_capture(path, (t_config::EncoderType)g_config.encoder_type, ask_preset,
                                   [](const auto result) {
                                       if (result)
                                       {
                                           Statusbar::post(L"Capture started...");
                                       }
                                   });
}

static void start_capture_normal()
{
    start_capture(true);
}

static void start_capture_from_preset()
{
    start_capture(false);
}

static void stop_capture()
{
    EncodingManager::stop_capture([](const auto result) {
        if (result)
        {
            Statusbar::post(L"Capture stopped");
        }
    });
}

#pragma endregion

#pragma region Help

static void check_for_updates(const bool manual)
{
    ThreadPool::submit_task([=] { UpdateChecker::check(manual); });
}

static void check_for_updates_manual()
{
    check_for_updates(true);
}

static void show_about_dialog()
{
    BetterEmulationLock lock;
    const auto msg = L"Mupen64 - Advanced N64 TASing emulator."
                     L"\r\n"
                     L"\r\n"
                     L"Copyright ©️ 2025"
                     L"\r\n"
                     L"Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).";
    const auto result = DialogService::show_multiple_choice_dialog(VIEW_DLG_ABOUT, {L"Website", L"OK"}, msg, L"About",
                                                                   fsvc_information);

    if (result == 0)
    {
        ShellExecute(0, 0, L"https://mupen64.com", 0, 0, SW_SHOW);
    }
}

#pragma endregion

#pragma region Lua Script

static void show_lua_dialog()
{
    LuaDialog::show();
}

static void load_recent_script(size_t i)
{
    if (g_config.recent_lua_script_paths.size() <= i)
    {
        return;
    }

    const auto path = g_config.recent_lua_script_paths[i];

    LuaDialog::start_and_add_if_needed(path);
}

static void close_all_lua_scripts()
{
    LuaDialog::close_all();
}

#pragma endregion

#pragma endregion

#pragma region Enabled Getters

static bool enable_when_emu_launched()
{
    return g_main_ctx.core_ctx->vr_get_launched();
}

static bool disable_when_emu_launched()
{
    return !g_main_ctx.core_ctx->vr_get_launched();
}

static bool enable_when_emu_launched_and_vcr_active()
{
    return g_main_ctx.core_ctx->vr_get_launched() && g_main_ctx.core_ctx->vcr_get_task() != task_idle;
}

static bool enable_during_playback()
{
    const auto task = g_main_ctx.core_ctx->vcr_get_task();
    return g_main_ctx.core_ctx->vr_get_launched() && (task == task_playback || task == task_start_playback_from_reset ||
                                                      task == task_start_playback_from_snapshot);
}

static bool enable_when_emu_launched_and_capturing()
{
    return g_main_ctx.core_ctx->vr_get_launched() && EncodingManager::is_capturing();
}

static bool enable_when_emu_launched_and_core_is_pure_interpreter()
{
    return g_main_ctx.core_ctx->vr_get_launched() && g_config.core.core_type == 2;
}

static bool enable_when_tracelog_active()
{
    return g_main_ctx.core_ctx->tl_active();
}

static bool always_enabled()
{
    return true;
}

#pragma endregion

static void add_action_with_up(const std::wstring &path, const Hotkey::t_hotkey &default_hotkey,
                               const std::function<void()> &on_press, const std::function<void()> &on_release,
                               const std::function<bool()> &get_enabled = {},
                               const std::function<bool()> &get_active = {},
                               const std::function<std::wstring()> &get_display_name = {})
{
    bool success = ActionManager::add({
        .path = path,
        .on_press = on_press,
        .on_release = on_release,
        .get_display_name = get_display_name,
        .get_enabled = get_enabled,
        .get_active = get_active,
    });
    RT_ASSERT(success, std::format(L"Failed to add action for path '{}'.", path));

    success = ActionManager::associate_hotkey(path, default_hotkey, false);
    RT_ASSERT(success, std::format(L"Failed to associate hotkey for path '{}'.", path));
}

static void add_action(const std::wstring &path, const Hotkey::t_hotkey &default_hotkey,
                       const std::function<void()> &callback, const std::function<bool()> &get_enabled = {},
                       const std::function<bool()> &get_active = {},
                       const std::function<std::wstring()> &get_display_name = {})
{
    add_action_with_up(path, default_hotkey, callback, nullptr, get_enabled, get_active, get_display_name);
}

static void generate_path_recent_menu(const std::wstring &base_path, const Hotkey::t_hotkey &load_first_hotkey,
                                      std::vector<std::wstring> *paths, int32_t *frozen,
                                      const std::function<void(size_t)> &callback)
{
    const auto freeze_action = std::format(L"{} > Freeze ---", base_path);

    const auto reset_list = [=] {
        paths->clear();
        ActionManager::notify_display_name_changed(std::format(L"{} > *", base_path));
    };

    const auto toggle_frozen = [=] {
        *frozen = *frozen == 0 ? 1 : 0;
        ActionManager::notify_active_changed(freeze_action);
    };

    const auto get_frozen = [=] { return *frozen; };

    add_action(std::format(L"{} > Reset", base_path), Hotkey::t_hotkey::make_empty(), reset_list);
    add_action(freeze_action, Hotkey::t_hotkey::make_empty(), toggle_frozen, always_enabled, get_frozen);

    for (size_t i = 0; i < RecentMenu::MAX_RECENT_ITEMS; ++i)
    {
        const auto get_display_name = [=] -> std::wstring {
            if (paths->size() > i)
            {
                return std::filesystem::path(paths->at(i)).filename();
            }
            return L"(nothing)";
        };

        const auto path = std::format(L"{} > Load Recent Item {}", base_path, i + 1);

        Hotkey::t_hotkey hotkey = i == 0 ? load_first_hotkey : Hotkey::t_hotkey::make_empty();

        add_action(path, hotkey, [=] { callback(i); }, {}, {}, get_display_name);
    }
}

void AppActions::init()
{
    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged,
                         [](const auto &) { ActionManager::notify_enabled_changed(std::format(L"{} *", APP)); });
    Messenger::subscribe(Messenger::Message::EmuPausedChanged,
                         [](const auto &) { ActionManager::notify_active_changed(PAUSE); });
    Messenger::subscribe(Messenger::Message::FastForwardNeedsUpdate,
                         [](const auto &) { ActionManager::notify_active_changed(FAST_FORWARD); });
    Messenger::subscribe(Messenger::Message::CapturingChanged, [](const auto &) {
        ActionManager::notify_enabled_changed(std::format(L"{} *", VIDEO_CAPTURE));
    });
    Messenger::subscribe(Messenger::Message::StatusbarVisibilityChanged,
                         [](const auto &) { ActionManager::notify_active_changed(STATUSBAR); });
    Messenger::subscribe(Messenger::Message::MovieLoopChanged,
                         [](const auto &) { ActionManager::notify_active_changed(LOOP_MOVIE_PLAYBACK); });
    Messenger::subscribe(Messenger::Message::ReadonlyChanged,
                         [](const auto &) { ActionManager::notify_active_changed(READONLY); });
    Messenger::subscribe(Messenger::Message::TaskChanged, [](const auto &) {
        ActionManager::notify_enabled_changed(STOP_MOVIE);
        ActionManager::notify_enabled_changed(CONTINUE_MOVIE_RECORDING);
        ActionManager::notify_enabled_changed(CREATE_MOVIE_BACKUP);
        ActionManager::notify_enabled_changed(SEEK_TO);
    });
    Messenger::subscribe(Messenger::Message::SlotChanged,
                         [](const auto &) { ActionManager::notify_active_changed(std::format(L"{} *", SELECT_SLOT)); });
    Messenger::subscribe(Messenger::Message::FullscreenChanged,
                         [](const auto &) { ActionManager::notify_enabled_changed(FULL_SCREEN); });
}

void AppActions::add()
{
    ActionManager::begin_batch_work();

    add_action(LOAD_ROM, Hotkey::t_hotkey('O', true), load_rom);
    add_action(CLOSE_ROM, Hotkey::t_hotkey('W', true), close_rom, enable_when_emu_launched);
    add_action(RESET_ROM, Hotkey::t_hotkey('R', true), reset_rom, enable_when_emu_launched);
    add_action(REFRESH_ROM_LIST, Hotkey::t_hotkey(VK_F5, true), refresh_rombrowser);
    generate_path_recent_menu(RECENT_ROMS, Hotkey::t_hotkey('O', true, true), &g_config.recent_rom_paths,
                              &g_config.is_recent_rom_paths_frozen, load_recent_rom);
    add_action(EXIT, Hotkey::t_hotkey(VK_F4, false, false, true), exit_app);

    add_action(PAUSE, Hotkey::t_hotkey(VK_PAUSE), pause_emu, enable_when_emu_launched);
    add_action(SPEED_DOWN, Hotkey::t_hotkey(VK_OEM_MINUS), speed_down, enable_when_emu_launched);
    add_action(SPEED_UP, Hotkey::t_hotkey(VK_OEM_PLUS), speed_up, enable_when_emu_launched);
    add_action(SPEED_RESET, Hotkey::t_hotkey(VK_OEM_PLUS, true), speed_reset, enable_when_emu_launched);
    add_action_with_up(FAST_FORWARD, Hotkey::t_hotkey(VK_TAB), fastforward_enable, fastforward_disable,
                       enable_when_emu_launched, fastforward_active);
    add_action_with_up(GS_BUTTON, Hotkey::t_hotkey('G'), gs_button_enable, gs_button_disable, enable_when_emu_launched,
                       gs_button_active);
    add_action(FRAME_ADVANCE, Hotkey::t_hotkey(VK_OEM_5), frame_advance, enable_when_emu_launched);
    add_action(MULTI_FRAME_ADVANCE, Hotkey::t_hotkey(VK_OEM_5, true), multi_frame_advance, enable_when_emu_launched);
    add_action(MULTI_FRAME_ADVANCE_DECREMENT, Hotkey::t_hotkey('E', true), multi_frame_advance_increment,
               enable_when_emu_launched);
    add_action(MULTI_FRAME_ADVANCE_INCREMENT, Hotkey::t_hotkey('Q', true), multi_frame_advance_decrement,
               enable_when_emu_launched);
    add_action(MULTI_FRAME_ADVANCE_RESET, Hotkey::t_hotkey('E', true, true), multi_frame_advance_reset,
               enable_when_emu_launched);
    add_action(SAVE_CURRENT_SLOT, Hotkey::t_hotkey('I'), save_slot, enable_when_emu_launched);
    add_action(SAVE_STATE_FILE, Hotkey::t_hotkey::make_empty(), save_state_as, enable_when_emu_launched);
    add_action(LOAD_CURRENT_SLOT, Hotkey::t_hotkey('P'), load_slot, enable_when_emu_launched);
    add_action(LOAD_STATE_FILE, Hotkey::t_hotkey::make_empty(), load_state_as, enable_when_emu_launched);
    for (size_t i = 0; i < 10; ++i)
    {
        const int32_t save_key = i < 9 ? '1' + i : '0';
        const int32_t load_key = VK_F1 + i;

        const auto do_work = [=](const core_st_job job) {
            g_main_ctx.core_ctx->vr_wait_increment();

            g_config.st_slot = i;
            Messenger::broadcast(Messenger::Message::SlotChanged, (size_t)g_config.st_slot);

            ThreadPool::submit_task([=] {
                g_main_ctx.core_ctx->vr_wait_decrement();
                g_main_ctx.core_ctx->st_do_file(get_st_with_slot_path(i), job, nullptr, false);
            });
        };

        const auto save = [=] { do_work(core_st_job_save); };

        const auto load = [=] { do_work(core_st_job_load); };

        size_t visual_slot = i + 1;
        add_action(std::vformat(SAVE_SLOT_X, std::make_wformat_args(visual_slot)),
                   Hotkey::t_hotkey(save_key, false, true), save, enable_when_emu_launched);
        add_action(std::vformat(LOAD_SLOT_X, std::make_wformat_args(visual_slot)), Hotkey::t_hotkey(load_key), load,
                   enable_when_emu_launched);
    }
    for (size_t i = 0; i < 10; ++i)
    {
        const int32_t key = i < 9 ? '1' + i : '0';

        const auto get_active = [=] { return g_config.st_slot == i; };

        const auto set_slot = [=] { set_save_slot(i); };

        size_t visual_slot = i + 1;
        add_action(std::vformat(SELECT_SLOT_X, std::make_wformat_args(visual_slot)), Hotkey::t_hotkey(key), set_slot,
                   enable_when_emu_launched, get_active);
    }
    add_action(UNDO_LOAD_STATE, Hotkey::t_hotkey('Z', true), undo_load_state, enable_when_emu_launched);

    add_action(FULL_SCREEN, Hotkey::t_hotkey(VK_RETURN, false, false, true), toggle_fullscreen,
               enable_when_emu_launched, fastforward_active);
    add_action(VIDEO_SETTINGS, Hotkey::t_hotkey::make_empty(), show_video_plugin_settings);
    add_action(AUDIO_SETTINGS, Hotkey::t_hotkey::make_empty(), show_audio_plugin_settings);
    add_action(INPUT_SETTINGS, Hotkey::t_hotkey::make_empty(), show_input_plugin_settings);
    add_action(RSP_SETTINGS, Hotkey::t_hotkey::make_empty(), show_rsp_plugin_settings);
    add_action(STATUSBAR, Hotkey::t_hotkey('S', false, false, true), toggle_statusbar, disable_when_emu_launched,
               [] { return g_config.is_statusbar_enabled; });
    add_action(SETTINGS, Hotkey::t_hotkey('S', true), show_settings_dialog);

    add_action(START_MOVIE_RECORDING, Hotkey::t_hotkey('R', true, true), start_movie_recording,
               enable_when_emu_launched);
    add_action(START_MOVIE_PLAYBACK, Hotkey::t_hotkey('P', true, true), start_movie_playback);
    add_action(CONTINUE_MOVIE_RECORDING, Hotkey::t_hotkey::make_empty(), continue_movie_recording,
               enable_during_playback);
    add_action(STOP_MOVIE, Hotkey::t_hotkey('C', true, true), stop_movie, enable_when_emu_launched_and_vcr_active);
    add_action(CREATE_MOVIE_BACKUP, Hotkey::t_hotkey('B', true, true), create_movie_backup,
               enable_when_emu_launched_and_vcr_active);
    generate_path_recent_menu(RECENT_MOVIES, Hotkey::t_hotkey('T', true, true), &g_config.recent_movie_paths,
                              &g_config.is_recent_movie_paths_frozen, load_recent_movie);
    add_action(LOOP_MOVIE_PLAYBACK, Hotkey::t_hotkey('L', false, true), toggle_movie_loop, always_enabled,
               [] { return g_config.core.is_movie_loop_enabled; });
    add_action(READONLY, Hotkey::t_hotkey('R', false, true), toggle_readonly, always_enabled,
               [] { return g_config.core.vcr_readonly; });
    add_action(WAIT_AT_MOVIE_END, Hotkey::t_hotkey::make_empty(), toggle_wait_at_movie_end, always_enabled,
               [] { return g_config.core.wait_at_movie_end; });

    add_action(COMMAND_PALETTE, Hotkey::t_hotkey('P', true), show_command_palette);
    add_action(PIANO_ROLL, Hotkey::t_hotkey::make_empty(), show_piano_roll, enable_when_emu_launched);
    add_action(CHEATS, Hotkey::t_hotkey::make_empty(), show_cheat_dialog, enable_when_emu_launched);
    add_action(SEEK_TO, Hotkey::t_hotkey('G', true), show_seek_dialog, enable_when_emu_launched_and_vcr_active);
    add_action(USAGE_STATISTICS, Hotkey::t_hotkey::make_empty(), show_statistics);
    add_action(CORE_INFORMATION, Hotkey::t_hotkey::make_empty(), show_ram_start);
    add_action(DEBUGGER, Hotkey::t_hotkey::make_empty(), show_debugger, enable_when_emu_launched);
    add_action(START_TRACE_LOGGER, Hotkey::t_hotkey::make_empty(), start_tracelog,
               enable_when_emu_launched_and_core_is_pure_interpreter);
    add_action(STOP_TRACE_LOGGER, Hotkey::t_hotkey::make_empty(), stop_tracelog, enable_when_tracelog_active);
    add_action(VIDEO_CAPTURE_START, Hotkey::t_hotkey::make_empty(), start_capture_normal, enable_when_emu_launched);
    add_action(VIDEO_CAPTURE_START_PRESET, Hotkey::t_hotkey::make_empty(), start_capture_from_preset,
               enable_when_emu_launched);
    add_action(VIDEO_CAPTURE_STOP, Hotkey::t_hotkey::make_empty(), stop_capture,
               enable_when_emu_launched_and_capturing);
    add_action(SCREENSHOT, Hotkey::t_hotkey(VK_F12), screenshot, enable_when_emu_launched);

    add_action(CHECK_FOR_UPDATES, Hotkey::t_hotkey::make_empty(), check_for_updates_manual);
    add_action(ABOUT, Hotkey::t_hotkey::make_empty(), show_about_dialog);

    add_action(SHOW_INSTANCES, Hotkey::t_hotkey('N', true), show_lua_dialog);
    generate_path_recent_menu(RECENT_SCRIPTS, Hotkey::t_hotkey('K', true, true), &g_config.recent_lua_script_paths,
                              &g_config.is_recent_scripts_frozen, load_recent_script);
    add_action(CLOSE_ALL, Hotkey::t_hotkey('W', true, true), close_all_lua_scripts);

    ActionManager::end_batch_work();

    check_for_updates(false);
}
