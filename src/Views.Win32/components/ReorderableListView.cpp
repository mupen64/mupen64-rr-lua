/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "ReorderableListView.h"

namespace ReorderableListView
{
const auto CTX_KEY = L"Mupen64_ReorderableListViewContext";

struct Context
{
    Params params;
    HWND lv_hwnd;
    std::optional<size_t> drag_start_index;
    int last_mouse_over_index = -1;
};

static void draw_reorder_bar(const Context &ctx, int index)
{
    RECT rc;
    ListView_GetItemRect(ctx.lv_hwnd, index, &rc, LVIR_BOUNDS);

    HDC hdc = GetDC(ctx.lv_hwnd);

    SetROP2(hdc, R2_NOTXORPEN);

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HGDIOBJ old = SelectObject(hdc, pen);

    MoveToEx(hdc, rc.left, rc.top, NULL);
    LineTo(hdc, rc.right, rc.top);

    SelectObject(hdc, old);
    DeleteObject(pen);

    ReleaseDC(ctx.lv_hwnd, hdc);
}

LRESULT CALLBACK parent_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                      DWORD_PTR ref_data)
{
    auto ctx = static_cast<Context *>(GetProp(hwnd, CTX_KEY));

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, parent_subclass_proc, id);
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lparam)->code)
        {
        case LVN_BEGINDRAG: {
            const auto item = (NMLISTVIEW *)lparam;
            ctx->drag_start_index = item->iItem;
            SetCapture(ctx->lv_hwnd);
            break;
        }
        default:
            break;
        }
        break;
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK listview_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                        DWORD_PTR ref_data)
{
    auto ctx = static_cast<Context *>(GetProp(hwnd, CTX_KEY));

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveProp(hwnd, CTX_KEY);
        delete ctx;
        ctx = nullptr;
        RemoveWindowSubclass(hwnd, listview_subclass_proc, id);
        break;
    case WM_MOUSEMOVE: {
        if (!ctx->drag_start_index.has_value()) break;

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);

        LVHITTESTINFO hit = {0};
        hit.pt = pt;

        const auto target = ListView_HitTest(hwnd, &hit);

        // Because our hacky reorder bar drawing flickers, we minimize how often we redraw it.
        if (target == -1 || target == ctx->last_mouse_over_index) break;

        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        draw_reorder_bar(*ctx, target);

        ctx->last_mouse_over_index = target;
    }
    break;

    case WM_LBUTTONUP: {
        if (!ctx->drag_start_index.has_value()) break;

        ReleaseCapture();

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);

        LVHITTESTINFO hit = {0};
        hit.pt = pt;

        int target = ListView_HitTest(hwnd, &hit);

        if (target == -1) break;

        ctx->params.on_reorder(ctx->drag_start_index.value(), target);
        ctx->drag_start_index = std::nullopt;
        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    break;

    case LVN_BEGINDRAG: {
        const auto lv = (NMLISTVIEW *)lparam;
        ctx->drag_start_index = lv->iItem;
        SetCapture(hwnd);
        break;
    }
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

void make_reorderable(HWND hwnd, HWND parent_hwnd, const Params &params)
{
    auto context = new Context();
    context->lv_hwnd = hwnd;
    context->params = params;

    bool success = true;

    success = SetProp(parent_hwnd, CTX_KEY, context);
    RT_ASSERT(success, L"Failed to set context property on parent");

    success = SetProp(hwnd, CTX_KEY, context);
    RT_ASSERT(success, L"Failed to set context property on list view");

    success = SetWindowSubclass(parent_hwnd, parent_subclass_proc, 0, 0);
    RT_ASSERT(success, L"Failed to set parent window subclass");

    success = SetWindowSubclass(hwnd, listview_subclass_proc, 0, 0);
    RT_ASSERT(success, L"Failed to set list view subclass");
}
} // namespace ReorderableListView