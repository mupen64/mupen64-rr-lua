/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/Messages.hpp>
#include <Common/I18n.hpp>
#include <Common.Views/ActionManager.hpp>
#include <action/AppActions.hpp>
#include <components/Statusbar.hpp>
#include <wil/resource.h>
#include <atomic>
#include <array>

namespace
{
HWND statusbar_hwnd;
HWND tooltip_hwnd;
std::atomic_bool starting{}, state_pending{};
std::atomic_uint64_t rerecords{};
std::array<wil::unique_hicon, 7> icons{};
std::array<int, 7> previous_edges{};
int previous_parts{}, previous_symbol = -2, previous_mode_symbol = -1;
std::array<std::wstring, 7> texts{}, tips{};

std::string shortcut(const std::string &path, std::string_view label_key)
{
    const auto it = g_config.hotkeys.find(ActionManager::normalize_filter(path));
    return std::format("{}: {}", I18n::get().get(label_key),
        it == g_config.hotkeys.end() || it->second.is_empty() ? I18n::get().get("statusbar.shortcut.unassigned")
                                                              : it->second.to_string());
}

void refresh_icons()
{
    static COLORREF previous_background = CLR_INVALID, previous_foreground = CLR_INVALID;
    static int previous_width{}, previous_height{};
    const auto background = WinDarkMode::theme_data().bg_color;
    const auto foreground = WinDarkMode::theme_data().text_1_color;
    const int width = GetSystemMetrics(SM_CXSMICON), height = GetSystemMetrics(SM_CYSMICON);
    if (background == previous_background && foreground == previous_foreground && width == previous_width &&
        height == previous_height)
        return;
    const bool dark = WinDarkMode::effective_theme() == WinDarkMode::EffectiveTheme::Dark;
    // Solid-color contrast against #f0f0f0 / #383838: at least 4.59:1 / 4.30:1.
    const std::array<COLORREF, 5> colors = dark ? std::array<COLORREF, 5>{RGB(75, 205, 115), RGB(95, 170, 255),
                                                      RGB(255, 110, 110), RGB(245, 185, 65), RGB(245, 185, 65)}
                                                : std::array<COLORREF, 5>{RGB(20, 125, 55), RGB(25, 100, 195),
                                                      RGB(190, 35, 35), RGB(145, 90, 0), RGB(145, 90, 0)};
    SendMessage(statusbar_hwnd, SB_SETICON, 0, 0);
    SendMessage(statusbar_hwnd, SB_SETICON, 1, 0);
    for (size_t i = 0; i < icons.size(); ++i)
    {
        icons[i] = load_tinted_png_icon(
            g_main_ctx.hinst, 29001 + static_cast<int>(i), {width, height}, i < colors.size() ? colors[i] : foreground);
    }
    previous_background = background;
    previous_foreground = foreground;
    previous_width = width;
    previous_height = height;
    previous_symbol = -2;
    previous_mode_symbol = -1;
}

void refresh()
{
    if (!statusbar_hwnd || !g_main_ctx.core_ctx) return;
    refresh_icons();
    const auto &core = g_main_ctx.core_ctx;
    const auto task = core->vcr_get_task();
    const bool active = task != CoreVCRTask::Idle;
    const bool launched = core->vr_get_launched();
    int symbol = -1;
    std::string state, state_tip;
    if (starting || state_pending || (active && task != CoreVCRTask::Recording && task != CoreVCRTask::Playback))
    {
        state = I18n::get().get("statusbar.state.loading");
        symbol = 4;
        state_tip = I18n::get().get("statusbar.state.loading.tooltip");
    }
    else if (active && core->vcr_get_warp_modify_status())
    {
        state = I18n::get().get("statusbar.state.modifying");
        symbol = 3;
        state_tip = I18n::get().get("statusbar.state.modifying.tooltip");
    }
    else if (active && core->vcr_is_seeking())
    {
        state = I18n::get().get("statusbar.state.seeking");
        symbol = 3;
        state_tip = I18n::get().get("statusbar.state.seeking.tooltip");
    }
    else if (task == CoreVCRTask::Recording)
    {
        state = I18n::get().get("statusbar.state.recording");
        symbol = 2;
        state_tip = I18n::get().get("statusbar.state.recording.tooltip");
    }
    else if (task == CoreVCRTask::Playback)
    {
        state = I18n::get().get("statusbar.state.playing");
        symbol = 1;
        state_tip = I18n::get().get("statusbar.state.playing.tooltip");
    }
    else if (launched)
    {
        state = I18n::get().get("statusbar.state.emulating");
        symbol = 0;
        state_tip = I18n::get().get("statusbar.state.emulating.tooltip");
    }
    else
        state_tip = I18n::get().get("statusbar.state.stopped.tooltip");
    if (launched && core->vr_get_paused()) state_tip += "\n\n" + I18n::get().get("statusbar.state.paused.tooltip");

    const bool readonly = g_config.core.vcr_readonly;
    const auto slot = g_config.st_slot + 1;
    const auto count = static_cast<int64_t>(g_config.multi_frame_advance_count);
    const auto magnitude = count < 0 ? -count : count;
    float fps{}, vis{};
    static ULONGLONG last_timing{};
    static std::string timing;
    if (timing.empty() || GetTickCount64() - last_timing >= 1000 || !launched)
    {
        if (launched) core->vr_get_timings(fps, vis);
        timing = I18n::get().format("statusbar.timings", fps, vis);
        last_timing = GetTickCount64();
    }
    std::array<std::wstring, 7> next = {IOUtils::to_wide_string(state), L"",
        IOUtils::to_wide_string(I18n::get().format("statusbar.slot", slot)), IOUtils::to_wide_string(timing),
        IOUtils::to_wide_string(I18n::get().format("statusbar.mfa", count)), L"", L""};
    if (active)
    {
        const auto sample =
            static_cast<int64_t>(core->vcr_get_seek_info().current_sample) - (g_config.vcr_0_index ? 1 : 0);
        next[5] = IOUtils::to_wide_string(std::format("{}/{} ({}/{})", core->vcr_get_current_vi(),
            core->vcr_get_length_vis(), std::max<int64_t>(0, sample), core->vcr_get_length_samples()));
        next[6] = IOUtils::to_wide_string(I18n::get().format("statusbar.rerecords", rerecords.load()));
    }
    std::array<std::wstring, 7> next_tips = {IOUtils::to_wide_string(state_tip),
        IOUtils::to_wide_string(
            I18n::get().format(readonly ? "statusbar.readonly.tooltip" : "statusbar.readwrite.tooltip",
                shortcut(AppActions::READONLY, "statusbar.shortcut.switch_mode"))),
        IOUtils::to_wide_string(I18n::get().format("statusbar.slot.tooltip", slot,
            shortcut(AppActions::SAVE_CURRENT_SLOT, "statusbar.shortcut.save_slot"),
            shortcut(AppActions::LOAD_CURRENT_SLOT, "statusbar.shortcut.load_slot"))),
        IOUtils::to_wide_string(I18n::get().get("statusbar.timings.tooltip")),
        IOUtils::to_wide_string(
            I18n::get().format(count < 0 ? "statusbar.mfa.rewind.tooltip" : "statusbar.mfa.advance.tooltip", magnitude,
                shortcut(AppActions::MULTI_FRAME_ADVANCE, "statusbar.shortcut.mfa"))),
        IOUtils::to_wide_string(I18n::get().get("statusbar.movie.tooltip")),
        IOUtils::to_wide_string(I18n::get().get("statusbar.rerecords.tooltip"))};

    const int parts = active ? 7 : 5;
    auto dc = GetDC(statusbar_hwnd);
    const auto font = reinterpret_cast<HFONT>(SendMessage(statusbar_hwnd, WM_GETFONT, 0, 0));
    const auto old_font = SelectObject(dc, font);
    std::array<int, 7> edges{};
    int borders[3]{};
    SendMessage(statusbar_hwnd, SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders));
    int state_text_width = 0;
    for (const auto key : {"statusbar.state.emulating", "statusbar.state.playing", "statusbar.state.recording",
             "statusbar.state.seeking", "statusbar.state.modifying", "statusbar.state.loading"})
    {
        const auto label = IOUtils::to_wide_string(I18n::get().get(key));
        SIZE size{};
        GetTextExtentPoint32W(dc, label.c_str(), static_cast<int>(label.size()), &size);
        state_text_width = std::max(state_text_width, static_cast<int>(size.cx));
    }
    int edge = 0;
    for (int i = 0; i < parts; ++i)
    {
        SIZE size{};
        GetTextExtentPoint32W(dc, next[i].c_str(), static_cast<int>(next[i].size()), &size);
        if (i == 0)
            edge += state_text_width + 18 + GetSystemMetrics(SM_CXSMICON) + 4;
        else if (i == 1)
            edge += GetSystemMetrics(SM_CXSMICON) + 2 * borders[0] + 5;
        else
            edge += size.cx + 18;
        edges[i] = edge;
    }
    SelectObject(dc, old_font);
    ReleaseDC(statusbar_hwnd, dc);
    if (previous_parts != parts || previous_edges != edges)
    {
        SendMessage(statusbar_hwnd, SB_SETPARTS, parts, reinterpret_cast<LPARAM>(edges.data()));
        previous_parts = parts;
        previous_edges = edges;
    }
    tips = std::move(next_tips);
    for (int i = 0; i < 7; ++i)
    {
        TOOLINFOW tool{sizeof(tool)};
        tool.hwnd = statusbar_hwnd;
        tool.uId = i + 1;
        if (i < parts) SendMessage(statusbar_hwnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&tool.rect));
        tool.lpszText = tips[i].data();
        SendMessageW(tooltip_hwnd, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&tool));
        SendMessageW(tooltip_hwnd, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&tool));
        if (i >= parts) continue;
        if (texts[i] != next[i])
            SendMessageW(statusbar_hwnd, SB_SETTEXTW, i, reinterpret_cast<LPARAM>(next[i].c_str()));
    }
    if (previous_symbol != symbol)
    {
        SendMessage(
            statusbar_hwnd, SB_SETICON, 0, reinterpret_cast<LPARAM>(symbol < 0 ? nullptr : icons[symbol].get()));
        previous_symbol = symbol;
    }
    const int mode_symbol = readonly ? 5 : 6;
    if (previous_mode_symbol != mode_symbol)
    {
        SendMessage(statusbar_hwnd, SB_SETICON, 1, reinterpret_cast<LPARAM>(icons[mode_symbol].get()));
        previous_mode_symbol = mode_symbol;
    }
    texts = std::move(next);
}

