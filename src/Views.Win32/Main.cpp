/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <Config.h>
#include <DialogService.h>
#include <Messenger.h>
#include <Plugin.h>
#include <ThreadPool.h>
#include <strsafe.h>
#include <capture/CaptureManager.h>
#include <components/CoreUtils.h>
#include <components/ActionMenu.h>
#include <components/AppActions.h>
#include <components/CLI.h>
#include <components/CommandPalette.h>
#include <components/ParameterPalette.h>
#include <components/ConfigDialog.h>
#include <components/CoreDbg.h>
#include <components/CrashManager.h>
#include <components/Dispatcher.h>
#include <components/HotkeyTracker.h>
#include <components/LuaDialog.h>
#include <components/MGECompositor.h>
#include <components/PianoRoll.h>
#include <components/RecentItems.h>
#include <components/RomBrowser.h>
#include <components/Seeker.h>
#include <components/Statusbar.h>
#include <lua/LuaCallbacks.h>
#include <lua/LuaManager.h>
#include <lua/LuaRenderer.h>
#include <spdlog/sinks/basic_file_sink.h>

// Throwaway actions which can be spammed get keys as to not clog up the async executor queue
#define ASYNC_KEY_CLOSE_ROM (1)
#define ASYNC_KEY_START_ROM (2)
#define ASYNC_KEY_RESET_ROM (3)
#define ASYNC_KEY_PLAY_MOVIE (4)

t_main_context g_main_ctx{};

bool g_frame_changed = true;

MMRESULT g_ui_timer;
bool g_paused_before_focus;
bool g_vis_since_input_poll_warning_dismissed;
bool g_emu_starting;
DWORD g_ui_thread_id{};

ULONG_PTR gdi_plus_token;

constexpr auto WND_CLASS = L"myWindowClass";

BetterEmulationLock::BetterEmulationLock()
{
    if (g_main_ctx.in_menu_loop)
    {
        was_paused = g_main_ctx.paused_before_menu;

        // This fires before WM_EXITMENULOOP (which restores the paused_before_menu state), so we need to trick it...
        g_main_ctx.paused_before_menu = true;
    }
    else
    {
        was_paused = g_main_ctx.core_ctx->vr_get_paused();
        g_main_ctx.core_ctx->vr_pause_emu();
    }
}

BetterEmulationLock::~BetterEmulationLock()
{
    if (was_paused)
    {
        g_main_ctx.core_ctx->vr_pause_emu();
    }
    else
    {
        g_main_ctx.core_ctx->vr_resume_emu();
    }
}

std::wstring get_mupen_name(bool simple)
{
#ifdef _DEBUG
#define BUILD_TARGET_INFO L"-debug"
#else
#define BUILD_TARGET_INFO L""
#endif

#ifdef UNICODE
#define CHARSET_INFO L""
#else
#define CHARSET_INFO L"-a"
#endif

#ifdef _M_X64
#define ARCH_INFO L"-x64"
#else
#define ARCH_INFO L""
#endif

#define BASE_NAME L"Mupen 64 "

    std::wstring version_suffix = VERSION_SUFFIX;
    if (version_suffix.empty())
    {
        // version_suffix = L"-rc5";
    }

    if (simple)
    {
        return BASE_NAME CURRENT_VERSION + version_suffix;
    }

    return BASE_NAME CURRENT_VERSION + version_suffix + ARCH_INFO CHARSET_INFO BUILD_TARGET_INFO;
}

const wchar_t *get_input_text()
{
    static wchar_t text[1024]{};
    memset(text, 0, sizeof(text));

    core_buttons b = LuaCallbacks::get_last_controller_data(0);
    wsprintf(text, L"(%d, %d) ", b.x, b.y);
    if (b.start) lstrcatW(text, L"S");
    if (b.z) lstrcatW(text, L"Z");
    if (b.a) lstrcatW(text, L"A");
    if (b.b) lstrcatW(text, L"B");
    if (b.l) lstrcatW(text, L"L");
    if (b.r) lstrcatW(text, L"R");
    if (b.cu || b.cd || b.cl || b.cr)
    {
        lstrcatW(text, L" C");
        if (b.cu) lstrcatW(text, L"^");
        if (b.cd) lstrcatW(text, L"v");
        if (b.cl) lstrcatW(text, L"<");
        if (b.cr) lstrcatW(text, L">");
    }
    if (b.du || b.dd || b.dl || b.dr)
    {
        lstrcatW(text, L"D");
        if (b.du) lstrcatW(text, L"^");
        if (b.dd) lstrcatW(text, L"v");
        if (b.dl) lstrcatW(text, L"<");
        if (b.dr) lstrcatW(text, L">");
    }
    return text;
}

