/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Messenger.h>
#include <Config.h>
#include <components/Seeker.h>

#define WM_SEEK_COMPLETED (WM_USER + 11)

struct seeker_state
{
    HWND hwnd{};
    UINT_PTR refresh_timer{};
};

static seeker_state seeker{};

static INT_PTR CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        seeker.hwnd = hwnd;

        SetDlgItemText(hwnd, IDC_SEEKER_STATUS, L"Idle");
        SetDlgItemText(hwnd, IDC_SEEKER_START, L"Start");
        SetDlgItemText(hwnd, IDC_SEEKER_FRAME, g_config.seeker_value.c_str());

        SetFocus(GetDlgItem(hwnd, IDC_SEEKER_FRAME));
        break;
    case WM_DESTROY:
        EnableWindow(g_main_ctx.hwnd, TRUE);
        KillTimer(hwnd, seeker.refresh_timer);
        g_main_ctx.core_ctx->vcr_stop_seek();
        break;
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    case WM_SEEK_COMPLETED:
        SetDlgItemText(hwnd, IDC_SEEKER_STATUS, L"Seek completed");
        SetDlgItemText(hwnd, IDC_SEEKER_START, L"Start");
        SetDlgItemText(hwnd, IDC_SEEKER_SUBTEXT, L"");
        KillTimer(hwnd, seeker.refresh_timer);
        break;
    case WM_TIMER: {
        if (!g_main_ctx.core_ctx->vcr_is_seeking())
        {
            break;
        }
        const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();

        const float effective_progress =
            remap(static_cast<float>(info.current_sample), static_cast<float>(info.seek_start_sample),
                  static_cast<float>(info.seek_target_sample), 0.0f, 1.0f);
        const auto str = std::format(L"Seeked {:.2f}%", effective_progress * 100.0);
        SetDlgItemText(hwnd, IDC_SEEKER_STATUS, str.c_str());
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_SEEKER_FRAME: {
            wchar_t str[260] = {0};
            GetDlgItemText(hwnd, IDC_SEEKER_FRAME, str, std::size(str));
            g_config.seeker_value = str;
        }
        break;
        case IDC_SEEKER_START: {
            if (g_main_ctx.core_ctx->vcr_is_seeking())
            {
                g_main_ctx.core_ctx->vcr_stop_seek();
                break;
            }

            SetDlgItemText(hwnd, IDC_SEEKER_START, L"Stop");
            if (g_config.core.seek_savestate_interval == 0)
            {
                SetDlgItemText(hwnd, IDC_SEEKER_SUBTEXT,
                               L"Seek savestates disabled. Seeking backwards will be slower.");
            }

            if (g_main_ctx.core_ctx->vcr_begin_seek(IOUtils::to_utf8_string(g_config.seeker_value), true) != Res_Ok)
            {
                SetDlgItemText(hwnd, IDC_SEEKER_START, L"Start");
                SetDlgItemText(hwnd, IDC_SEEKER_STATUS, L"Couldn't seek");
                SetDlgItemText(hwnd, IDC_SEEKER_SUBTEXT, L"");
                break;
            }

            seeker.refresh_timer = SetTimer(hwnd, NULL, 1000 / 10, nullptr);

            break;
        }
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
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
    Messenger::subscribe(Messenger::Message::SeekCompleted, [](std::any) {
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
