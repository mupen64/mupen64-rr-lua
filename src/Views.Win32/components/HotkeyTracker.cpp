/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <components/AppActions.h>
#include <components/HotkeyTracker.h>
#include <components/ParameterPalette.h>

const auto HOTKEY_TRACKER_CTX = L"Mupen64_HotkeyTrackerContext";

struct t_hotkey_tracker_context
{
    bool last_lmb{};
    bool last_rmb{};
    bool last_mmb{};
    bool last_xmb1{};
    bool last_xmb2{};
};

static std::optional<bool> on_key(bool is_up, int32_t key)
{
    if (ActionManager::get_hotkeys_locked())
    {
        return std::nullopt;
    }

    const bool shift = GetKeyState(VK_SHIFT) & 0x8000;
    const bool ctrl = GetKeyState(VK_CONTROL) & 0x8000;
    const bool alt = GetKeyState(VK_MENU) & 0x8000;
    bool hit = false;

    const auto hotkeys = g_config.hotkeys;
    for (const auto &[path, hotkey] : hotkeys)
    {
        if ((int)key == hotkey.key && shift == hotkey.shift && ctrl == hotkey.ctrl && alt == hotkey.alt)
        {
            if (ActionManager::get_enabled(path) == false) continue;

            // HACK: Fast Forward is a special case: we don't want it to be constantly toggled on and off because it
            // messes up flow
            const bool release_on_repress = path != ActionManager::normalize_filter(AppActions::FAST_FORWARD);

            const auto params = ActionManager::get_params(path);

            // Has params: hand off to ParameterPalette.
            if (!params.empty())
                ParameterPalette::show(path);
            else
                ActionManager::invoke(path, is_up, release_on_repress);

            hit = true;
        }
    }

    return hit;
}

static LRESULT CALLBACK action_menu_wnd_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId,
                                                      DWORD_PTR dwRefData)
{
    auto ctx = static_cast<t_hotkey_tracker_context *>(GetProp(hwnd, HOTKEY_TRACKER_CTX));

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, action_menu_wnd_subclass_proc, sId);
        RemoveProp(hwnd, HOTKEY_TRACKER_CTX);
        delete ctx;
        ctx = nullptr;
        break;
    case WM_SETCURSOR: {
        if (ActionManager::get_hotkeys_locked())
        {
            break;
        }

        const bool mmb = GetAsyncKeyState(VK_MBUTTON) & 0x8000;
        const bool xmb1 = GetAsyncKeyState(VK_XBUTTON1) & 0x8000;
        const bool xmb2 = GetAsyncKeyState(VK_XBUTTON2) & 0x8000;
        bool hit = false;

        const auto hotkeys = g_config.hotkeys;
        for (const auto &[path, hotkey] : hotkeys)
        {
            const auto down = (mmb && !ctx->last_mmb && hotkey.key == VK_MBUTTON) ||
                              (xmb1 && !ctx->last_xmb1 && hotkey.key == VK_XBUTTON1) ||
                              (xmb2 && !ctx->last_xmb2 && hotkey.key == VK_XBUTTON2);
            const auto up = (!mmb && ctx->last_mmb && hotkey.key == VK_MBUTTON) ||
                            (!xmb1 && ctx->last_xmb1 && hotkey.key == VK_XBUTTON1) ||
                            (!xmb2 && ctx->last_xmb2 && hotkey.key == VK_XBUTTON2);

            hit = down || up;

            if (down)
            {
                const auto params = ActionManager::get_params(path);

                // Has params: hand off to ParameterPalette.
                if (!params.empty())
                    ParameterPalette::show(path);
                else
                    ActionManager::invoke(path);
            }

            if (up)
            {
                ActionManager::invoke(path, true, true);
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
    case WM_SYSKEYUP: {
        const auto result = on_key(msg == WM_KEYUP || msg == WM_SYSKEYUP, (int)wParam);
        if (result.has_value() && result.value()) return 0;
        break;
    }
    case WM_NOTIFY: {
        auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);
        if (nmhdr && nmhdr->hwndFrom && IsWindow(nmhdr->hwndFrom) &&
            SendMessage(nmhdr->hwndFrom, WM_GETDLGCODE, 0, 0) != 0)
        {
            wchar_t class_name[32];
            GetClassName(nmhdr->hwndFrom, class_name, std::size(class_name));

            if (lstrcmpiW(class_name, WC_LISTVIEWW) == 0 && nmhdr->code == LVN_KEYDOWN)
            {
                auto key = reinterpret_cast<LPNMLVKEYDOWN>(lParam)->wVKey;

                if (on_key(false, key))
                {
                    return TRUE;
                }
            }
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
