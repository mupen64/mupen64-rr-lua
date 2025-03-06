/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <Messenger.h>
#include <gui/Main.h>
#include <gui/features/Statusbar.h>

namespace Statusbar
{
	typedef struct Segment
	{
		std::vector<Section> sections;
		size_t width;
	} t_segment;

	typedef struct
	{
		std::vector<t_segment> emu_parts;
		std::vector<t_segment> idle_parts;
	} t_segment_layout;

	std::unordered_map<cfg_statusbar_layout, t_segment_layout> g_layout_map = {
		{
			LAYOUT_CLASSIC, t_segment_layout{
				.emu_parts = {
					t_segment{
						.sections = {Section::VCR, Section::Notification},
						.width = 260,
					},
					t_segment{
						.sections = {Section::FPS},
						.width = 70,
					},
					t_segment{
						.sections = {Section::VIs},
						.width = 70,
					},
					t_segment{
						.sections = {Section::Input},
						.width = 140,
					},
				},
				.idle_parts = {},
			}
		},
		{
			LAYOUT_MODERN, t_segment_layout{
				.emu_parts = {
					t_segment{
						.sections = {Section::Notification, Section::Readonly},
						.width = 200,
					},
					t_segment{
						.sections = {Section::VCR},
						.width = 180,
					},
					t_segment{
						.sections = {Section::Input},
						.width = 80,
					},
					t_segment{
						.sections = {Section::Rerecords},
						.width = 70,
					},
					t_segment{
						.sections = {Section::FPS},
						.width = 80,
					},
					t_segment{
						.sections = {Section::VIs},
						.width = 80,
					},
					t_segment{
						.sections = {Section::Slot},
						.width = 50,
					},
				    t_segment{
				        .sections = {Section::MultiFrameAdvanceCount},
                        .width = 60,
                    },
				},
				.idle_parts = {},
			}
		},
		{
			LAYOUT_MODERN_WITH_READONLY, t_segment_layout{
				.emu_parts = {
					t_segment{
						.sections = {Section::Notification},
						.width = 150,
					},
					t_segment{
						.sections = {Section::VCR},
						.width = 160,
					},
					t_segment{
						.sections = {Section::Readonly},
						.width = 80,
					},
					t_segment{
						.sections = {Section::Input},
						.width = 80,
					},
					t_segment{
						.sections = {Section::Rerecords},
						.width = 70,
					},
					t_segment{
						.sections = {Section::FPS},
						.width = 80,
					},
					t_segment{
						.sections = {Section::VIs},
						.width = 80,
					},
				    t_segment{
				        .sections = {Section::Slot},
                        .width = 50,
                    },
                    t_segment{
                        .sections = {Section::MultiFrameAdvanceCount},
                        .width = 60,
                    },
				},
				.idle_parts = {},
			}
		}
	};


	HWND statusbar_hwnd;

	HWND hwnd()
	{
		return statusbar_hwnd;
	}

	std::vector<t_segment> get_current_parts()
	{
		const t_segment_layout layout = g_layout_map[static_cast<cfg_statusbar_layout>(g_config.statusbar_layout)];
		return (core_vr_get_launched()) ? layout.emu_parts : layout.idle_parts;
	}

	size_t section_to_segment_index(const Section section)
	{
		size_t i = 0;

		for (const auto& part : get_current_parts())
		{
			if (std::ranges::any_of(part.sections, [=](const auto s)
			{
				return s == section;
			}))
			{
				return i;
			}

			i++;
		}

		return SIZE_MAX;
	}

	void post(const std::wstring& text, Section section)
	{
		const auto segment_index = section_to_segment_index(section);

		if (segment_index == SIZE_MAX)
		{
			return;
		}

		SendMessage(statusbar_hwnd, SB_SETTEXT, segment_index, (LPARAM)text.c_str());
	}

	void refresh_segments()
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

	void emu_launched_changed(std::any data)
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

	void create()
	{
		// undocumented behaviour of CCS_BOTTOM: it skips applying SBARS_SIZEGRIP in style pre-computation phase
		statusbar_hwnd = CreateWindowEx(0, STATUSCLASSNAME, nullptr,
		                                WS_CHILD | WS_VISIBLE | CCS_BOTTOM,
		                                0, 0,
		                                0, 0,
		                                g_main_hwnd, (HMENU)IDC_MAIN_STATUS,
		                                g_app_instance, nullptr);
	}


	void statusbar_visibility_changed(std::any data)
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

	void on_readonly_changed(std::any data)
	{
		auto value = std::any_cast<bool>(data);
		post(value ? L"Read-only" : L"Read/write", Section::Readonly);
	}

	void on_rerecords_changed(std::any data)
	{
		auto value = std::any_cast<uint64_t>(data);
		post(std::format(L"{} rr", value), Section::Rerecords);
	}

	void on_task_changed(std::any data)
	{
		auto value = std::any_cast<core_vcr_task>(data);

		if (value == task_idle)
		{
			post(L"", Section::Rerecords);
		}
	}

	void on_slot_changed(std::any data)
	{
		auto value = std::any_cast<size_t>(data);
		post(std::format(L"Slot {}", value + 1), Section::Slot);
	}

	void on_size_changed(std::any)
	{
		refresh_segments();
	}

    void on_multi_frame_advance_count_changed(std::any)
    {
        post(std::format(L"MFA {}x", g_config.multi_frame_advance_count), Section::MultiFrameAdvanceCount);
    }

	void init()
	{
		create();
		Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, emu_launched_changed);
		Messenger::subscribe(Messenger::Message::StatusbarVisibilityChanged, statusbar_visibility_changed);
		Messenger::subscribe(Messenger::Message::ReadonlyChanged, on_readonly_changed);
		Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed);
		Messenger::subscribe(Messenger::Message::RerecordsChanged, on_rerecords_changed);
		Messenger::subscribe(Messenger::Message::SlotChanged, on_slot_changed);
		Messenger::subscribe(Messenger::Message::MultiFrameAdvanceCountChanged, on_multi_frame_advance_count_changed);
		Messenger::subscribe(Messenger::Message::SizeChanged, on_size_changed);
		Messenger::subscribe(Messenger::Message::ConfigLoaded, [](std::any)
		{
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
}
