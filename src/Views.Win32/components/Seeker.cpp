/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/Messages.hpp>
#include <Common.Views/Config.hpp>
#include <components/Seeker.hpp>
#include <components/CoreUtils.hpp>

#define WM_SEEK_COMPLETED (WM_USER + 11)

struct seeker_state
{
    HWND hwnd{};
    UINT_PTR refresh_timer{};
    std::uint64_t seek_start_tick{};
};

static seeker_state seeker{};

static INT_PTR CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        seeker.hwnd = hwnd;

        SetDlgItemText(hwnd, IDC_SEEKER_STATUS, "Idle");
        SetDlgItemText(hwnd, IDC_SEEKER_START, "Start");
        EnableWindow(GetDlgItem(hwnd, IDC_SEEKER_STOP), FALSE);
        SetDlgItemText(hwnd, IDC_SEEKER_FRAME, g_config.seeker_value.c_str());

        if (g_config.core.seek_savestate_interval == 0)
            SetDlgItemText(hwnd, IDC_SEEKER_SUBTEXT, "Seek savestates disabled. Seeking backwards will be slower.");
        else
        {
            ShowWindow(GetDlgItem(hwnd, IDC_SEEKER_SUBTEXT), SW_HIDE);

            RECT shrink = {0, 0, 0, 13};
            MapDialogRect(hwnd, &shrink);

            for (const int id : {IDC_SEEKER_STOP, IDC_SEEKER_START, IDCANCEL})
            {
                const HWND item = GetDlgItem(hwnd, id);
                RECT rc;
                GetWindowRect(item, &rc);
                MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&rc), 2);
                SetWindowPos(item, nullptr, rc.left, rc.top - shrink.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }

            RECT window;
            GetWindowRect(hwnd, &window);
            SetWindowPos(hwnd, nullptr, 0, 0, window.right - window.left, window.bottom - window.top - shrink.bottom,
                SWP_NOMOVE | SWP_NOZORDER);
        }

        SetFocus(GetDlgItem(hwnd, IDC_SEEKER_FRAME));
        WinDarkMode::attach(hwnd);
        break;
    case WM_DESTROY:
        g_main_ctx.core_ctx->vcr_stop_seek();
        KillTimer(hwnd, seeker.refresh_timer);
        EnableWindow(g_main_ctx.hwnd, TRUE);
        SetForegroundWindow(g_main_ctx.hwnd);
        SetActiveWindow(g_main_ctx.hwnd);
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_SEEK_COMPLETED:
        SetDlgItemText(hwnd, IDC_SEEKER_STATUS, "Seek completed");
        SetDlgItemText(hwnd, IDC_SEEKER_ETA, "");
        EnableWindow(GetDlgItem(hwnd, IDC_SEEKER_STOP), FALSE);
        KillTimer(hwnd, seeker.refresh_timer);
        break;
    case WM_TIMER: {
        if (!g_main_ctx.core_ctx->vcr_is_seeking())
        {
            SetDlgItemText(hwnd, IDC_SEEKER_ETA, "");
            break;
        }
        const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();

        const float effective_progress = MiscHelpers::remap(static_cast<float>(info.current_sample),
            static_cast<float>(info.seek_start_sample), static_cast<float>(info.seek_target_sample), 0.0f, 1.0f);
        const auto str = std::format("Seeked {:.2f}%", effective_progress * 100.0);
        SetDlgItemText(hwnd, IDC_SEEKER_STATUS, str.c_str());

        if (effective_progress > 0.0f)
        {
            const double elapsed = static_cast<double>(GetTickCount64() - seeker.seek_start_tick) / 1000.0;
            const double eta = elapsed * (1.0 - effective_progress) / effective_progress;
            SetDlgItemText(hwnd, IDC_SEEKER_ETA, format_eta(eta).c_str());
        }
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_SEEKER_FRAME: {
            char str[260] = {0};
            GetDlgItemText(hwnd, IDC_SEEKER_FRAME, str, std::size(str));
            g_config.seeker_value = str;
        }
        break;
        case IDC_SEEKER_START: {
            const auto result = g_main_ctx.core_ctx->vcr_begin_seek(g_config.seeker_value, true);
            if (result != Res_Ok)
            {
                const auto [_, error] = CoreUtils::get_error_message_for_result(result);
                EnableWindow(GetDlgItem(hwnd, IDC_SEEKER_STOP), FALSE);
                SetDlgItemText(hwnd, IDC_SEEKER_STATUS, error.c_str());
                break;
            }

            const bool is_seeking = g_main_ctx.core_ctx->vcr_is_seeking();
            EnableWindow(GetDlgItem(hwnd, IDC_SEEKER_STOP), is_seeking);
            if (is_seeking)
            {
                SetFocus(GetDlgItem(hwnd, IDC_SEEKER_STOP));
                seeker.seek_start_tick = GetTickCount64();
                SetDlgItemText(hwnd, IDC_SEEKER_ETA, "");
                seeker.refresh_timer = SetTimer(hwnd, 0, 1000 / 10, nullptr);
            }

            break;
        }
        case IDC_SEEKER_STOP:
            g_main_ctx.core_ctx->vcr_stop_seek();
            break;
        case IDCANCEL:
            DestroyWindow(hwnd);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

void Seeker::init()
{
    Messenger::subscribe<Messenger::Message::SeekCompleted>([] {
        if (!seeker.hwnd) return;
        SendMessage(seeker.hwnd, WM_SEEK_COMPLETED, 0, 0);
    });
}

void Seeker::show()
{
    CreateDialog(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_SEEKER), g_main_ctx.hwnd, dlgproc);
    EnableWindow(g_main_ctx.hwnd, FALSE);
    ShowWindow(seeker.hwnd, SW_SHOW);
}

HWND Seeker::hwnd()
{
    return seeker.hwnd;
}
