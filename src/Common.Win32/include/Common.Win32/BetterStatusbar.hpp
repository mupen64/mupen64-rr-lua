/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Win32/Common.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

/**
 * \brief An extended statusbar control.
 * \remark All functions must be called on the control's UI thread.
 */
namespace BetterStatusbar
{
/**
 * \brief Describes a statusbar part.
 */
struct Part
{
    std::wstring text;
    std::wstring tooltip;
    // Borrowed icon that must remain valid until replaced or cleared.
    HICON icon{};
    std::vector<std::wstring> sizing_labels;
    bool reserve_icon{};
    int padding = 16;
    std::function<void()> on_click;
};

namespace Internal
{
struct Entry
{
    Part part;
    int width{};
    bool initialized{};
};

struct Context
{
    HWND tooltip{};
    std::vector<std::unique_ptr<Entry>> entries;
    std::vector<int> edges;
    HFONT font{};
    bool sizing_dirty = true;
};

inline LRESULT CALLBACK subclass_proc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data)
{
    auto *ctx = reinterpret_cast<Context *>(data);
    if (message == WM_SIZE || message == WM_SETFONT || message == WM_THEMECHANGED ||
        message == WM_DPICHANGED_AFTERPARENT)
        ctx->sizing_dirty = true;
    if (message == WM_LBUTTONUP)
    {
        const POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        for (size_t i = 0; i < ctx->entries.size(); ++i)
        {
            RECT rect{};
            if (SendMessage(hwnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&rect)) && PtInRect(&rect, point))
            {
                const auto callback = ctx->entries[i]->part.on_click;
                const auto result = DefSubclassProc(hwnd, message, wparam, lparam);
                if (callback) callback();
                return result;
            }
        }
    }
    if (message == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hwnd, subclass_proc, id);
        if (IsWindow(ctx->tooltip)) DestroyWindow(ctx->tooltip);
        delete ctx;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

inline Context *get_ctx(HWND hwnd)
{
    DWORD_PTR data{};
    return GetWindowSubclass(hwnd, subclass_proc, 0, &data) ? reinterpret_cast<Context *>(data) : nullptr;
}
} // namespace Internal

/**
 * \brief Creates a statusbar at the bottom of its parent.
 * \param parent The parent window.
 * \param instance The application instance.
 * \param id The control identifier.
 * \return The control handle, or nullptr on failure.
 */
inline HWND create(HWND parent, HINSTANCE instance, int id)
{
    const auto hwnd = CreateWindowEx(0, STATUSCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | CCS_BOTTOM, 0, 0, 0, 0,
        parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
    if (!hwnd) return nullptr;
    auto ctx = std::make_unique<Internal::Context>();
    ctx->tooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, nullptr, instance, nullptr);
    if (!ctx->tooltip || !SetWindowSubclass(hwnd, Internal::subclass_proc, 0, reinterpret_cast<DWORD_PTR>(ctx.get())))
    {
        if (ctx->tooltip) DestroyWindow(ctx->tooltip);
        DestroyWindow(hwnd);
        return nullptr;
    }
    SendMessage(ctx->tooltip, TTM_SETMAXTIPWIDTH, 0, 420);
    ctx.release();
    return hwnd;
}

/**
 * \brief Clears borrowed icons before the caller releases or recreates them.
 * \param hwnd The control handle.
 */
inline void clear_icons(HWND hwnd)
{
    const auto ctx = Internal::get_ctx(hwnd);
    if (!ctx) return;
    for (size_t i = 0; i < ctx->entries.size(); ++i)
    {
        auto &part = ctx->entries[i]->part;
        if (!part.icon) continue;
        SendMessage(hwnd, SB_SETICON, i, 0);
        part.icon = nullptr;
    }
}

/**
 * \brief Replaces the displayed.
 * \param hwnd The control handle.
 * \param parts The parts in display order, with omitted parts hidden.
 * \return Whether the control exists and the part count is supported.
 */
