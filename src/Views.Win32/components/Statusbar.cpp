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
#include <components/StatusbarOld.hpp>
#include <CaptureManager.hpp>
#include <wil/resource.h>
#include <Common.Win32/BetterStatusbar.hpp>
#include <atomic>
#include <array>

namespace
{
HWND statusbar_hwnd;
std::atomic_bool starting{}, state_pending{};
std::atomic_uint64_t rerecords{};
std::array<wil::unique_hicon, 16> icons{};
bool legacy_active{};

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
    BetterStatusbar::clear_icons(statusbar_hwnd);
    for (size_t i = 0; i < icons.size(); ++i)
    {
        const auto tint = i == 14              ? colors[0]
                          : i == 10 || i == 13 ? colors[2]
                          : i < colors.size()  ? colors[i]
                                               : foreground;
        const auto resource_id = i >= 13 ? 29000 + static_cast<int>(i) : 29001 + static_cast<int>(i);
        icons[i] = load_tinted_png_icon(g_main_ctx.hinst, resource_id, {width, height}, tint);
    }
    previous_background = background;
    previous_foreground = foreground;
    previous_width = width;
    previous_height = height;
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
    {
        state = I18n::get().get("statusbar.state.stopped");
        symbol = 7;
        state_tip = I18n::get().get("statusbar.state.stopped.tooltip");
    }
    if (launched && core->vr_get_paused()) state_tip += "\n\n" + I18n::get().get("statusbar.state.paused.tooltip");

