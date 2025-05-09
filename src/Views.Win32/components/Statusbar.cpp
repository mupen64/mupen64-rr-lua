/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <Messenger.h>
#include <Main.h>
#include <components/Statusbar.h>

struct t_segment {
    std::vector<Statusbar::Section> sections;
    size_t width;
};

struct t_segment_layout {
    std::vector<t_segment> emu_parts;
    std::vector<t_segment> idle_parts;
};

const std::unordered_map<t_config::StatusbarLayout, t_segment_layout> LAYOUT_MAP = {
{t_config::StatusbarLayout::Classic, t_segment_layout{
                                     .emu_parts = {
                                     t_segment{
                                     .sections = {Statusbar::Section::VCR, Statusbar::Section::Notification},
                                     .width = 260,
                                     },
                                     t_segment{
                                     .sections = {Statusbar::Section::FPS},
                                     .width = 70,
                                     },
                                     t_segment{
                                     .sections = {Statusbar::Section::VIs},
                                     .width = 70,
                                     },
                                     t_segment{
                                     .sections = {Statusbar::Section::Input},
                                     .width = 140,
                                     },
                                     },
                                     .idle_parts = {},
                                     }},
{t_config::StatusbarLayout::Modern, t_segment_layout{
                                    .emu_parts = {
                                    t_segment{
                                    .sections = {Statusbar::Section::Notification, Statusbar::Section::Readonly},
                                    .width = 200,
                                    },
                                    t_segment{
                                    .sections = {Statusbar::Section::VCR},
                                    .width = 180,
                                    },
                                    t_segment{
                                    .sections = {Statusbar::Section::Input},
                                    .width = 80,
                                    },
                                    t_segment{
                                    .sections = {Statusbar::Section::Rerecords},
                                    .width = 70,
                                    },
                                    t_segment{
                                    .sections = {Statusbar::Section::FPS},
                                    .width = 80,
                                    },
                                    t_segment{
                                    .sections = {Statusbar::Section::VIs},
                                    .width = 80,
                                    },
                                    t_segment{
                                    .sections = {Statusbar::Section::Slot},
                                    .width = 50,
                                    },
                                    t_segment{
                                    .sections = {Statusbar::Section::MultiFrameAdvanceCount},
                                    .width = 60,
                                    },
                                    },
                                    .idle_parts = {},
                                    }},
{t_config::StatusbarLayout::ModernWithReadOnly, t_segment_layout{
                                                .emu_parts = {
                                                t_segment{
                                                .sections = {Statusbar::Section::Notification},
                                                .width = 150,
                                                },
                                                t_segment{
                                                .sections = {Statusbar::Section::VCR},
                                                .width = 160,
                                                },
                                                t_segment{
                                                .sections = {Statusbar::Section::Readonly},
                                                .width = 80,
                                                },
                                                t_segment{
                                                .sections = {Statusbar::Section::Input},
                                                .width = 80,
                                                },
                                                t_segment{
                                                .sections = {Statusbar::Section::Rerecords},
                                                .width = 70,
                                                },
                                                t_segment{
                                                .sections = {Statusbar::Section::FPS},
                                                .width = 80,
                                                },
                                                t_segment{
                                                .sections = {Statusbar::Section::VIs},
                                                .width = 80,
                                                },
                                                t_segment{
                                                .sections = {Statusbar::Section::Slot},
                                                .width = 50,
                                                },
                                                t_segment{
                                                .sections = {Statusbar::Section::MultiFrameAdvanceCount},
                                                .width = 60,
                                                },
                                                },
                                                .idle_parts = {},
                                                }}};


static HWND statusbar_hwnd;

static std::vector<t_segment> get_current_parts()
{
    const t_segment_layout layout = LAYOUT_MAP.at(static_cast<t_config::StatusbarLayout>(g_config.statusbar_layout));
    return (core_vr_get_launched()) ? layout.emu_parts : layout.idle_parts;
}

static size_t section_to_segment_index(const Statusbar::Section section)
{
    size_t i = 0;

    for (const auto& part : get_current_parts())
    {
        if (std::ranges::any_of(part.sections, [=](const auto s) {
                return s == section;
            }))
        {
            return i;
        }

        i++;
    }

    return SIZE_MAX;
}

static void refresh_segments()
{
    const auto parts = get_current_parts();

    std::vector<int32_t> sizes;
    for (const auto& part : parts)
    {
        sizes.emplace_back(static_cast<int32_t>(part.width));
    }

    RECT rect{};
    GetClientRect(g_main_hwnd, &rect);

    if (parts.empty())
    {
        set_statusbar_parts(statusbar_hwnd, {-1});
        return;
    }

    // Compute the desired size of the statusbar and use that for the scaling factor
    auto desired_size = std::accumulate(sizes.begin(), sizes.end(), 0);

    auto scale = static_cast<float>(rect.right - rect.left) / static_cast<float>(desired_size);

    if (!g_config.statusbar_scale_down)
    {
        scale = std::max(scale, 1.0f);
    }

    if (!g_config.statusbar_scale_up)
    {
        scale = std::min(scale, 1.0f);
    }

    for (auto& size : sizes)
    {
        size = static_cast<int>(size * scale);
    }

    set_statusbar_parts(statusbar_hwnd, sizes);
}