const wchar_t *get_status_text()
{
    static wchar_t text[1024]{};
    memset(text, 0, sizeof(text));

    const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();

    const auto index_adjustment = g_config.vcr_0_index ? 1 : 0;
    const auto current_sample = info.current_sample;
    const auto current_vi = g_main_ctx.core_ctx->vcr_get_current_vi();
    const auto is_before_start = static_cast<int64_t>(current_sample) - static_cast<int64_t>(index_adjustment) < 0;

    if (g_main_ctx.core_ctx->vcr_get_warp_modify_status())
    {
        StringCbPrintfW(text, sizeof(text), L"Warping (%.2f%%)",
                        (double)current_sample / (double)g_main_ctx.core_ctx->vcr_get_length_samples() * 100.0);
        return text;
    }

    if (g_main_ctx.core_ctx->vcr_get_task() == task_recording)
    {
        if (is_before_start)
        {
            memset(text, 0, sizeof(text));
        }
        else
        {
            wsprintfW(text, L"%d (%d) ", current_vi, current_sample - index_adjustment);
        }
    }

    if (g_main_ctx.core_ctx->vcr_get_task() == task_playback)
    {
        if (is_before_start)
        {
            memset(text, 0, sizeof(text));
        }
        else
        {
            wsprintfW(text, L"%d / %d (%d / %d) ", current_vi, g_main_ctx.core_ctx->vcr_get_length_vis(),
                      current_sample - index_adjustment, g_main_ctx.core_ctx->vcr_get_length_samples());
        }
    }

    return text;
}

std::filesystem::path get_summercart_path()
{
    return Config::save_directory() / "card.vhd";
}

std::filesystem::path get_st_with_slot_path(const size_t slot)
{
    const auto hdr = g_main_ctx.core_ctx->vr_get_rom_header();
    const auto fname =
        std::format(L"{} {}.st{}", IOUtils::to_wide_string((const char *)hdr->nom),
                    IOUtils::to_wide_string(g_main_ctx.core_ctx->vr_country_code_to_country_name(hdr->Country_code)),
                    std::to_wstring(slot));
    return Config::save_directory() / fname;
}

void st_callback_wrapper(const core_st_callback_info &info, const std::vector<uint8_t> &)
{
    if (info.medium == core_st_medium_memory)
    {
        return;
    }

    if (info.medium == core_st_medium_path)
    {
        const auto &fname = info.params.path.filename().wstring();
        const bool is_slot = fname.find(L".st") != std::wstring::npos && std::isdigit(fname.back());

        if (is_slot)
        {
            const size_t slot = std::stoul(fname.substr(fname.size() - 1));

            switch (info.result)
            {
            case Res_Ok:
                Statusbar::post(
                    std::format(L"{} slot {}", info.job == core_st_job_save ? L"Saved" : L"Loaded", slot + 1));
                break;
            case Res_Cancelled:
                Statusbar::post(std::format(L"Cancelled {}", info.job == core_st_job_save ? L"save" : L"load"));
                break;
            default:
                Statusbar::post(
                    std::format(L"Failed to {} slot {}", info.job == core_st_job_save ? L"save" : L"load", slot + 1));
                break;
            }
            return;
        }

        switch (info.result)
        {
        case Res_Ok:
            Statusbar::post(std::format(L"{} {}", info.job == core_st_job_save ? L"Saved" : L"Loaded",
                                        info.params.path.filename().wstring()));
            break;
        case Res_Cancelled:
            Statusbar::post(std::format(L"Cancelled {}", info.job == core_st_job_save ? L"save" : L"load"));
            break;
        default: {
            const auto message =
                std::format(L"Failed to {} {} (error code {}).\nVerify that the savestate is valid and accessible.",
                            info.job == core_st_job_save ? L"save" : L"load", info.params.path.filename().wstring(),
                            (int32_t)info.result);
            DialogService::show_dialog(message.c_str(), L"Savestate", fsvc_error);
            break;
        }
        }
    }
}

void update_screen()
{
    if (PluginUtil::mge_available())
    {
        MGECompositor::update_screen();
    }
    else
    {
        g_plugin_funcs.video_update_screen();
    }
}

void ai_len_changed()
{
    if (!CaptureManager::is_capturing())
    {
        return;
    }

    CaptureManager::ai_len_changed();
}

static std::wstring get_titlebar_text()
{
    auto text = get_mupen_name();

    if (g_emu_starting) text += L" - Starting...";

    if (g_main_ctx.core_ctx->vr_get_launched())
        text += std::format(
            L" - {}", IOUtils::to_wide_string(reinterpret_cast<char *>(g_main_ctx.core_ctx->vr_get_rom_header()->nom)));

    if (g_main_ctx.core_ctx->vcr_get_task() != task_idle)
    {
        auto vcr_filename = g_main_ctx.core_ctx->vcr_get_path().filename();
        text += std::format(L" - {}", vcr_filename.c_str());
    }

    if (CaptureManager::is_capturing())
        text += std::format(L" - {}", CaptureManager::get_current_path().filename().wstring());

    return text;
}

static void update_titlebar()
{
    ThreadPool::submit_task([] {
        const auto text = get_titlebar_text();
        g_main_ctx.dispatcher->invoke([&] { SetWindowText(g_main_ctx.hwnd, text.c_str()); });
    });
}

#pragma region Change notifications

void on_script_started(std::any data)
{
    g_main_ctx.dispatcher->invoke([=] {
        auto value = std::any_cast<std::filesystem::path>(data);
        RecentMenu::add(AppActions::RECENT_SCRIPTS, g_config.recent_lua_script_paths, value.wstring(),
                        g_config.is_recent_scripts_frozen);
    });
}

