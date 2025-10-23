/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Messenger.h>
#include <components/CoreDbg.h>

#define WM_DEBUGGER_CPU_STATE_UPDATED (WM_USER + 20)
#define WM_DEBUGGER_RESUMED_UPDATED (WM_USER + 21)

struct t_core_dbg_context
{
    HWND hwnd{};
    HWND list_hwnd{};
    core_dbg_cpu_state cpu{};
};
static t_core_dbg_context g_ctx{};

INT_PTR CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        g_ctx.hwnd = hwnd;
        g_ctx.list_hwnd = GetDlgItem(g_ctx.hwnd, IDC_COREDBG_LIST);

        CheckDlgButton(hwnd, IDC_COREDBG_RSP_TOGGLE, 1);
        return TRUE;
    case WM_DESTROY:
        g_ctx.hwnd = nullptr;
        return TRUE;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_COREDBG_CART_TILT:
            g_main_ctx.core_ctx->dbg_set_dma_read_enabled(!IsDlgButtonChecked(hwnd, IDC_COREDBG_CART_TILT));
            break;
        case IDC_COREDBG_RSP_TOGGLE:
            g_main_ctx.core_ctx->dbg_set_rsp_enabled(IsDlgButtonChecked(hwnd, IDC_COREDBG_RSP_TOGGLE));
            break;
        case IDC_COREDBG_STEP:
            g_main_ctx.core_ctx->dbg_step();
            break;
        case IDC_COREDBG_TOGGLEPAUSE:
            g_main_ctx.core_ctx->dbg_set_is_resumed(!g_main_ctx.core_ctx->dbg_get_resumed());
            break;
        default:
            break;
        }
        break;
    case WM_DEBUGGER_CPU_STATE_UPDATED: {
        char disasm[32] = {};
        g_main_ctx.core_ctx->dbg_disassemble(disasm, g_ctx.cpu.opcode, g_ctx.cpu.address);

        const auto str = std::format(L"{} ({:#08x}, {:#08x})", IOUtils::to_wide_string(disasm),
                                     g_ctx.cpu.opcode, g_ctx.cpu.address);
        ListBox_InsertString(g_ctx.list_hwnd, 0, str.c_str());

        if (ListBox_GetCount(g_ctx.list_hwnd) > 1024)
        {
            ListBox_DeleteString(g_ctx.list_hwnd, ListBox_GetCount(g_ctx.list_hwnd) - 1);
        }
        break;
    }
    case WM_DEBUGGER_RESUMED_UPDATED:
        Button_SetText(GetDlgItem(g_ctx.hwnd, IDC_COREDBG_TOGGLEPAUSE),
                       g_main_ctx.core_ctx->dbg_get_resumed() ? L"Pause" : L"Resume");
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void CoreDbg::show()
{
    CreateDialog(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_COREDBG), g_main_ctx.hwnd, dlgproc);
    ShowWindow(g_ctx.hwnd, SW_SHOW);
}

void CoreDbg::init()
{
    Messenger::subscribe(Messenger::Message::DebuggerCpuStateChanged, [](const std::any &data) {
        g_ctx.cpu = *std::any_cast<core_dbg_cpu_state *>(data);

        if (g_ctx.hwnd)
        {
            SendMessage(g_ctx.hwnd, WM_DEBUGGER_CPU_STATE_UPDATED, 0, 0);
        }
    });

    Messenger::subscribe(Messenger::Message::DebuggerResumedChanged, [](std::any) {
        if (g_ctx.hwnd)
        {
            SendMessage(g_ctx.hwnd, WM_DEBUGGER_RESUMED_UPDATED, 0, 0);
        }
    });
}
