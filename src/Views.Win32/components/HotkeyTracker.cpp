/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/ActionManager.hpp>
#include <action/AppActions.hpp>
#include <components/HotkeyTracker.hpp>
#include <components/ParameterPalette.hpp>
#include <HotkeyUtils.hpp>

const auto HOTKEY_TRACKER_CTX = "Mupen64_HotkeyTrackerContext";

struct HotkeyTrackerContext
{
    std::string window_context;
    bool last_mmb{};
    bool last_xmb1{};
    bool last_xmb2{};
};

static bool on_key(bool is_up, int32_t key, const std::string &window_context)
{
    const bool shift = GetKeyState(VK_SHIFT) & 0x8000;
    const bool ctrl = GetKeyState(VK_CONTROL) & 0x8000;
    const bool alt = GetKeyState(VK_MENU) & 0x8000;
    bool hit = false;

    const auto trigger = HotkeyUtils::vk_to_trigger(key);
    if (!trigger) return false;

    const auto hotkeys = g_config.hotkeys;
    for (const auto &[path, hotkey] : hotkeys)
    {
        if (!hotkey.matches_context(window_context)) continue;
        if (*trigger == hotkey.trigger && shift == hotkey.shift && ctrl == hotkey.ctrl && alt == hotkey.alt)
        {
            if (!ActionManager::get_enabled(path)) continue;
            if (ActionManager::is_hotkey_locked(hotkey)) continue;

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

static LRESULT CALLBACK action_menu_wnd_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
    auto ctx = static_cast<HotkeyTrackerContext *>(GetProp(hwnd, HOTKEY_TRACKER_CTX));

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, action_menu_wnd_subclass_proc, sId);
        RemoveProp(hwnd, HOTKEY_TRACKER_CTX);
        delete ctx;
        ctx = nullptr;
        break;
    case WM_SETCURSOR: {
        const bool mmb = GetAsyncKeyState(VK_MBUTTON) & 0x8000;
        const bool xmb1 = GetAsyncKeyState(VK_XBUTTON1) & 0x8000;
        const bool xmb2 = GetAsyncKeyState(VK_XBUTTON2) & 0x8000;
        const auto window_context = ctx->window_context;
        const auto last_mmb = ctx->last_mmb;
        const auto last_xmb1 = ctx->last_xmb1;
        const auto last_xmb2 = ctx->last_xmb2;
        ctx->last_mmb = mmb;
        ctx->last_xmb1 = xmb1;
        ctx->last_xmb2 = xmb2;

        if (GetAncestor(hwnd, GA_ROOT) != GetForegroundWindow()) break;

        bool hit = false;
        if (mmb != last_mmb) hit |= on_key(!mmb, VK_MBUTTON, window_context);
        if (xmb1 != last_xmb1) hit |= on_key(!xmb1, VK_XBUTTON1, window_context);
        if (xmb2 != last_xmb2) hit |= on_key(!xmb2, VK_XBUTTON2, window_context);
        if (hit) return 0;

        break;
    }
    default:
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool HotkeyTracker::process_message(const MSG &msg)
{
    if (msg.message != WM_KEYDOWN && msg.message != WM_KEYUP && msg.message != WM_SYSKEYDOWN &&
        msg.message != WM_SYSKEYUP)
        return false;

    const auto hwnd = GetAncestor(msg.hwnd, GA_ROOT);
    const auto ctx = static_cast<HotkeyTrackerContext *>(GetProp(hwnd, HOTKEY_TRACKER_CTX));
    if (!ctx) return false;

    const auto window_context = ctx->window_context;
    return on_key(
        msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP, static_cast<int32_t>(msg.wParam), window_context);
}

bool HotkeyTracker::attach(const HWND hwnd, const std::string &window_context)
{
    auto context = std::make_unique<HotkeyTrackerContext>();
    context->window_context = window_context;

    if (!SetProp(hwnd, HOTKEY_TRACKER_CTX, context.get()))
    {
        g_view_logger->error("HotkeyTracker::attach: Couldn't set context property");
        return false;
    }

    if (!SetWindowSubclass(hwnd, action_menu_wnd_subclass_proc, 0, (DWORD_PTR)context.get()))
    {
        g_view_logger->error("HotkeyTracker::attach: Couldn't set window subclass");
        RemoveProp(hwnd, HOTKEY_TRACKER_CTX);
        return false;
    }

    (void)context.release();

    return true;
}
