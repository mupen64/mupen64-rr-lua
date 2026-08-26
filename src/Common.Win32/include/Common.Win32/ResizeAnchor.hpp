/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <ranges>

#include <Common.Win32/Common.hpp>

namespace ResizeAnchor
{
/**
 * \brief Flags used to specify a window's anchor behaviour.
 * \details The flags can be combined using the bitwise OR operator to anchor a window on multiple sides, thereby
 * causing the anchors to "pull" the window apart and growing it in the specified directions.
 */
enum class AnchorFlags : uint64_t
{
    /**
     * \brief Keep the window's X position relative to the parent window constant. This is a no-op on its own.
     */
    Left = 1 << 1,
    /**
     * \brief Maintain the right edge of the window at a constant distance from the right edge of the parent window.
     */
    Right = 1 << 2,
    /**
     * \brief Keep the window's Y position relative to the parent window constant. This is a no-op on its own.
     */
    Top = 1 << 3,
    /**
     * \brief Maintain the bottom edge of the window at a constant distance from the bottom edge of the parent window.
     */
    Bottom = 1 << 4,
    /**
     * \brief Invalidates the window when resized.
     */
    Invalidate = 1 << 5,
    /**
     * \brief Erases the window graphics when resized.
     */
    Erase = 1 << 6,
};

constexpr AnchorFlags operator|(AnchorFlags a, AnchorFlags b)
{
    return static_cast<AnchorFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

constexpr AnchorFlags operator&(AnchorFlags a, AnchorFlags b)
{
    return static_cast<AnchorFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

constexpr AnchorFlags FULL_ANCHOR = AnchorFlags::Left | AnchorFlags::Right | AnchorFlags::Top | AnchorFlags::Bottom;
constexpr AnchorFlags HORIZONTAL_ANCHOR = AnchorFlags::Left | AnchorFlags::Right;
constexpr AnchorFlags VERTICAL_ANCHOR = AnchorFlags::Top | AnchorFlags::Bottom;
constexpr AnchorFlags INVALIDATE_ERASE = AnchorFlags::Invalidate | AnchorFlags::Erase;

namespace detail
{
#define CTX_PROP "ResizeAnchor_ctx"

struct t_anchor_context
{
    HWND hwnd{};
    std::vector<std::pair<HWND, AnchorFlags>> anchors{};
    std::unordered_map<HWND, RECT> initial_rects{};
    bool first_resize{};
};

inline bool update_anchors(const HWND hwnd)
{
    if (!IsWindow(hwnd))
    {
        return false;
    }

    auto ctx = static_cast<t_anchor_context *>(GetProp(hwnd, CTX_PROP));

    if (!ctx)
    {
        return false;
    }

    RECT wnd_rc{};
    GetClientRect(hwnd, &wnd_rc);

    for (const auto &anchor : ctx->anchors)
    {
        const auto anchor_hwnd = anchor.first;
        const auto anchor_type = anchor.second;

        RECT ctl_rc{};
        auto update_ctl_rc = [&] {
            GetWindowRect(anchor_hwnd, &ctl_rc);
            MapWindowRect(HWND_DESKTOP, hwnd, &ctl_rc);
        };

        if (static_cast<bool>(anchor_type & AnchorFlags::Top) && static_cast<bool>(anchor_type & AnchorFlags::Bottom))
        {
            update_ctl_rc();
            const auto dist = ctx->initial_rects[hwnd].bottom - ctx->initial_rects[anchor_hwnd].bottom;
            SetWindowPos(anchor_hwnd, nullptr, 0, 0, ctl_rc.right - ctl_rc.left, (wnd_rc.bottom - dist) - ctl_rc.top,
                SWP_NOMOVE | SWP_NOZORDER);
        }

        if (!static_cast<bool>(anchor_type & AnchorFlags::Top) && static_cast<bool>(anchor_type & AnchorFlags::Bottom))
        {
            update_ctl_rc();
            const auto dist = ctx->initial_rects[hwnd].bottom - ctx->initial_rects[anchor_hwnd].top;
            SetWindowPos(anchor_hwnd, nullptr, ctl_rc.left, wnd_rc.bottom - dist, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }

        if (static_cast<bool>(anchor_type & AnchorFlags::Left) && static_cast<bool>(anchor_type & AnchorFlags::Right))
        {
            update_ctl_rc();
            const auto dist = ctx->initial_rects[hwnd].right - ctx->initial_rects[anchor_hwnd].right;
            SetWindowPos(anchor_hwnd, nullptr, 0, 0, (wnd_rc.right - dist) - ctl_rc.left, ctl_rc.bottom - ctl_rc.top,
                SWP_NOMOVE | SWP_NOZORDER);
        }

        if (!static_cast<bool>(anchor_type & AnchorFlags::Left) && static_cast<bool>(anchor_type & AnchorFlags::Right))
        {
            update_ctl_rc();
            const auto dist = ctx->initial_rects[hwnd].right - ctx->initial_rects[anchor_hwnd].left;
            SetWindowPos(anchor_hwnd, nullptr, wnd_rc.right - dist, ctl_rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }

        const bool invalidate = static_cast<bool>(anchor_type & AnchorFlags::Invalidate);
        const bool erase = static_cast<bool>(anchor_type & AnchorFlags::Erase);
        if (invalidate || erase)
        {
            UINT flags{};
            if (invalidate)
            {
                flags |= RDW_INVALIDATE;
            }
            if (erase)
            {
                flags |= RDW_ERASE;
            }
            RedrawWindow(anchor_hwnd, nullptr, nullptr, flags);
        }
    }

    return true;
}

inline LRESULT CALLBACK wnd_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
    auto ctx = static_cast<t_anchor_context *>(GetProp(hwnd, CTX_PROP));

    switch (msg)
    {
    case WM_SIZE:
        update_anchors(hwnd);
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, wnd_subclass_proc, sId);
        RemoveProp(hwnd, CTX_PROP);
        delete ctx;
        ctx = nullptr;
        break;
    default:
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline void add_anchors_base(
    t_anchor_context &ctx, const std::vector<std::pair<HWND, AnchorFlags>> &anchors, bool replace_child_anchors)
{
    std::erase_if(ctx.anchors, [&](const std::pair<HWND, AnchorFlags> &pair) {
        return std::ranges::find_if(anchors, [&](const std::pair<HWND, AnchorFlags> &new_pair) {
            return new_pair.first == pair.first;
        }) != anchors.end();
    });

    for (const auto &anchor_hwnd : anchors | std::views::keys)
    {
        if (!replace_child_anchors && ctx.initial_rects.contains(anchor_hwnd))
        {
            continue;
        }

        RECT rc{};
        GetWindowRect(anchor_hwnd, &rc);
        MapWindowRect(HWND_DESKTOP, ctx.hwnd, &rc);

        ctx.initial_rects[anchor_hwnd] = rc;
    }

    ctx.anchors.insert(ctx.anchors.end(), anchors.begin(), anchors.end());
}

inline bool add_anchors_impl(
    HWND hwnd, const std::vector<std::pair<HWND, AnchorFlags>> &anchors, bool replace_child_anchors)
{
    // Not implemented yet
    assert(replace_child_anchors);

    // We don't want to do all of this "initial state" business if we're already set up
    if (GetProp(hwnd, CTX_PROP) != nullptr)
    {
        auto ctx = static_cast<t_anchor_context *>(GetProp(hwnd, CTX_PROP));
        add_anchors_base(*ctx, anchors, replace_child_anchors);
        update_anchors(hwnd);
        return true;
    }

    auto ctx = new t_anchor_context();

    ctx->hwnd = hwnd;
    ctx->anchors = anchors;

    add_anchors_base(*ctx, anchors, replace_child_anchors);

    RECT wnd_rc{};
    GetClientRect(hwnd, &wnd_rc);
    ctx->initial_rects[hwnd] = wnd_rc;

    SetProp(hwnd, CTX_PROP, ctx);

    SetWindowSubclass(hwnd, wnd_subclass_proc, 0, 0);

    update_anchors(hwnd);

    return true;
}

inline bool remove_anchor_impl(HWND hwnd, HWND child_hwnd)
{
    auto ctx = static_cast<t_anchor_context *>(GetProp(hwnd, CTX_PROP));
    if (!ctx)
    {
        return false;
    }

    std::erase_if(ctx->anchors, [&](const std::pair<HWND, AnchorFlags> &pair) { return pair.first == child_hwnd; });

    update_anchors(hwnd);

    return true;
}

} // namespace detail

/**
 * \brief Adds anchors to a window.
 * \param hwnd The handle of the window to which anchors will be added.
 * \param anchors A vector of pairs where each pair contains a child window handle and its associated anchor flags.
 * \param replace_child_anchors If true, existing anchors for child windows will be replaced with the new ones. If
 * false, only new anchors will be added. \return Whether the operation succeeded. \details If the window already has
 * anchors, they will be updated based on the <c>replace_child_anchors</c> parameter. \remarks This function must be
 * called before any resizing of the parent window occurs. A good place to call this is during WM_INITDIALOG (dialog) or
 * WM_CREATE (window) processing. \remarks The window handles provided in `anchors` must be direct children of the
 * window provided via `hwnd`. \remarks The resizing hook will be removed when the window is destroyed.
 */
inline bool add_anchors(
    HWND hwnd, const std::vector<std::pair<HWND, AnchorFlags>> &anchors, bool replace_child_anchors = true)
{
    return detail::add_anchors_impl(hwnd, anchors, replace_child_anchors);
}

/**
 * \brief Removes an anchor from a child window.
 * \param hwnd The handle of the parent window from which the anchor will be removed.
 * \param child_hwnd The handle of the child window whose anchor will be removed.
 * \return Whether the operation succeeded.
 */
inline bool remove_anchor(HWND hwnd, HWND child_hwnd)
{
    return detail::remove_anchor_impl(hwnd, child_hwnd);
}

} // namespace ResizeAnchor
