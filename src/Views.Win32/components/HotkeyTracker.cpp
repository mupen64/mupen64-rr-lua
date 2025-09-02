/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <components/HotkeyTracker.h>

const auto HOTKEY_TRACKER_CTX = L"Mupen64_HotkeyTrackerContext";

struct t_hotkey_tracker_context {
    bool last_lmb{};
    bool last_rmb{};
    bool last_mmb{};
    bool last_xmb1{};
    bool last_xmb2{};
};

static LRESULT CALLBACK action_menu_wnd_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
    auto ctx = static_cast<t_hotkey_tracker_context*>(GetProp(hwnd, HOTKEY_TRACKER_CTX));

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, action_menu_wnd_subclass_proc, sId);
        RemoveProp(hwnd, HOTKEY_TRACKER_CTX);
        delete ctx;
        ctx = nullptr;
        break;
    case WM_SETCURSOR:
        {
            if (ActionManager::get_hotkeys_locked())
            {
                break;
            }

            const bool mmb = GetAsyncKeyState(VK_MBUTTON) & 0x8000;
            const bool xmb1 = GetAsyncKeyState(VK_XBUTTON1) & 0x8000;
            const bool xmb2 = GetAsyncKeyState(VK_XBUTTON2) & 0x8000;
            bool hit = false;

            const auto hotkeys = g_config.hotkeys;
            for (const auto& [path, hotkey] : hotkeys)
            {
                const auto down = (mmb && !ctx->last_mmb && hotkey.key == VK_MBUTTON) || (xmb1 && !ctx->last_xmb1 && hotkey.key == VK_XBUTTON1) || (xmb2 && !ctx->last_xmb2 && hotkey.key == VK_XBUTTON2);
                const auto up = (!mmb && ctx->last_mmb && hotkey.key == VK_MBUTTON) || (!xmb1 && ctx->last_xmb1 && hotkey.key == VK_XBUTTON1) || (!xmb2 && ctx->last_xmb2 && hotkey.key == VK_XBUTTON2);

                if (down)
                {
                    ActionManager::invoke(path);
                    hit = true;
                }

                if (up)
                {
                    ActionManager::invoke(path, true);
                    hit = true;
                }
            }

            ctx->last_mmb = mmb;
            ctx->last_xmb1 = xmb1;
            ctx->last_xmb2 = xmb2;

            if (hit)
            {
                return 0;
            }

            break;
        }
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        {
            if (ActionManager::get_hotkeys_locked())
            {
                break;
            }

            const bool shift = GetKeyState(VK_SHIFT) & 0x8000;
            const bool ctrl = GetKeyState(VK_CONTROL) & 0x8000;
            const bool alt = GetKeyState(VK_MENU) & 0x8000;
            bool hit = false;

            const bool is_up = (msg == WM_KEYUP || msg == WM_SYSKEYUP);

            const auto hotkeys = g_config.hotkeys;
            for (const auto& [path, hotkey] : hotkeys)
            {
                if ((int)wParam == hotkey.key && shift == hotkey.shift && ctrl == hotkey.ctrl && alt == hotkey.alt)
                {
                    ActionManager::invoke(path, is_up);
                    hit = true;
                }
            }

            if (hit)
            {
                return 0;
            }

            break;
        }
    default:
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}


bool HotkeyTracker::attach(const HWND hwnd)
{
    auto context = std::make_unique<t_hotkey_tracker_context>();

    if (!SetProp(hwnd, HOTKEY_TRACKER_CTX, context.get()))
    {
        g_view_logger->error(L"HotkeyTracker::attach: Couldn't set context property");
        return false;
    }

    if (!SetWindowSubclass(hwnd, action_menu_wnd_subclass_proc, 0, (DWORD_PTR)context.get()))
    {
        g_view_logger->error(L"HotkeyTracker::attach: Couldn't set window subclass");
        RemoveProp(hwnd, HOTKEY_TRACKER_CTX);
        return false;
    }

    (void)context.release();

    return true;
}
