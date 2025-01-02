#include "Seeker.h"

#include <Windows.h>
#include <thread>

#include <core/r4300/r4300.h>
#include <core/r4300/vcr.h>
#include <shared/Config.hpp>
#include <shared/Messenger.h>
#include <view/resource.h>
#include "../Main.h"

#define WM_SEEK_COMPLETED (WM_USER + 11)

namespace Seeker
{
    HWND current_hwnd;
    UINT_PTR refresh_timer;

    LRESULT CALLBACK SeekerProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        switch (Message)
        {
        case WM_INITDIALOG:
            current_hwnd = hwnd;

            SetDlgItemText(hwnd, IDC_SEEKER_STATUS, L"Idle");
            SetDlgItemText(hwnd, IDC_SEEKER_START, L"Start");
            SetDlgItemText(hwnd, IDC_SEEKER_FRAME, g_config.seeker_value.c_str());

            refresh_timer = SetTimer(hwnd, NULL, 1000 / 10, nullptr);
            SetFocus(GetDlgItem(hwnd, IDC_SEEKER_FRAME));
            break;
        case WM_DESTROY:
            KillTimer(hwnd, refresh_timer);
            VCR::stop_seek();
            break;
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;
        case WM_SEEK_COMPLETED:
            SetDlgItemText(hwnd, IDC_SEEKER_STATUS, L"Seek completed");
            SetDlgItemText(hwnd, IDC_SEEKER_START, L"Start");
            SetDlgItemText(hwnd, IDC_SEEKER_SUBTEXT, L"");
            break;
        case WM_TIMER:
            {
                if (!VCR::is_seeking())
                {
                    break;
                }
                auto [current, total] = VCR::get_seek_completion();
                const auto str = std::format(L"Seeked {:.2f}%", static_cast<float>(current) / static_cast<float>(total) * 100.0);
                SetDlgItemText(hwnd, IDC_SEEKER_STATUS, str.c_str());
                break;
            }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDC_SEEKER_FRAME:
                {
                    wchar_t str[260] = {0};
                    GetDlgItemText(hwnd, IDC_SEEKER_FRAME, str, std::size(str));
                    g_config.seeker_value = str;
                }
                break;
            case IDC_SEEKER_START:
                {
                    if (VCR::is_seeking())
                    {
                        VCR::stop_seek();
                        break;
                    }

                    SetDlgItemText(hwnd, IDC_SEEKER_START, L"Stop");
                    if (g_config.seek_savestate_interval == 0)
                    {
                        SetDlgItemText(hwnd, IDC_SEEKER_SUBTEXT, L"Seek savestates disabled. Seeking backwards will be slower.");
                    }

                    if (VCR::begin_seek(g_config.seeker_value, true) != CoreResult::Ok)
                    {
                        SetDlgItemText(hwnd, IDC_SEEKER_START, L"Start");
                        SetDlgItemText(hwnd, IDC_SEEKER_STATUS, L"Couldn't seek");
                        SetDlgItemText(hwnd, IDC_SEEKER_SUBTEXT, L"");
                        break;
                    }

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

    void show()
    {
        // Let's not have 2 instances of a singleton...
        if (current_hwnd)
        {
            return;
        }

        // We need to run the dialog on another thread, since we don't want to block
        // the main one
        std::thread(
        []
        {
            DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_SEEKER), g_main_hwnd, (DLGPROC)SeekerProc);
            current_hwnd = nullptr;
        })
        .detach();
    }

    void init()
    {
        Messenger::subscribe(Messenger::Message::SeekCompleted,
                             [](std::any)
                             {
                                 if (!current_hwnd)
                                     return;
                                 SendMessage(current_hwnd, WM_SEEK_COMPLETED, 0, 0);
                             });
    }
} // namespace Seeker