void on_task_changed(std::any data)
{
    g_main_ctx.dispatcher->invoke([=] {
        auto value = std::any_cast<core_vcr_task>(data);
        static auto previous_value = value;
        if (!vcr_is_task_recording(value) && vcr_is_task_recording(previous_value))
        {
            Statusbar::post(L"Recording stopped");
        }
        if (!task_is_playback(value) && task_is_playback(previous_value))
        {
            Statusbar::post(L"Playback stopped");
        }

        if ((vcr_is_task_recording(value) && !vcr_is_task_recording(previous_value)) ||
            task_is_playback(value) && !task_is_playback(previous_value) &&
                !g_main_ctx.core_ctx->vcr_get_path().empty())
        {
            RecentMenu::add(AppActions::RECENT_MOVIES, g_config.recent_movie_paths,
                            g_main_ctx.core_ctx->vcr_get_path().wstring(), g_config.is_recent_movie_paths_frozen);
        }

        update_titlebar();
        previous_value = value;
    });
}

void on_emu_stopping(std::any)
{
    g_main_ctx.dispatcher->invoke([] {
        LuaDialog::store_running_scripts();
        LuaDialog::stop_all();
    });
}

void on_emu_launched_changed(std::any data)
{
    g_main_ctx.dispatcher->invoke([=] {
        auto value = std::any_cast<bool>(data);
        static auto previous_value = value;

        const auto window_style = GetWindowLong(g_main_ctx.hwnd, GWL_STYLE);
        if (value)
        {
            SetWindowLong(g_main_ctx.hwnd, GWL_STYLE, window_style & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
        }
        else
        {
            SetWindowLong(g_main_ctx.hwnd, GWL_STYLE, window_style | WS_THICKFRAME | WS_MAXIMIZEBOX);
        }

        update_titlebar();
        // Some menu items, like movie ones, depend on both this and vcr task
        on_task_changed(g_main_ctx.core_ctx->vcr_get_task());

        // Reset and restore view stuff when emulation starts
        if (value)
        {
            g_vis_since_input_poll_warning_dismissed = false;

            const auto rom_path = g_main_ctx.core_ctx->vr_get_rom_path();
            if (!rom_path.empty())
            {
                RecentMenu::add(AppActions::RECENT_ROMS, g_config.recent_rom_paths, rom_path.wstring(),
                                g_config.is_recent_rom_paths_frozen);
            }

            LuaDialog::load_running_scripts();
        }

        if (!value && previous_value)
        {
            g_view_logger->info("[View] Restoring window size to {}x{}...", g_config.window_width,
                                g_config.window_height);
            SetWindowPos(g_main_ctx.hwnd, nullptr, 0, 0, g_config.window_width, g_config.window_height, SWP_NOMOVE);
        }

        RedrawWindow(g_main_ctx.hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);

        previous_value = value;
    });
}

void on_capturing_changed(std::any data)
{
    g_main_ctx.dispatcher->invoke([=] {
        auto value = std::any_cast<bool>(data);

        if (value)
        {
            SetWindowLong(g_main_ctx.hwnd, GWL_STYLE, GetWindowLong(g_main_ctx.hwnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
            // NOTE: WS_EX_LAYERED fixes BitBlt'ing from the window when its off-screen, as it wouldnt redraw otherwise
            // (relevant for Window capture mode)
            SetWindowLong(g_main_ctx.hwnd, GWL_EXSTYLE, GetWindowLong(g_main_ctx.hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        }
        else
        {
            SetWindowPos(g_main_ctx.hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetWindowLong(g_main_ctx.hwnd, GWL_STYLE, GetWindowLong(g_main_ctx.hwnd, GWL_STYLE) | WS_MINIMIZEBOX);
            SetWindowLong(g_main_ctx.hwnd, GWL_EXSTYLE, GetWindowLong(g_main_ctx.hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
        }

        update_titlebar();
    });
}

void on_speed_modifier_changed(std::any data)
{
    auto value = std::any_cast<int32_t>(data);

    const auto vis_per_second =
        g_main_ctx.core_ctx->vr_get_vis_per_second(g_main_ctx.core_ctx->vr_get_rom_header()->Country_code);
    const auto effective_vis_per_second = (double)vis_per_second * ((double)value / 100.0);

    Statusbar::post(std::format(L"Speed limit: {}% ({:.0f} VI/s)", value, effective_vis_per_second));
}

void on_emu_paused_changed(std::any data)
{
    g_frame_changed = true;
}

void on_vis_since_input_poll_exceeded(std::any)
{
    if (g_vis_since_input_poll_warning_dismissed)
    {
        return;
    }

    if (g_config.silent_mode ||
        DialogService::show_ask_dialog(VIEW_DLG_LAG_EXCEEDED,
                                       L"An unusual execution pattern was detected. Continuing might leave the "
                                       L"emulator in an unusable state.\r\nWould you like to terminate emulation?",
                                       L"Warning", true))
    {
        ThreadPool::submit_task([] {
            const auto result = g_main_ctx.core_ctx->vr_close_rom(true);
            CoreUtils::show_error_dialog_for_result(result);
        });
    }
    g_vis_since_input_poll_warning_dismissed = true;
}

void on_movie_loop_changed(std::any data)
{
    auto value = std::any_cast<bool>(data);
    Statusbar::post(value ? L"Movies restart after ending" : L"Movies stop after ending");
}

void on_fullscreen_changed(std::any data)
{
    g_main_ctx.dispatcher->invoke([=] {
        auto value = std::any_cast<bool>(data);
        ShowCursor(!value);
    });
}

void on_config_loaded(std::any)
{
    RomBrowser::build();
}

void on_seek_completed(std::any)
{
    LuaCallbacks::call_seek_completed();
}

void on_warp_modify_status_changed(std::any data)
{
    auto value = std::any_cast<bool>(data);
    LuaCallbacks::call_warp_modify_status_changed(value);
}

void on_emu_starting_changed(std::any data)
{
    g_emu_starting = std::any_cast<bool>(data);
    update_titlebar();
}

t_window_info get_window_info()
{
    t_window_info info;

    RECT client_rect = {};
    GetClientRect(g_main_ctx.hwnd, &client_rect);

    // full client dimensions including statusbar
    info.width = client_rect.right - client_rect.left;
    info.height = client_rect.bottom - client_rect.top;

    RECT statusbar_rect = {0};
    if (Statusbar::hwnd()) GetClientRect(Statusbar::hwnd(), &statusbar_rect);
    info.statusbar_height = statusbar_rect.bottom - statusbar_rect.top;

    // subtract size of toolbar and statusbar from buffer dimensions
    // if video plugin knows about this, whole game screen should be captured. Most of the plugins do.
    info.height -= info.statusbar_height;
    return info;
}

#pragma endregion

bool is_on_gui_thread()
{
    return GetCurrentThreadId() == g_ui_thread_id;
}

std::filesystem::path get_app_full_path()
{
    std::wstring app_path(MAX_PATH, 0);
    const DWORD app_path_len = GetModuleFileName(nullptr, app_path.data(), app_path.size());

    if (app_path_len == 0)
    {
        return L"";
    }

    app_path.resize(app_path_len);

    // equals drive + dir
    return std::filesystem::path(app_path).parent_path();
}

void open_console()
{
    AllocConsole();
    FILE *f = 0;
    freopen_s(&f, "CONIN$", "r", stdin);
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    SetConsoleOutputCP(CP_UTF8);
}

static t_lua_key_event_args get_base_key_event_args()
{
    t_lua_key_event_args args;
    args.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    args.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    args.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    args.meta = (GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0;
    return args;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_DROPFILES: {
        auto drop = (HDROP)wParam;
        wchar_t fname[MAX_PATH] = {0};
        DragQueryFile(drop, 0, fname, std::size(fname));

        std::filesystem::path path = fname;
        std::string extension = MiscHelpers::to_lower(path.extension().string());

        if (extension == ".n64" || extension == ".z64" || extension == ".v64" || extension == ".rom")
        {
            AppActions::load_rom_from_path(path);
        }
        else if (extension == ".m64")
        {
            g_config.core.vcr_readonly = true;
            Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)g_config.core.vcr_readonly);
            ThreadPool::submit_task([fname] {
                auto result = g_main_ctx.core_ctx->vcr_start_playback(fname);
                CoreUtils::show_error_dialog_for_result(result);
            });
        }
        else if (extension == ".st" || extension == ".savestate" || extension == ".st0" || extension == ".st1" ||
                 extension == ".st2" || extension == ".st3" || extension == ".st4" || extension == ".st5" ||
                 extension == ".st6" || extension == ".st7" || extension == ".st8" || extension == ".st9")
        {
            if (!g_main_ctx.core_ctx->vr_get_launched()) break;
            g_main_ctx.core_ctx->vr_wait_increment();
            ThreadPool::submit_task([=] {
                g_main_ctx.core_ctx->vr_wait_decrement();
                g_main_ctx.core_ctx->st_do_file(fname, core_st_job_load, nullptr, false);
            });
        }
        else if (extension == ".lua")
        {
            LuaDialog::start_and_add_if_needed(path);
        }
        break;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        const bool repeat = (HIWORD(lParam) & KF_REPEAT) == KF_REPEAT;

        t_lua_key_event_args args = get_base_key_event_args();
        args.keycode = wParam;
        args.pressed = true;
        args.repeat = repeat;

        LuaCallbacks::call_atkey(args);

        if (g_plugin_funcs.input_key_down && g_main_ctx.core_ctx->vr_get_launched())
            g_plugin_funcs.input_key_down(wParam, lParam);
        break;
    }
    case WM_SYSKEYUP:
    case WM_KEYUP: {
        t_lua_key_event_args args = get_base_key_event_args();
        args.keycode = wParam;
        args.pressed = false;
        args.repeat = false;

        LuaCallbacks::call_atkey(args);

        if (g_plugin_funcs.input_key_up && g_main_ctx.core_ctx->vr_get_launched())
            g_plugin_funcs.input_key_up(wParam, lParam);
        break;
    }
    case WM_CHAR: {
        t_lua_key_event_args args = get_base_key_event_args();
        const bool repeat = (HIWORD(lParam) & KF_REPEAT) == KF_REPEAT;
        const auto chr = static_cast<wchar_t>(wParam);

        if (std::iswcntrl(chr)) break;

        args.text = std::wstring(1, chr);
        args.repeat = repeat;
        LuaCallbacks::call_atkey(args);
        break;
    }
    case WM_MOUSEWHEEL:
        g_main_ctx.last_wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam);

        // https://github.com/mupen64/mupen64-rr-lua/issues/190
        LuaCallbacks::call_window_message(hwnd, Message, wParam, lParam);
        break;
    case WM_NOTIFY: {
        if (wParam == IDC_ROMLIST)
        {
            RomBrowser::notify(lParam);
        }
        return 0;
    }
    case WM_MOVE: {
        if (g_main_ctx.core_ctx->vr_get_launched())
        {
            g_plugin_funcs.video_move_screen((int)wParam, lParam);
        }

        if (IsIconic(g_main_ctx.hwnd))
        {
            // GetWindowRect values are nonsense when minimized
            break;
        }

        RECT rect = {0};
        GetWindowRect(g_main_ctx.hwnd, &rect);
        g_config.window_x = rect.left;
        g_config.window_y = rect.top;

        Messenger::broadcast(Messenger::Message::MainWindowMoved, nullptr);

        break;
    }
    case WM_SIZE: {
        SendMessage(Statusbar::hwnd(), WM_SIZE, 0, 0);
        RECT rect{};
        GetClientRect(g_main_ctx.hwnd, &rect);
        Messenger::broadcast(Messenger::Message::SizeChanged, rect);

        if (g_main_ctx.core_ctx->vr_get_launched())
        {
            // We don't need to remember the dimensions set by gfx plugin
            break;
        }

        if (IsIconic(g_main_ctx.hwnd))
        {
            // GetWindowRect values are nonsense when minimized
            break;
        }

        // Window creation expects the size with nc area, so it's easiest to just use the window rect here
        GetWindowRect(hwnd, &rect);
        g_config.window_width = rect.right - rect.left;
        g_config.window_height = rect.bottom - rect.top;

        break;
    }
    case WM_FOCUS_MAIN_WINDOW:
        SetFocus(g_main_ctx.hwnd);
        break;
    case WM_EXECUTE_DISPATCHER:
        g_main_ctx.dispatcher->execute();
        break;
    case WM_NCCREATE:
        g_main_ctx.hwnd = hwnd;
        break;
    case WM_CREATE:
        SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES);

        ActionMenu::init();

        ActionMenu::add_managed_menu(hwnd);
        AppActions::add();
        HotkeyTracker::attach(hwnd);

        MGECompositor::create(hwnd);
        PianoRoll::init();
        LuaDialog::init();

        return TRUE;
    case WM_DESTROY:
        timeKillEvent(g_ui_timer);
        Config::save();
        LuaRenderer::stop();
        Gdiplus::GdiplusShutdown(gdi_plus_token);
        CoUninitialize();
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        if (confirm_user_exit())
        {
            g_main_ctx.exiting = true;

            LuaDialog::close_all();

            std::thread([] {
                g_main_ctx.core_ctx->vr_close_rom(true);
                g_main_ctx.dispatcher->invoke([] { DestroyWindow(g_main_ctx.hwnd); });
            }).detach();
            break;
        }
        return 0;
    case WM_WINDOWPOSCHANGING: // allow gfx plugin to set arbitrary size
        return 0;
    case WM_GETMINMAXINFO: {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 100;
        lpMMI->ptMinTrackSize.y = 100;
        // this might break small res with gfx plugin!!!
    }
    break;
    case WM_ENTERMENULOOP:
        g_main_ctx.in_menu_loop = true;
        g_main_ctx.paused_before_menu = g_main_ctx.core_ctx->vr_get_paused();
        g_main_ctx.core_ctx->vr_pause_emu();
        break;
    case WM_EXITMENULOOP:
        // This message is sent when we escape the blocking menu loop, including situations where the clicked menu
        // spawns a dialog. In those situations, we would unpause the game here (since this message is sent first), and
        // then pause it again in the menu item message handler. It's almost guaranteed that a game frame will pass
        // between those messages, so we need to wait a bit on another thread before unpausing.
        std::thread([] {
            Sleep(60);
            g_main_ctx.in_menu_loop = false;
            if (g_main_ctx.paused_before_menu)
            {
                g_main_ctx.core_ctx->vr_pause_emu();
            }
            else
            {
                g_main_ctx.core_ctx->vr_resume_emu();
            }
        }).detach();
        break;
    case WM_ACTIVATE:
        UpdateWindow(hwnd);

        if (!g_config.is_unfocused_pause_enabled)
        {
            break;
        }

        switch (LOWORD(wParam))
        {
        case WA_ACTIVE:
        case WA_CLICKACTIVE:
            if (!g_paused_before_focus)
            {
                g_main_ctx.core_ctx->vr_resume_emu();
            }
            break;
        case WA_INACTIVE:
            g_paused_before_focus = g_main_ctx.core_ctx->vr_get_paused();
            g_main_ctx.core_ctx->vr_pause_emu();
            break;
        default:
            break;
        }
        break;
    default:
        return DefWindowProc(hwnd, Message, wParam, lParam);
    }

    return TRUE;
}

static void CALLBACK invalidate_callback(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
    g_main_ctx.core_ctx->vr_invalidate_visuals();

    static std::chrono::high_resolution_clock::time_point last_statusbar_update =
        std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point time = std::chrono::high_resolution_clock::now();

    if (g_frame_changed)
    {
        Statusbar::post(get_input_text(), Statusbar::Section::Input);

        if (CaptureManager::is_capturing())
        {
            if (g_main_ctx.core_ctx->vcr_get_task() == task_idle)
            {
                Statusbar::post(std::format(L"{}", CaptureManager::get_video_frame()), Statusbar::Section::VCR);
            }
            else
            {
                Statusbar::post(std::format(L"{}({})", get_status_text(), CaptureManager::get_video_frame()),
                                Statusbar::Section::VCR);
            }
        }
        else
        {
            Statusbar::post(get_status_text(), Statusbar::Section::VCR);
        }

        g_frame_changed = false;
    }

    // We throttle FPS and VI/s visual updates to 1 per second, so no unstable values are displayed
    if (time - last_statusbar_update > std::chrono::seconds(1))
    {
        float fps, vis;
        g_main_ctx.core_ctx->vr_get_timings(fps, vis);

        Statusbar::post(std::format(L"FPS: {:.1f}", fps), Statusbar::Section::FPS);
        Statusbar::post(std::format(L"VI/s: {:.1f}", vis), Statusbar::Section::VIs);

        last_statusbar_update = time;
    }
}

static core_result init_core()
{
    g_main_ctx.core.cfg = &g_config.core;
    // g_main_ctx.core.io_service = &g_main_ctx.io_service;
    g_main_ctx.core.callbacks = {};
    g_main_ctx.core.callbacks.vi = [](const bool new_present) {
        LuaCallbacks::call_interval();
        LuaCallbacks::call_vi();
        if (CaptureManager::is_capturing()) CaptureManager::append_video(!new_present);
    };
    g_main_ctx.core.callbacks.input = LuaCallbacks::call_input;
    g_main_ctx.core.callbacks.frame = [] { g_frame_changed = true; };
    g_main_ctx.core.callbacks.interval = LuaCallbacks::call_interval;
    g_main_ctx.core.callbacks.ai_len_changed = ai_len_changed;
    g_main_ctx.core.callbacks.play_movie = LuaCallbacks::call_play_movie;
    g_main_ctx.core.callbacks.stop_movie = [] {
        LuaCallbacks::call_stop_movie();
        if (g_config.stop_capture_at_movie_end && CaptureManager::is_capturing()) CaptureManager::stop_capture();
    };
    g_main_ctx.core.callbacks.loop_movie = [] {
        if (g_config.stop_capture_at_movie_end && CaptureManager::is_capturing()) CaptureManager::stop_capture();
    };
    g_main_ctx.core.callbacks.save_state = LuaCallbacks::call_save_state;
    g_main_ctx.core.callbacks.load_state = LuaCallbacks::call_load_state;
    g_main_ctx.core.callbacks.reset = LuaCallbacks::call_reset;
    g_main_ctx.core.callbacks.seek_completed = [] {
        Messenger::broadcast(Messenger::Message::SeekCompleted, nullptr);
        LuaCallbacks::call_seek_completed();
    };
    g_main_ctx.core.callbacks.core_executing_changed = [](bool value) {
        Messenger::broadcast(Messenger::Message::CoreExecutingChanged, value);
    };
    g_main_ctx.core.callbacks.emu_paused_changed = [](bool value) {
        Messenger::broadcast(Messenger::Message::EmuPausedChanged, value);
    };
    g_main_ctx.core.callbacks.emu_launched_changed = [](bool value) {
        Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, value);
    };
    g_main_ctx.core.callbacks.emu_starting_changed = [](bool value) {
        Messenger::broadcast(Messenger::Message::EmuStartingChanged, value);
    };
    g_main_ctx.core.callbacks.emu_starting = [] { PluginUtil::start_plugins(); };
    g_main_ctx.core.callbacks.emu_stopped = [] { PluginUtil::stop_plugins(); };
    g_main_ctx.core.callbacks.emu_stopping = []() { Messenger::broadcast(Messenger::Message::EmuStopping, nullptr); };
    g_main_ctx.core.callbacks.reset_completed = []() {
        Messenger::broadcast(Messenger::Message::ResetCompleted, nullptr);
    };
    g_main_ctx.core.callbacks.speed_modifier_changed = [](int32_t value) {
        Messenger::broadcast(Messenger::Message::SpeedModifierChanged, value);
    };
    g_main_ctx.core.callbacks.warp_modify_status_changed = [](bool value) {
        Messenger::broadcast(Messenger::Message::WarpModifyStatusChanged, value);
    };
    g_main_ctx.core.callbacks.current_sample_changed = [](int32_t value) {
        Messenger::broadcast(Messenger::Message::CurrentSampleChanged, value);
    };
    g_main_ctx.core.callbacks.task_changed = [](core_vcr_task value) {
        Messenger::broadcast(Messenger::Message::TaskChanged, value);
    };
    g_main_ctx.core.callbacks.rerecords_changed = [](uint64_t value) {
        Messenger::broadcast(Messenger::Message::RerecordsChanged, value);
    };
    g_main_ctx.core.callbacks.unfreeze_completed = []() {
        Messenger::broadcast(Messenger::Message::UnfreezeCompleted, nullptr);
    };
    g_main_ctx.core.callbacks.seek_savestate_changed = [](size_t value) {
        Messenger::broadcast(Messenger::Message::SeekSavestateChanged, value);
    };
    g_main_ctx.core.callbacks.readonly_changed = [](bool value) {
        Messenger::broadcast(Messenger::Message::ReadonlyChanged, value);
    };
    g_main_ctx.core.callbacks.dacrate_changed = [](core_system_type value) {
        Messenger::broadcast(Messenger::Message::DacrateChanged, value);
    };
    g_main_ctx.core.callbacks.debugger_resumed_changed = [](bool value) {
        Messenger::broadcast(Messenger::Message::DebuggerResumedChanged, value);
    };
    g_main_ctx.core.callbacks.debugger_cpu_state_changed = [](core_dbg_cpu_state *value) {
        Messenger::broadcast(Messenger::Message::DebuggerCpuStateChanged, value);
    };
    g_main_ctx.core.callbacks.lag_limit_exceeded = []() {
        Messenger::broadcast(Messenger::Message::LagLimitExceeded, nullptr);
    };
    g_main_ctx.core.callbacks.seek_status_changed = []() {
        Messenger::broadcast(Messenger::Message::SeekStatusChanged, nullptr);
    };
    g_main_ctx.core.log_trace = [](std::string_view str) { g_core_logger->trace(str); };
    g_main_ctx.core.log_info = [](std::string_view str) { g_core_logger->info(str); };
    g_main_ctx.core.log_warn = [](std::string_view str) { g_core_logger->warn(str); };
    g_main_ctx.core.log_error = [](std::string_view str) { g_core_logger->error(str); };
    g_main_ctx.core.load_plugins = PluginUtil::load_plugins;
    g_main_ctx.core.initiate_plugins = PluginUtil::initiate_plugins;
    g_main_ctx.core.submit_task = [](const auto cb) { ThreadPool::submit_task(cb); };
    g_main_ctx.core.get_saves_directory = Config::save_directory;
    g_main_ctx.core.get_backups_directory = Config::backup_directory;
    g_main_ctx.core.get_summercart_path = get_summercart_path;
    g_main_ctx.core.show_multiple_choice_dialog = [](std::string_view id, const std::vector<std::string> &choices,
                                                     const char *str, const char *title, core_dialog_type type) {
        auto choices_wide = choices |
                            std::views::transform([](std::string value) { return IOUtils::to_wide_string(value); }) |
                            std::ranges::to<std::vector>();
        auto str_wide = IOUtils::to_wide_string(str);
        auto title_wide = IOUtils::to_wide_string(title);

        return DialogService::show_multiple_choice_dialog(id, choices_wide, str_wide.c_str(), title_wide.c_str(), type);
    };
    g_main_ctx.core.show_ask_dialog = [](std::string_view id, const char *str, const char *title, bool warning) {
        auto str_wide = IOUtils::to_wide_string(str);
        auto title_wide = IOUtils::to_wide_string(title);
        return DialogService::show_ask_dialog(id, str_wide.c_str(), title_wide.c_str(), warning);
    };
    g_main_ctx.core.show_dialog = [](const char *str, const char *title, core_dialog_type type) {
        auto str_wide = IOUtils::to_wide_string(str);
        auto title_wide = IOUtils::to_wide_string(title);
        DialogService::show_dialog(str_wide.c_str(), title_wide.c_str(), type);
    };
    g_main_ctx.core.show_statusbar = [](const char *str) {
        auto str_wide = IOUtils::to_wide_string(str);
        DialogService::show_statusbar(str_wide.c_str());
    };
    g_main_ctx.core.update_screen = update_screen;
    g_main_ctx.core.copy_video = MGECompositor::copy_video;
    g_main_ctx.core.find_available_rom = RomBrowser::find_available_rom;
    g_main_ctx.core.mge_available = PluginUtil::mge_available;
    g_main_ctx.core.load_screen = MGECompositor::load_screen;
    g_main_ctx.core.st_pre_callback = st_callback_wrapper;
    g_main_ctx.core.get_plugin_names = PluginUtil::get_plugin_names;

    const auto result = core_create(&g_main_ctx.core, &g_main_ctx.core_ctx);

    PluginUtil::init_dummy_and_extended_funcs();

    return result;
}

static void main_dispatcher_init()
{
    g_ui_thread_id = GetCurrentThreadId();
    g_main_ctx.dispatcher =
        std::make_unique<Dispatcher>(g_ui_thread_id, [] { SendMessage(g_main_ctx.hwnd, WM_EXECUTE_DISPATCHER, 0, 0); });
}

void set_cwd()
{
    if (!g_config.keep_default_working_directory)
    {
        SetCurrentDirectory(g_main_ctx.app_path.c_str());
    }

    wchar_t cwd[MAX_PATH] = {0};
    GetCurrentDirectory(sizeof(cwd), cwd);
    g_view_logger->info(L"cwd: {}", cwd);
}

/**
 * \brief Calls IsDialogMessage for problematic modeless child dialogs with no message loops of their own.
 */
static bool is_dialog_message(MSG *msg)
{
    if (IsWindow(LuaDialog::hwnd()) && IsDialogMessage(LuaDialog::hwnd(), msg))
    {
        return true;
    }
    if (IsWindow(CommandPalette::hwnd()) && IsDialogMessage(CommandPalette::hwnd(), msg))
    {
        return true;
    }
    if (IsWindow(ParameterPalette::hwnd()) && IsDialogMessage(ParameterPalette::hwnd(), msg))
    {
        return true;
    }
    if (IsWindow(Seeker::hwnd()) && IsDialogMessage(Seeker::hwnd(), msg))
    {
        return true;
    }
    if (IsWindow(PianoRoll::hwnd()) && IsDialogMessage(PianoRoll::hwnd(), msg))
    {
        return true;
    }
    return false;
}

/**
 * \brief Enables common mitigations.
 */
static void enable_mitigations()
{
    PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handles = {0};
    handles.RaiseExceptionOnInvalidHandleReference = 1;
    handles.HandleExceptionsPermanentlyEnabled = 1;
    RT_ASSERT(SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &handles, sizeof(handles)),
              L"Couldn't set process mitigation policy.");

    PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY ext = {0};
    ext.DisableExtensionPoints = 1;
    RT_ASSERT(SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy, &ext, sizeof(ext)),
              L"Couldn't set process mitigation policy.");
}

/**
 * \brief Calls `SetErrorMode` to disable miscellaneous error popups.
 */
static void set_error_mode()
{
    const UINT prev_mode = GetErrorMode();
    SetErrorMode(prev_mode | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
}

int CALLBACK WinMain(const HINSTANCE hInstance, HINSTANCE, LPSTR, const int nShowCmd)
{
    enable_mitigations();
    set_error_mode();

#ifdef _DEBUG
    open_console();
#endif

    g_main_ctx.app_path = get_app_full_path();

    std::filesystem::create_directories(Config::logs_directory());

    Loggers::init();

    g_view_logger->info("WinMain");
    g_view_logger->info(get_mupen_name());

    g_main_ctx.hinst = hInstance;
    set_cwd();

    Config::init();
    Config::load();
    main_dispatcher_init();

    std::filesystem::create_directories(Config::save_directory());
    std::filesystem::create_directories(Config::screenshot_directory());
    std::filesystem::create_directories(Config::plugin_directory());
    std::filesystem::create_directories(Config::backup_directory());

    const auto core_result = init_core();
    if (core_result != Res_Ok)
    {
        CoreUtils::show_error_dialog_for_result(core_result);
        return 1;
    }

    Gdiplus::GdiplusStartupInput startup_input;
    GdiplusStartup(&gdi_plus_token, &startup_input, NULL);

    const auto hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    RT_ASSERT(SUCCEEDED(hr), L"Failed to initialize COM.");

    LuaManager::init();

    CrashManager::init();
    MGECompositor::init();
    LuaRenderer::init();
    CaptureManager::init();
    CLI::init();
    Seeker::init();
    CoreDbg::init();
    AppActions::init();

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(g_main_ctx.hinst, MAKEINTRESOURCE(IDI_M64ICONBIG));
    wc.hIconSm = LoadIcon(g_main_ctx.hinst, MAKEINTRESOURCE(IDI_M64ICONSMALL));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = WND_CLASS;
    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassEx(&wc);

    g_view_logger->info("[View] Restoring window @ ({}|{}) {}x{}...", g_config.window_x, g_config.window_y,
                        g_config.window_width, g_config.window_height);

    CreateWindow(WND_CLASS, get_titlebar_text().c_str(), WS_OVERLAPPEDWINDOW, g_config.window_x, g_config.window_y,
                 g_config.window_width, g_config.window_height, NULL, NULL, g_main_ctx.hinst, NULL);
    ShowWindow(g_main_ctx.hwnd, nShowCmd);

    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, on_emu_launched_changed);
    Messenger::subscribe(Messenger::Message::EmuStopping, on_emu_stopping);
    Messenger::subscribe(Messenger::Message::EmuPausedChanged, on_emu_paused_changed);
    Messenger::subscribe(Messenger::Message::CapturingChanged, on_capturing_changed);
    Messenger::subscribe(Messenger::Message::MovieLoopChanged, on_movie_loop_changed);
    Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed);
    Messenger::subscribe(Messenger::Message::ScriptStarted, on_script_started);
    Messenger::subscribe(Messenger::Message::SpeedModifierChanged, on_speed_modifier_changed);
    Messenger::subscribe(Messenger::Message::LagLimitExceeded, on_vis_since_input_poll_exceeded);
    Messenger::subscribe(Messenger::Message::FullscreenChanged, on_fullscreen_changed);
    Messenger::subscribe(Messenger::Message::ConfigLoaded, on_config_loaded);
    Messenger::subscribe(Messenger::Message::SeekCompleted, on_seek_completed);
    Messenger::subscribe(Messenger::Message::WarpModifyStatusChanged, on_warp_modify_status_changed);
    Messenger::subscribe(Messenger::Message::FastForwardNeedsUpdate,
                         [](auto) { AppActions::update_core_fast_forward(); });
    Messenger::subscribe(Messenger::Message::SeekStatusChanged, [](auto) { AppActions::update_core_fast_forward(); });
    Messenger::subscribe(Messenger::Message::EmuStartingChanged, on_emu_starting_changed);

    Statusbar::create();
    RomBrowser::create();
    AppActions::update_core_fast_forward();

    Messenger::broadcast(Messenger::Message::StatusbarVisibilityChanged, (bool)g_config.is_statusbar_enabled);
    Messenger::broadcast(Messenger::Message::MovieLoopChanged, (bool)g_config.core.is_movie_loop_enabled);
    Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)g_config.core.vcr_readonly);
    Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, false);
    Messenger::broadcast(Messenger::Message::CoreExecutingChanged, false);
    Messenger::broadcast(Messenger::Message::CapturingChanged, false);
    Messenger::broadcast(Messenger::Message::AppReady, nullptr);
    Messenger::broadcast(Messenger::Message::ConfigLoaded, nullptr);

    g_ui_timer = timeSetEvent(16, 1, invalidate_callback, 0, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
    if (!g_ui_timer)
    {
        DialogService::show_dialog(L"timeSetEvent call failed. Verify that your system supports multimedia timers.",
                                   L"Error", fsvc_error);
        return -1;
    }

    MSG msg{};
    while (true)
    {
        MsgWaitForMultipleObjects(0, nullptr, FALSE, INFINITE, QS_ALLEVENTS | QS_ALLINPUT);
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return (int)msg.wParam;
            if (is_dialog_message(&msg)) continue;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}