inline bool set_parts(HWND hwnd, std::span<const Part> parts)
{
    const auto ctx = Internal::get_ctx(hwnd);
    if (!ctx || parts.size() > 255) return false;
    const auto font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
    const bool metrics_changed = ctx->sizing_dirty || font != ctx->font;
    while (ctx->entries.size() > parts.size())
    {
        TOOLINFOW tool{sizeof(tool)};
        tool.hwnd = hwnd;
        tool.uId = ctx->entries.size();
        SendMessageW(ctx->tooltip, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        ctx->entries.pop_back();
    }
    while (ctx->entries.size() < parts.size()) ctx->entries.push_back(std::make_unique<Internal::Entry>());

    std::vector<bool> text_changed(parts.size()), icon_changed(parts.size()), tip_changed(parts.size());
    std::vector<int> edges;
    HDC dc{};
    HGDIOBJ old_font{};
    int borders[3]{};
    SendMessage(hwnd, SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders));
    int edge = 0;
    bool visual_changed = false;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        auto &entry = *ctx->entries[i];
        const auto &next = parts[i];
        const auto &previous = entry.part;
        text_changed[i] = !entry.initialized || previous.text != next.text;
        icon_changed[i] = !entry.initialized || previous.icon != next.icon;
        tip_changed[i] = !entry.initialized || previous.tooltip != next.tooltip;
        if (metrics_changed || !entry.initialized || previous.sizing_labels != next.sizing_labels ||
            previous.reserve_icon != next.reserve_icon || previous.padding != next.padding ||
            bool(previous.icon) != bool(next.icon) || (next.sizing_labels.empty() && text_changed[i]))
        {
            if (!dc)
            {
                dc = GetDC(hwnd);
                old_font = SelectObject(dc, font);
            }
            int text_width = 0;
            const auto measure = [&](const std::wstring &text) {
                SIZE size{};
                GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
                text_width = std::max(text_width, static_cast<int>(size.cx));
            };
            if (next.sizing_labels.empty())
                measure(next.text);
            else
                for (const auto &label : next.sizing_labels) measure(label);
            const bool has_icon = next.reserve_icon || next.icon;
            entry.width = text_width + next.padding + 2 * borders[0] +
                          (has_icon ? GetSystemMetrics(SM_CXSMICON) + (text_width ? 4 : 0) : 0);
        }
        edges.push_back(edge += entry.width);
        visual_changed = visual_changed || text_changed[i] || icon_changed[i];
    }
    if (dc)
    {
        SelectObject(dc, old_font);
        ReleaseDC(hwnd, dc);
    }
    const bool layout_changed = edges != ctx->edges;
    const bool batch_redraw = (visual_changed || layout_changed) && IsWindowVisible(hwnd);
    if (batch_redraw) SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    if (layout_changed)
    {
        const int empty_edge = -1;
        SendMessage(hwnd, SB_SETPARTS, edges.empty() ? 1 : edges.size(),
            reinterpret_cast<LPARAM>(edges.empty() ? &empty_edge : edges.data()));
        if (edges.empty())
        {
            SendMessageW(hwnd, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L""));
            SendMessage(hwnd, SB_SETICON, 0, 0);
        }
        ctx->edges = std::move(edges);
    }
    for (size_t i = 0; i < parts.size(); ++i)
    {
        auto &entry = *ctx->entries[i];
        TOOLINFOW tool{sizeof(tool)};
        tool.hwnd = hwnd;
        tool.uId = i + 1;
        tool.uFlags = TTF_SUBCLASS;
        if (tip_changed[i]) entry.part.tooltip = parts[i].tooltip;
        tool.lpszText = entry.part.tooltip.data();
        if (!entry.initialized || layout_changed || metrics_changed)
        {
            SendMessage(hwnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&tool.rect));
            SendMessageW(
                ctx->tooltip, entry.initialized ? TTM_NEWTOOLRECTW : TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        }
        if (entry.initialized && tip_changed[i])
            SendMessageW(ctx->tooltip, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&tool));
        if (text_changed[i]) SendMessageW(hwnd, SB_SETTEXTW, i, reinterpret_cast<LPARAM>(parts[i].text.c_str()));
        if (icon_changed[i]) SendMessage(hwnd, SB_SETICON, i, reinterpret_cast<LPARAM>(parts[i].icon));
        entry.part.text = parts[i].text;
        entry.part.icon = parts[i].icon;
        entry.part.sizing_labels = parts[i].sizing_labels;
        entry.part.reserve_icon = parts[i].reserve_icon;
        entry.part.padding = parts[i].padding;
        entry.part.on_click = parts[i].on_click;
        entry.initialized = true;
    }
    if (batch_redraw)
    {
        SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
        if (layout_changed)
            InvalidateRect(hwnd, nullptr, FALSE);
        else
            for (size_t i = 0; i < parts.size(); ++i)
            {
                if (!text_changed[i] && !icon_changed[i]) continue;
                RECT rect{};
                SendMessage(hwnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&rect));
                InvalidateRect(hwnd, &rect, FALSE);
            }
    }
    ctx->font = font;
    ctx->sizing_dirty = false;
    return true;
}
} // namespace BetterStatusbar
