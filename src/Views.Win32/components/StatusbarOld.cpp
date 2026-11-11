/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/Config.hpp>
#include <Common.Views/Messages.hpp>
#include <Common.Win32/BetterStatusbar.hpp>
#include <components/StatusbarOld.hpp>
#include <CaptureManager.hpp>
#include <array>
#include <atomic>
#include <numeric>

namespace
{
enum Section : uint8_t
{
    Readonly,
    VCR,
    Input,
    Rerecords,
    FPS,
    VIs,
    Slot,
    MultiFrameAdvanceCount,
    Count,
};

constexpr std::array<int32_t, Section::Count> SEGMENT_WIDTHS = {200, 180, 80, 70, 80, 80, 50, 60};

HWND statusbar_hwnd;
std::atomic_uint64_t rerecords{};

std::string get_input_text()
{
    const CoreButtons b = g_main_ctx.last_controller_data[0];

    auto text = std::format("({}, {}) ", (int)b.x, (int)b.y);
    if (b.start) text += 'S';
    if (b.z) text += 'Z';
    if (b.a) text += 'A';
    if (b.b) text += 'B';
    if (b.l) text += 'L';
    if (b.r) text += 'R';
    if (b.cu || b.cd || b.cl || b.cr)
    {
        text += " C";
        if (b.cu) text += '^';
        if (b.cd) text += 'v';
        if (b.cl) text += '<';
        if (b.cr) text += '>';
    }
    if (b.du || b.dd || b.dl || b.dr)
    {
        text += 'D';
        if (b.du) text += '^';
        if (b.dd) text += 'v';
        if (b.dl) text += '<';
        if (b.dr) text += '>';
    }
    return text;
}

std::string get_vcr_text()
{
    const auto &core = g_main_ctx.core_ctx;
    const auto task = core->vcr_get_task();

    std::string text;

    if (task != CoreVCRTask::Idle && core->vcr_get_warp_modify_status())
    {
        const auto current_sample = core->vcr_get_seek_info().current_sample;
        text = std::format("Warping ({:.2f}%)",
            static_cast<double>(current_sample) / static_cast<double>(core->vcr_get_length_samples()) * 100.0);
    }
    else if (task == CoreVCRTask::Recording || task == CoreVCRTask::Playback)
    {
        const auto sample =
            static_cast<int64_t>(core->vcr_get_seek_info().current_sample) - (g_config.vcr_0_index ? 1 : 0);

        if (sample >= 0)
        {
            const auto current_vi = core->vcr_get_current_vi();
            text = task == CoreVCRTask::Recording
                       ? std::format("{} ({}) ", current_vi, sample)
                       : std::format("{} / {} ({} / {}) ", current_vi, core->vcr_get_length_vis(), sample,
                             core->vcr_get_length_samples());
        }
    }

    if (CaptureManager::is_capturing())
    {
        text += task == CoreVCRTask::Idle ? std::format("{}", CaptureManager::get_video_frame())
                                          : std::format("({})", CaptureManager::get_video_frame());
    }

    return text;
}

float get_segment_scale()
{
    RECT rect{};
    GetClientRect(g_main_ctx.hwnd, &rect);

    constexpr auto desired_width = std::accumulate(SEGMENT_WIDTHS.begin(), SEGMENT_WIDTHS.end(), 0);
    auto scale = static_cast<float>(rect.right - rect.left) / static_cast<float>(desired_width);
    if (!g_config.statusbar_scale_down) scale = std::max(scale, 1.0f);
    if (!g_config.statusbar_scale_up) scale = std::min(scale, 1.0f);
    return scale;
}

void refresh()
{
    if (!statusbar_hwnd || !g_main_ctx.core_ctx) return;

    const auto &core = g_main_ctx.core_ctx;

    if (!core->vr_get_launched())
    {
        BetterStatusbar::set_parts(statusbar_hwnd, {});
        return;
    }

    std::array<std::string, Section::Count> text;
    text[Readonly] = g_config.core.vcr_readonly ? "Read-only" : "Read/write";
    text[VCR] = get_vcr_text();
    text[Input] = get_input_text();
    if (core->vcr_get_task() != CoreVCRTask::Idle) text[Rerecords] = std::format("{} rr", format_short(rerecords));
    text[Slot] = std::format("Slot {}", g_config.st_slot + 1);
    text[MultiFrameAdvanceCount] = std::format("MFA {}x", g_config.multi_frame_advance_count);

    static ULONGLONG last_timing_update{};
    static std::string fps_text, vis_text;
    const auto now = GetTickCount64();
    if (fps_text.empty() || now - last_timing_update >= 1000)
    {
        float fps{}, vis{};
        core->vr_get_timings(fps, vis);
        fps_text = std::format("FPS: {:.1f}", fps);
        vis_text = std::format("VI/s: {:.1f}", vis);
        last_timing_update = now;
    }
    text[FPS] = fps_text;
    text[VIs] = vis_text;

    const auto scale = get_segment_scale();
    std::array<BetterStatusbar::Part, Section::Count> parts;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        parts[i].text = IOUtils::to_wide_string(text[i]);
        parts[i].width = std::max(1, static_cast<int>(static_cast<float>(SEGMENT_WIDTHS[i]) * scale));
    }
    BetterStatusbar::set_parts(statusbar_hwnd, parts);
}

void CALLBACK tick(HWND, UINT, UINT_PTR, DWORD)
{
    refresh();
}
} // namespace

void StatusbarOld::init()
{
    Messenger::subscribe<Messenger::Message::RerecordsChanged>([](uint64_t value) { rerecords = value; });
}

HWND StatusbarOld::create_window()
{
    statusbar_hwnd = BetterStatusbar::create(g_main_ctx.hwnd, g_main_ctx.hinst, IDC_MAIN_STATUS);
    if (!statusbar_hwnd) return nullptr;
    refresh();
    SetTimer(statusbar_hwnd, 1, 1000 / 60, tick);
    return statusbar_hwnd;
}

void StatusbarOld::destroy_window()
{
    if (statusbar_hwnd) DestroyWindow(statusbar_hwnd);
    statusbar_hwnd = nullptr;
}

HWND StatusbarOld::hwnd()
{
    return statusbar_hwnd;
}