    const bool readonly = g_config.core.vcr_readonly;
    const auto slot = g_config.st_slot + 1;
    const auto count = static_cast<int64_t>(g_config.multi_frame_advance_count);
    const auto magnitude = count < 0 ? -count : count;
    static ULONGLONG last_timing_update{};
    static std::string timing;
    const auto now = GetTickCount64();
    if (timing.empty() || now - last_timing_update >= 1000)
    {
        float fps{}, vis{};
        if (launched) core->vr_get_timings(fps, vis);
        timing = I18n::get().format("statusbar.timings", fps, vis);
        last_timing_update = now;
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

    std::array<BetterStatusbar::Part, 11> parts;
    for (size_t i = 0; i < next.size(); ++i)
    {
        const auto part_index = i < 2 ? i : i + 4;
        parts[part_index].text = std::move(next[i]);
        parts[part_index].tooltip = std::move(next_tips[i]);
    }
    parts[0].icon = symbol < 0 ? nullptr : icons[symbol].get();
    parts[0].reserve_icon = true;
    for (const auto key :
        {"statusbar.state.emulating", "statusbar.state.playing", "statusbar.state.recording", "statusbar.state.seeking",
            "statusbar.state.modifying", "statusbar.state.loading", "statusbar.state.stopped"})
        parts[0].sizing_labels.push_back(IOUtils::to_wide_string(I18n::get().get(key)));
    parts[1].icon = icons[readonly ? 5 : 6].get();
    parts[1].reserve_icon = true;
    parts[1].padding = 5;
    parts[1].on_click = [] { ActionManager::invoke(AppActions::READONLY); };
    parts[2].reserve_icon = true;
    parts[2].padding = 5;
    parts[2].icon = icons[g_config.core.is_movie_loop_enabled ? 9 : 8].get();
    parts[2].tooltip = IOUtils::to_wide_string(I18n::get().format(
        g_config.core.is_movie_loop_enabled ? "statusbar.loop.tooltip" : "statusbar.loop.disabled.tooltip",
        shortcut(AppActions::LOOP_MOVIE_PLAYBACK, "statusbar.shortcut.loop")));
    parts[2].on_click = [] { ActionManager::invoke(AppActions::LOOP_MOVIE_PLAYBACK); };
    const bool capturing = CaptureManager::is_capturing();
    parts[3].reserve_icon = true;
    parts[3].padding = 5;
    parts[3].icon = icons[capturing ? 10 : 11].get();
    parts[3].tooltip = IOUtils::to_wide_string(
        I18n::get().format(capturing ? "statusbar.capture.active.tooltip" : "statusbar.capture.inactive.tooltip",
            shortcut(AppActions::VIDEO_CAPTURE_START, "statusbar.shortcut.capture.start"),
            shortcut(AppActions::VIDEO_CAPTURE_STOP, "statusbar.shortcut.capture.stop")));
    parts[3].on_click = [capturing] {
        ActionManager::invoke(capturing ? AppActions::VIDEO_CAPTURE_STOP : AppActions::VIDEO_CAPTURE_START);
    };
    const auto speed = g_config.core.fps_modifier / 100.0;
    std::string target_rate;
    if (launched)
    {
        const auto base_vis = core->vr_get_vis_per_second(core->vr_get_rom_header()->Country_code);
        target_rate = I18n::get().format("statusbar.speed.target", base_vis * speed) + "\n\n";
    }
    parts[5].text = IOUtils::to_wide_string(I18n::get().format("statusbar.speed", speed));
    parts[5].icon = icons[g_config.core.fps_modifier == 100 ? 12 : 13].get();
    parts[5].tooltip = IOUtils::to_wide_string(I18n::get().format("statusbar.speed.tooltip", target_rate,
        shortcut(AppActions::SPEED_UP, "statusbar.shortcut.speed.up"),
        shortcut(AppActions::SPEED_DOWN, "statusbar.shortcut.speed.down"),
        shortcut(AppActions::SPEED_RESET, "statusbar.shortcut.speed.reset")));
    parts[5].on_click = [] { ActionManager::invoke(AppActions::SPEED_RESET); };
    const bool cheats_active = core->cht_has_active();
    parts[4].reserve_icon = true;
    parts[4].padding = 5;
    parts[4].icon = icons[cheats_active ? 14 : 15].get();
    auto cheats_tooltip =
        I18n::get().get(cheats_active ? "statusbar.cheats.active.tooltip" : "statusbar.cheats.inactive.tooltip");
    if (core->cht_has_override()) cheats_tooltip += "\n\n" + I18n::get().get("statusbar.cheats.override.tooltip");
    if (launched) cheats_tooltip += "\n\n" + I18n::get().get("statusbar.cheats.open.tooltip");
    cheats_tooltip += "\n\n" + shortcut(AppActions::CHEATS, "statusbar.shortcut.cheats");
    parts[4].tooltip = IOUtils::to_wide_string(cheats_tooltip);
    parts[4].on_click = [] { ActionManager::invoke(AppActions::CHEATS); };
    BetterStatusbar::set_parts(statusbar_hwnd, std::span<const BetterStatusbar::Part>(parts.data(), active ? 11 : 9));
}

void CALLBACK tick(HWND, UINT, UINT_PTR, DWORD)
{
    refresh();
}

void create_bar()
{
    statusbar_hwnd = BetterStatusbar::create(g_main_ctx.hwnd, g_main_ctx.hinst, IDC_MAIN_STATUS);
    if (!statusbar_hwnd) return;
    refresh();
    SetTimer(statusbar_hwnd, 1, 1000 / 60, tick);
}

void destroy_bar()
{
    if (statusbar_hwnd) DestroyWindow(statusbar_hwnd);
    statusbar_hwnd = nullptr;
}

void recreate(const bool visible)
{
    destroy_bar();
    StatusbarOld::destroy_window();
    legacy_active = g_config.use_legacy_statusbar;
    if (!visible) return;
    if (legacy_active)
        StatusbarOld::create_window();
    else
        create_bar();
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
    translations.add("statusbar.state.recording.tooltip", "Recording a movie.", "en-us");
    translations.add("statusbar.state.playing", "Playing", "en-us");
    translations.add("statusbar.state.playing.tooltip", "Playing a movie.", "en-us");
    translations.add("statusbar.state.emulating", "Emulating", "en-us");
    translations.add("statusbar.state.emulating.tooltip", "The emulator is running.", "en-us");
    translations.add("statusbar.state.stopped", "Not Running", "en-us");
    translations.add("statusbar.state.stopped.tooltip", "The emulator is stopped.", "en-us");
    translations.add("statusbar.state.paused.tooltip", "Emulation is paused.", "en-us");
    translations.add("statusbar.shortcut.unassigned", "Unassigned", "en-us");
    translations.add("statusbar.shortcut.switch_mode", "Switch mode", "en-us");
    translations.add("statusbar.shortcut.save_slot", "Save slot", "en-us");
    translations.add("statusbar.shortcut.load_slot", "Load slot", "en-us");
    translations.add("statusbar.shortcut.mfa", "Multi-Frame Advance", "en-us");
    translations.add("statusbar.shortcut.loop", "Toggle movie loop", "en-us");
    translations.add("statusbar.shortcut.cheats", "Open Cheats", "en-us");
    translations.add("statusbar.cheats.active.tooltip", "One or more cheats are enabled.", "en-us");
    translations.add("statusbar.cheats.inactive.tooltip", "No cheats are enabled.", "en-us");
    translations.add(
        "statusbar.cheats.override.tooltip", "Cheats are currently managed by the core and can't be changed.", "en-us");
    translations.add("statusbar.cheats.open.tooltip", "Click to open the Cheats dialog.", "en-us");
    translations.add("statusbar.speed", "{:g}x", "en-us");
    translations.add("statusbar.speed.tooltip",
        "Emulator speed multiplier.\n\n{}Click to reset speed to 1x.\n\n{}\n{}\n{}", "en-us");
    translations.add("statusbar.speed.target", "Target rate: {:.1f} VI/s", "en-us");
    translations.add("statusbar.shortcut.speed.up", "Speed up", "en-us");
    translations.add("statusbar.shortcut.speed.down", "Speed down", "en-us");
    translations.add("statusbar.shortcut.speed.reset", "Reset speed", "en-us");
    translations.add("statusbar.shortcut.capture.start", "Start capture", "en-us");
    translations.add("statusbar.shortcut.capture.stop", "Stop capture", "en-us");
    translations.add("statusbar.capture.active.tooltip",
        "Video capture is active.\n\nEmulator video and audio are being saved to a video file.\n\nClick to stop "
        "capture.\n\n{}\n{}",
        "en-us");
    translations.add("statusbar.capture.inactive.tooltip",
        "Video capture is inactive.\n\nClick to start capture.\n\n{}\n{}", "en-us");
    translations.add("statusbar.loop.disabled.tooltip",
        "Movie looping is disabled.\n\nPlayback will stop when the movie ends.\n\nClick to toggle movie looping.\n\n{}",
        "en-us");
    translations.add("statusbar.loop.tooltip",
        "Movie looping is enabled.\n\nPlayback will restart when the movie ends.\n\nClick to toggle movie "
        "looping.\n\n{}",
        "en-us");
    translations.add("statusbar.slot", "Slot {}", "en-us");
    translations.add("statusbar.timings", "FPS: {:.1f} · VI/s: {:.1f}", "en-us");
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
    StatusbarOld::init();
    recreate(true);
    Messenger::subscribe<Messenger::Message::EmuStartingChanged>([](bool value) { starting = value; });
    Messenger::subscribe<Messenger::Message::RerecordsChanged>([](uint64_t value) { rerecords = value; });
    Messenger::subscribe<Messenger::Message::StatusbarVisibilityChanged>(recreate);
    Messenger::subscribe<Messenger::Message::ConfigLoaded>([] {
        if (static_cast<bool>(g_config.use_legacy_statusbar) == legacy_active) return;
        Messenger::broadcast<Messenger::Message::StatusbarVisibilityChanged>((bool)g_config.is_statusbar_enabled);
    });
    // Core callbacks may run off the UI thread; the timer consumes only this atomic flag.
    g_main_ctx.core.st_pending_changed = [](bool value) { state_pending = value; };
}

HWND Statusbar::hwnd()
{
    return legacy_active ? StatusbarOld::hwnd() : statusbar_hwnd;
}