static void emu_launched_changed(std::any data)
{
    auto value = std::any_cast<bool>(data);
    static auto previous_value = value;

    if (!statusbar_hwnd)
    {
        previous_value = value;
        return;
    }

    // We don't want to keep the weird crap from previous state, so let's clear everything
    for (int i = 0; i < 255; ++i)
    {
        SendMessage(statusbar_hwnd, SB_SETTEXT, i, (LPARAM)L"");
    }

    // When starting the emu, we want to scale the statusbar segments to the window size
    if (value)
    {
        // Update this at first start, otherwise it doesnt initially appear
        Messenger::broadcast(Messenger::Message::SlotChanged, (size_t)g_config.st_slot);
        Messenger::broadcast(Messenger::Message::MultiFrameAdvanceCountChanged, 0);
    }

    refresh_segments();

    previous_value = value;
}

static void create()
{
    // undocumented behaviour of CCS_BOTTOM: it skips applying SBARS_SIZEGRIP in style pre-computation phase
    statusbar_hwnd = CreateWindowEx(0, STATUSCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | CCS_BOTTOM, 0, 0, 0, 0, g_main_hwnd, (HMENU)IDC_MAIN_STATUS, g_app_instance, nullptr);
}

static void statusbar_visibility_changed(std::any data)
{
    auto value = std::any_cast<bool>(data);

    if (statusbar_hwnd)
    {
        DestroyWindow(statusbar_hwnd);
        statusbar_hwnd = nullptr;
    }

    if (value)
    {
        create();
    }
}

static void on_readonly_changed(std::any data)
{
    auto value = std::any_cast<bool>(data);
    post(value ? L"Read-only" : L"Read/write", Statusbar::Section::Readonly);
}

static void on_rerecords_changed(std::any data)
{
    auto value = std::any_cast<uint64_t>(data);
    post(std::format(L"{} rr", value), Statusbar::Section::Rerecords);
}

static void on_task_changed(std::any data)
{
    auto value = std::any_cast<core_vcr_task>(data);

    if (value == task_idle)
    {
        post(L"", Statusbar::Section::Rerecords);
    }
}

static void on_slot_changed(std::any data)
{
    auto value = std::any_cast<size_t>(data);
    post(std::format(L"Slot {}", value + 1), Statusbar::Section::Slot);
}

static void on_size_changed(std::any)
{
    refresh_segments();
}

static void on_multi_frame_advance_count_changed(std::any)
{
    post(std::format(L"MFA {}x", g_config.multi_frame_advance_count), Statusbar::Section::MultiFrameAdvanceCount);
}

void Statusbar::create()
{
    ::create();
    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, emu_launched_changed);
    Messenger::subscribe(Messenger::Message::StatusbarVisibilityChanged, statusbar_visibility_changed);
    Messenger::subscribe(Messenger::Message::ReadonlyChanged, on_readonly_changed);
    Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed);
    Messenger::subscribe(Messenger::Message::RerecordsChanged, on_rerecords_changed);
    Messenger::subscribe(Messenger::Message::SlotChanged, on_slot_changed);
    Messenger::subscribe(Messenger::Message::MultiFrameAdvanceCountChanged, on_multi_frame_advance_count_changed);
    Messenger::subscribe(Messenger::Message::SizeChanged, on_size_changed);
    Messenger::subscribe(Messenger::Message::ConfigLoaded, [](std::any) {
        std::unordered_map<Section, std::wstring> section_text;

        for (int i = 0; i <= static_cast<int32_t>(Section::Slot); ++i)
        {
            const auto section = static_cast<Section>(i);

            const auto segment_index = section_to_segment_index(section);

            if (segment_index == SIZE_MAX)
            {
                continue;
            }

            const auto len = SendMessage(statusbar_hwnd, SB_GETTEXTLENGTH, segment_index, 0);

            auto str = (wchar_t*)calloc(len + 1, sizeof(wchar_t));

            SendMessage(statusbar_hwnd, SB_GETTEXT, segment_index, (LPARAM)str);

            section_text[section] = str;

            free(str);
        }

        refresh_segments();

        for (int i = 0; i < 255; ++i)
        {
            SendMessage(statusbar_hwnd, SB_SETTEXT, i, (LPARAM)L"");
        }

        for (const auto pair : section_text)
        {
            post(pair.second, pair.first);
        }
    });
}

void Statusbar::post(const std::wstring& text, Section section)
{
    const auto segment_index = section_to_segment_index(section);

    if (segment_index == SIZE_MAX)
    {
        return;
    }

    SendMessage(statusbar_hwnd, SB_SETTEXT, segment_index, (LPARAM)text.c_str());
}

HWND Statusbar::hwnd()
{
    return statusbar_hwnd;
}