void CALLBACK tick(HWND, UINT, UINT_PTR, DWORD)
{
    refresh();
}

LRESULT CALLBACK statusbar_subclass(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR)
{
    if (message == WM_LBUTTONUP)
    {
        RECT mode_rect{};
        const POINT position{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        if (SendMessage(hwnd, SB_GETRECT, 1, reinterpret_cast<LPARAM>(&mode_rect)) && PtInRect(&mode_rect, position))
            ActionManager::invoke(AppActions::READONLY);
    }
    else if (message == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hwnd, statusbar_subclass, id);
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

void create_bar()
{
    statusbar_hwnd = CreateWindowEx(0, STATUSCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | CCS_BOTTOM, 0, 0, 0, 0,
        g_main_ctx.hwnd, reinterpret_cast<HMENU>(IDC_MAIN_STATUS), g_main_ctx.hinst, nullptr);
    SetWindowSubclass(statusbar_hwnd, statusbar_subclass, 0, 0);
    texts = {};
    tips = {};
    previous_parts = 0;
    previous_symbol = -2;
    previous_mode_symbol = -1;
    tooltip_hwnd = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, statusbar_hwnd, nullptr, g_main_ctx.hinst, nullptr);
    SendMessage(tooltip_hwnd, TTM_SETMAXTIPWIDTH, 0, 420);
    for (int i = 0; i < 7; ++i)
    {
        TOOLINFOW tool{sizeof(tool)};
        tool.uFlags = TTF_SUBCLASS;
        tool.hwnd = statusbar_hwnd;
        tool.uId = i + 1;
        tool.lpszText = tips[i].data();
        SendMessageW(tooltip_hwnd, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
    }
    refresh();
    SetTimer(statusbar_hwnd, 1, 100, tick);
}
} // namespace

void Statusbar::create()
{
    auto &translations = I18n::get();
    translations.add("statusbar.state.loading", "Loading", "en-us");
    translations.add(
        "statusbar.state.loading.tooltip", "An emulator, movie, or savestate operation is in progress.", "en-us");
    translations.add("statusbar.state.modifying", "Modifying", "en-us");
    translations.add("statusbar.state.modifying.tooltip", "Applying modified movie inputs.", "en-us");
    translations.add("statusbar.state.seeking", "Seeking", "en-us");
    translations.add("statusbar.state.seeking.tooltip", "Seeking to the requested movie frame.", "en-us");
    translations.add("statusbar.state.recording", "Recording", "en-us");
    translations.add("statusbar.state.recording.tooltip", "Recording controller input to the movie.", "en-us");
    translations.add("statusbar.state.playing", "Playing", "en-us");
    translations.add("statusbar.state.playing.tooltip", "Playing controller input from the movie.", "en-us");
    translations.add("statusbar.state.emulating", "Emulating", "en-us");
    translations.add("statusbar.state.emulating.tooltip", "The emulator is running without an active movie.", "en-us");
    translations.add("statusbar.state.stopped.tooltip", "The emulator is stopped.", "en-us");
    translations.add("statusbar.state.paused.tooltip", "Emulation is paused.", "en-us");
    translations.add("statusbar.shortcut.unassigned", "Unassigned", "en-us");
    translations.add("statusbar.shortcut.switch_mode", "Switch mode", "en-us");
    translations.add("statusbar.shortcut.save_slot", "Save slot", "en-us");
    translations.add("statusbar.shortcut.load_slot", "Load slot", "en-us");
    translations.add("statusbar.shortcut.mfa", "Multi-Frame Advance", "en-us");
    translations.add("statusbar.slot", "Slot {}", "en-us");
    translations.add("statusbar.timings", "FPS: {:.1f} VI/s: {:.1f}", "en-us");
    translations.add("statusbar.mfa", "MFA: {}x", "en-us");
    translations.add("statusbar.rerecords", "RR: {}", "en-us");
    translations.add("statusbar.readonly.tooltip",
        "Read-Only Mode\n\nLoading a savestate from a currently playing movie will continue playback from that "
        "frame.\n\n{}",
        "en-us");
    translations.add("statusbar.readwrite.tooltip",
        "Read-Write Mode\n\nLoading a savestate from a currently playing movie will begin recording from that "
        "frame.\n\n{}",
        "en-us");
    translations.add("statusbar.slot.tooltip",
        "The current state slot is {}.\n\nSaving and loading states will use this slot.\n\n{}\n{}", "en-us");
    translations.add("statusbar.timings.tooltip", "Frames per second and vertical interrupts per second.", "en-us");
    translations.add("statusbar.mfa.rewind.tooltip", "Multi-Frame Advance will rewind by {} frames.\n\n{}", "en-us");
    translations.add("statusbar.mfa.advance.tooltip", "Multi-Frame Advance will advance by {} frames.\n\n{}", "en-us");
    translations.add(
        "statusbar.movie.tooltip", "Current/total vertical interrupts (current/total input samples).", "en-us");
    translations.add("statusbar.rerecords.tooltip", "Movie rerecord count.", "en-us");
    create_bar();
    Messenger::subscribe<Messenger::Message::EmuStartingChanged>([](bool value) { starting = value; });
    Messenger::subscribe<Messenger::Message::RerecordsChanged>([](uint64_t value) { rerecords = value; });
    Messenger::subscribe<Messenger::Message::StatusbarVisibilityChanged>([](bool value) {
        if (statusbar_hwnd) DestroyWindow(statusbar_hwnd);
        statusbar_hwnd = nullptr;
        if (value) create_bar();
    });
    // Core callbacks may run off the UI thread; the timer consumes only this atomic flag.
    g_main_ctx.core.st_pending_changed = [](bool value) { state_pending = value; };
}

HWND Statusbar::hwnd()
{
    return statusbar_hwnd;
}

void Statusbar::post(const std::string &, Section)
{
    // Compatibility for Lua and legacy notification callers. The bar now displays application state.
}
