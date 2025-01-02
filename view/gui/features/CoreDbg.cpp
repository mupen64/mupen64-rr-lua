#include <view/lua/LuaConsole.h>

#include <view/gui/Main.h>
#include <view/resource.h>
#include "CoreDbg.h"

#include <Windows.h>
#include <Windowsx.h>
#include <stdio.h>
#include <thread>

#include <core/r4300/disasm.h>
#include <core/r4300/r4300.h>

#include "core/r4300/debugger.h"
#include "shared/Messenger.h"

#define WM_DEBUGGER_CPU_STATE_UPDATED (WM_USER + 20)
#define WM_DEBUGGER_RESUMED_UPDATED (WM_USER + 21)

namespace CoreDbg
{
    std::atomic<HWND> g_hwnd = nullptr;
    Debugger::t_cpu_state g_cpu_state{};

    INT_PTR CALLBACK coredbg_dlgproc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            g_hwnd = hwnd;
            CheckDlgButton(hwnd, IDC_COREDBG_RSP_TOGGLE, 1);
            return TRUE;
        case WM_DESTROY:
            g_hwnd = nullptr;
            EndDialog(hwnd, LOWORD(w_param));
            return TRUE;
        case WM_CLOSE:
            EndDialog(hwnd, IDOK);
            break;
        case WM_COMMAND:
            switch (LOWORD(w_param))
            {
            case IDC_COREDBG_CART_TILT:
                Debugger::set_dma_read_enabled(!IsDlgButtonChecked(hwnd, IDC_COREDBG_CART_TILT));
                break;
            case IDC_COREDBG_RSP_TOGGLE:
                Debugger::set_rsp_enabled(IsDlgButtonChecked(hwnd, IDC_COREDBG_RSP_TOGGLE));
                break;
            case IDC_COREDBG_STEP:
                Debugger::step();
                break;
            case IDC_COREDBG_TOGGLEPAUSE:
                Debugger::set_is_resumed(!Debugger::get_resumed());
                break;
            default:
                break;
            }
            break;
        case WM_DEBUGGER_CPU_STATE_UPDATED:
            {
                HWND list_hwnd = GetDlgItem(g_hwnd, IDC_COREDBG_LIST);

                char disasm[255] = {0};
                DisassembleInstruction(disasm, g_cpu_state.opcode, g_cpu_state.address);

                auto str = std::format("{} ({:#08x}, {:#08x})", disasm, g_cpu_state.opcode, g_cpu_state.address);
                ListBox_InsertString(list_hwnd, 0, str.c_str());

                if (ListBox_GetCount(list_hwnd) > 1024)
                {
                    ListBox_DeleteString(list_hwnd, ListBox_GetCount(list_hwnd) - 1);
                }
                break;
            }
        case WM_DEBUGGER_RESUMED_UPDATED:
            Button_SetText(GetDlgItem(g_hwnd, IDC_COREDBG_TOGGLEPAUSE), Debugger::get_resumed() ? L"Pause" : L"Resume");
            break;
        default:
            return FALSE;
        }
        return TRUE;
    }

    void show()
    {
        std::thread([] { DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_COREDBG), 0, coredbg_dlgproc); }).detach();
    }

    void init()
    {
        Messenger::subscribe(Messenger::Message::DebuggerCpuStateChanged,
                             [](std::any data)
                             {
                                 g_cpu_state = std::any_cast<Debugger::t_cpu_state>(data);

                                 if (g_hwnd)
                                 {
                                     SendMessage(g_hwnd, WM_DEBUGGER_CPU_STATE_UPDATED, 0, 0);
                                 }
                             });

        Messenger::subscribe(Messenger::Message::DebuggerResumedChanged,
                             [](std::any data)
                             {
                                 if (g_hwnd)
                                 {
                                     SendMessage(g_hwnd, WM_DEBUGGER_RESUMED_UPDATED, 0, 0);
                                 }
                             });
    }
} // namespace CoreDbg
