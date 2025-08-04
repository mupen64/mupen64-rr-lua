/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "ResizeAnchor.h"

#include "DialogService.h"

#define CTX_PROP L"ResizeAnchor_ctx"

using namespace ResizeAnchor;

struct t_anchor_context {
    HWND hwnd{};
    std::vector<std::pair<HWND, AnchorFlags>> anchors{};
    std::unordered_map<HWND, RECT> initial_rects{};
    bool first_resize{};
};

static LRESULT CALLBACK wnd_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
    auto ctx = static_cast<t_anchor_context*>(GetProp(hwnd, CTX_PROP));

    switch (msg)
    {
    case WM_SIZE:
        {
            RECT wnd_rc{};
            GetClientRect(hwnd, &wnd_rc);

            for (const auto& anchor : ctx->anchors)
            {
                const auto anchor_hwnd = anchor.first;
                const auto anchor_type = anchor.second;

                RECT ctl_rc{};
                auto update_ctl_rc = [&]() {
                    GetWindowRect(anchor_hwnd, &ctl_rc);
                    MapWindowRect(HWND_DESKTOP, hwnd, &ctl_rc);
                };

                if (static_cast<bool>(anchor_type & AnchorFlags::Top) && static_cast<bool>(anchor_type & AnchorFlags::Bottom))
                {
                    update_ctl_rc();
                    const auto dist = ctx->initial_rects[hwnd].bottom - ctx->initial_rects[anchor_hwnd].bottom;
                    SetWindowPos(anchor_hwnd, nullptr, 0, 0, ctl_rc.right - ctl_rc.left, (wnd_rc.bottom - dist) - ctl_rc.top, SWP_NOMOVE | SWP_NOZORDER);
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
                    SetWindowPos(anchor_hwnd, nullptr, 0, 0, (wnd_rc.right - dist) - ctl_rc.left, ctl_rc.bottom - ctl_rc.top, SWP_NOMOVE | SWP_NOZORDER);
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
            break;
        }
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

static void add_anchors(t_anchor_context& ctx, const std::vector<std::pair<HWND, AnchorFlags>>& anchors, bool replace_child_anchors)
{
    std::erase_if(ctx.anchors, [&](const std::pair<HWND, AnchorFlags>& pair) {
        return std::ranges::find_if(anchors, [&](const std::pair<HWND, AnchorFlags>& new_pair) {
                   return new_pair.first == pair.first;
               }) != anchors.end();
    });

    for (const auto& anchor_hwnd : anchors | std::views::keys)
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

bool ResizeAnchor::add_anchors(HWND hwnd, const std::vector<std::pair<HWND, AnchorFlags>>& anchors, bool replace_child_anchors)
{
    // Not implemented yet
    assert(replace_child_anchors);

    // We don't want to do all of this "initial state" business if we're already set up
    if (GetProp(hwnd, CTX_PROP) != nullptr)
    {
        auto ctx = static_cast<t_anchor_context*>(GetProp(hwnd, CTX_PROP));
        add_anchors(*ctx, anchors, replace_child_anchors);
        return true;
    }

    auto ctx = new t_anchor_context();

    ctx->hwnd = hwnd;
    ctx->anchors = anchors;

    add_anchors(*ctx, anchors, replace_child_anchors);

    RECT wnd_rc{};
    GetClientRect(hwnd, &wnd_rc);
    ctx->initial_rects[hwnd] = wnd_rc;

    SetProp(hwnd, CTX_PROP, ctx);

    SetWindowSubclass(hwnd, wnd_subclass_proc, 0, 0);

    return true;
}

bool ResizeAnchor::remove_anchor(HWND hwnd, HWND child_hwnd)
{
    auto ctx = static_cast<t_anchor_context*>(GetProp(hwnd, CTX_PROP));
    if (!ctx)
    {
        return false;
    }

    std::erase_if(ctx->anchors, [&](const std::pair<HWND, AnchorFlags>& pair) {
        return pair.first == child_hwnd;
    });

    return true;
}
