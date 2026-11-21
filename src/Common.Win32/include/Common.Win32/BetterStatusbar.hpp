/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Win32/Common.hpp>
#include <algorithm>
#include <cstdint>
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
    int padding = 8;
    int width{};
    std::function<void()> on_click;
};

namespace Internal
{
struct Entry
{
    Part part;
    int width{};
    bool initialized{};
    RECT rect{};
    HFONT fitted_font{};
    int fitted_width = -1;
    int fitted_height = -1;
};

struct FontCache
{
    HFONT base_font{};
    LOGFONTW description{};
    int height{};
    std::vector<HFONT> fonts;

    FontCache() = default;
    FontCache(const FontCache &) = delete;
    FontCache &operator=(const FontCache &) = delete;

    ~FontCache() { clear(); }

    void clear()
    {
        for (const auto font : fonts)
            if (font) DeleteObject(font);
        fonts.clear();
        base_font = nullptr;
    }

    bool initialize(HDC dc)
    {
        const auto font = reinterpret_cast<HFONT>(GetCurrentObject(dc, OBJ_FONT));
        if (base_font == font) return true;
        TEXTMETRICW metrics{};
        if (!GetObjectW(font, sizeof(description), &description) || !GetTextMetricsW(dc, &metrics)) return false;
        height = std::max(1L, metrics.tmHeight - metrics.tmInternalLeading);
        fonts.resize(height);
        base_font = font;
        return true;
    }

    HFONT get(int candidate_height)
    {
        auto &font = fonts[candidate_height];
        if (!font)
        {
            auto candidate = description;
            candidate.lfHeight = -candidate_height;
            candidate.lfWidth =
                description.lfWidth ? std::max(1, MulDiv(description.lfWidth, candidate_height, height)) : 0;
            font = CreateFontIndirectW(&candidate);
        }
        return font;
    }
};

struct Context
{
    HWND tooltip{};
    std::vector<std::unique_ptr<Entry>> entries;
    std::vector<int> edges;
    HFONT font{};
    FontCache fitted_fonts;
    int borders[3]{};
    bool sizing_dirty = true;
    bool rects_dirty = true;

    void reset_fitted_fonts()
    {
        for (auto &entry : entries)
        {
            entry->fitted_font = nullptr;
            entry->fitted_width = -1;
        }
        fitted_fonts.clear();
    }
};

inline void select_fitted_font(HDC dc, Context &ctx, Entry &entry, int width, int height)
{
    const auto original_font = reinterpret_cast<HFONT>(GetCurrentObject(dc, OBJ_FONT));
    if (ctx.fitted_fonts.base_font && ctx.fitted_fonts.base_font != original_font) ctx.reset_fitted_fonts();
    if (entry.fitted_width != width || entry.fitted_height != height)
    {
        entry.fitted_font = nullptr;
        entry.fitted_width = width;
        entry.fitted_height = height;

        SIZE size{};
        const auto &text = entry.part.text;
        GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
        if (size.cx > width || size.cy > height)
        {
            auto &fonts = ctx.fitted_fonts;
            if (fonts.initialize(dc))
            {
                int low = 1, high = fonts.height - 1;
                while (low <= high)
                {
                    const int candidate_height = low + (high - low) / 2;
                    const auto candidate = fonts.get(candidate_height);
                    if (!candidate) break;
                    SelectObject(dc, candidate);
                    GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
                    SelectObject(dc, original_font);
                    if ((size.cx <= width && size.cy <= height) || candidate_height == 1)
                    {
                        entry.fitted_font = candidate;
                        low = candidate_height + 1;
                    }
                    else
                    {
                        high = candidate_height - 1;
                    }
                }
            }
        }
    }
    if (entry.fitted_font) SelectObject(dc, entry.fitted_font);
}

inline LRESULT CALLBACK parent_subclass_proc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data)
{
    if (message == WM_DRAWITEM)
    {
        const auto *draw = reinterpret_cast<const DRAWITEMSTRUCT *>(lparam);
        auto *ctx = reinterpret_cast<Context *>(data);
        if (draw && draw->hwndItem == reinterpret_cast<HWND>(id) && draw->itemID < ctx->entries.size())
        {
            auto &entry = *ctx->entries[draw->itemID];
            const auto &part = entry.part;
            auto rect = draw->rcItem;
            if ((part.text.empty() && !part.icon) || !RectVisible(draw->hDC, &rect)) return TRUE;
            const int saved = SaveDC(draw->hDC);
            if (!saved) return TRUE;
            IntersectClipRect(draw->hDC, rect.left, rect.top, rect.right, rect.bottom);
            SelectObject(draw->hDC, reinterpret_cast<HFONT>(SendMessage(draw->hwndItem, WM_GETFONT, 0, 0)));
            SetBkMode(draw->hDC, TRANSPARENT);
            const int width = std::max(0L, rect.right - rect.left);
            const int height = std::max(0L, rect.bottom - rect.top);
            const bool has_icon = part.reserve_icon || part.icon;
            const int icon_size =
                has_icon ? std::min({GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), width, height}) : 0;
            const int inset =
                std::min(std::max(0, part.padding / 2), std::max(0, (width - std::max(1, icon_size)) / 2));
            if (part.icon && icon_size > 0)
            {
                const int x = part.text.empty() ? rect.left + (width - icon_size) / 2 : rect.left + inset;
                DrawIconEx(draw->hDC, x, rect.top + (height - icon_size) / 2, part.icon, icon_size, icon_size, 0,
                    nullptr, DI_NORMAL);
            }
            rect.left += inset + icon_size + (has_icon && !part.text.empty() ? 4 : 0);
            rect.right -= inset;
            if (!part.text.empty() && rect.right > rect.left && height > 0)
            {
                select_fitted_font(draw->hDC, *ctx, entry, rect.right - rect.left, height);
                DrawTextW(draw->hDC, part.text.c_str(), static_cast<int>(part.text.size()), &rect,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }
            RestoreDC(draw->hDC, saved);
            return TRUE;
        }
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

inline LRESULT CALLBACK subclass_proc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data)
{
    auto *ctx = reinterpret_cast<Context *>(data);
    if (message == WM_SIZE) ctx->rects_dirty = true;
    if (message == WM_SETFONT || message == WM_THEMECHANGED || message == WM_DPICHANGED_AFTERPARENT ||
        message == WM_SYSCOLORCHANGE || message == WM_SETTINGCHANGE)
    {
        ctx->sizing_dirty = true;
        ctx->rects_dirty = true;
        ctx->reset_fitted_fonts();
    }
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
        RemoveWindowSubclass(GetParent(hwnd), parent_subclass_proc, reinterpret_cast<UINT_PTR>(hwnd));
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
    const auto hwnd = CreateWindowEx(WS_EX_COMPOSITED, STATUSCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | CCS_BOTTOM, 0,
        0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
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
    if (!SetWindowSubclass(parent, Internal::parent_subclass_proc, reinterpret_cast<UINT_PTR>(hwnd),
            reinterpret_cast<DWORD_PTR>(ctx.get())))
    {
        ctx.release();
        DestroyWindow(hwnd);
        return nullptr;
    }
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
        part.icon = nullptr;
        RECT rect{};
        if (SendMessage(hwnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&rect))) InvalidateRect(hwnd, &rect, TRUE);
    }
}

/**
 * \brief Replaces the displayed parts.
 * \param hwnd The control handle.
 * \param parts The parts in display order, with omitted parts hidden.
 * \param scale_down Whether to proportionally shrink parts to fit the control.
 * \param scale_up Whether to proportionally expand parts to fill the control.
 * \return Whether the control exists and the part count is supported.
 */
inline bool set_parts(HWND hwnd, std::span<const Part> parts, bool scale_down = false, bool scale_up = false)
{
    const auto ctx = Internal::get_ctx(hwnd);
    if (!ctx || parts.size() > 255) return false;
    const auto font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
    const bool metrics_changed = ctx->sizing_dirty || font != ctx->font;
    if (font != ctx->font) ctx->reset_fitted_fonts();
    while (ctx->entries.size() > parts.size())
    {
        TOOLINFOW tool{sizeof(tool)};
        tool.hwnd = hwnd;
        tool.uId = ctx->entries.size();
        SendMessageW(ctx->tooltip, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        ctx->entries.pop_back();
    }
    while (ctx->entries.size() < parts.size()) ctx->entries.push_back(std::make_unique<Internal::Entry>());

    std::vector<bool> text_changed(parts.size()), visual_changed(parts.size()), tip_changed(parts.size());
    std::vector<int> edges;
    edges.reserve(parts.size());
    HDC dc{};
    HGDIOBJ old_font{};
    if (metrics_changed) SendMessage(hwnd, SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(ctx->borders));
    const auto &borders = ctx->borders;
    int edge = 0;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        auto &entry = *ctx->entries[i];
        const auto &next = parts[i];
        const auto &previous = entry.part;
        text_changed[i] = !entry.initialized || previous.text != next.text;
        visual_changed[i] = metrics_changed || text_changed[i] || previous.icon != next.icon ||
                            previous.reserve_icon != next.reserve_icon || previous.padding != next.padding;
        tip_changed[i] = !entry.initialized || previous.tooltip != next.tooltip;
        if (next.width > 0)
        {
            entry.width = next.width;
        }
        else if (metrics_changed || !entry.initialized || previous.width > 0 ||
                 previous.sizing_labels != next.sizing_labels || previous.reserve_icon != next.reserve_icon ||
                 previous.padding != next.padding || bool(previous.icon) != bool(next.icon) ||
                 (next.sizing_labels.empty() && text_changed[i]))
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
            // SB_GETRECT starts after the outer border or inter-part gap, not at the previous edge.
            const int leading_space = i == 0 ? borders[0] : borders[2];
            entry.width = leading_space + text_width + next.padding + 2 * borders[0] +
                          (has_icon ? GetSystemMetrics(SM_CXSMICON) + (text_width ? 4 : 0) : 0);
        }
        edges.push_back(edge += std::max(1, entry.width));
    }
    if (dc)
    {
        SelectObject(dc, old_font);
        ReleaseDC(hwnd, dc);
    }
    RECT client{};
    GetClientRect(hwnd, &client);
    const int available_width = std::max(0L, client.right - client.left);
    if (edge > 0 && ((scale_down && available_width < edge) || (scale_up && available_width > edge)))
    {
        for (auto &part_edge : edges)
            part_edge = static_cast<int>(static_cast<int64_t>(part_edge) * available_width / edge);
    }
    const bool layout_changed = edges != ctx->edges;
    const bool batch_redraw = layout_changed && IsWindowVisible(hwnd);
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
        if (!entry.initialized || layout_changed || metrics_changed || ctx->rects_dirty)
        {
            SendMessage(hwnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&entry.rect));
            tool.rect = entry.rect;
            SendMessageW(
                ctx->tooltip, entry.initialized ? TTM_NEWTOOLRECTW : TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        }
        if (entry.initialized && tip_changed[i])
            SendMessageW(ctx->tooltip, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&tool));

        if (text_changed[i] || metrics_changed) entry.fitted_width = -1;
        entry.part.text = parts[i].text;
        entry.part.icon = parts[i].icon;
        entry.part.sizing_labels = parts[i].sizing_labels;
        entry.part.reserve_icon = parts[i].reserve_icon;
        entry.part.padding = parts[i].padding;
        entry.part.width = parts[i].width;
        entry.part.on_click = parts[i].on_click;
        if (!entry.initialized) SendMessageW(hwnd, SB_SETTEXTW, i | SBT_OWNERDRAW, 0);
        entry.initialized = true;
    }
    if (batch_redraw) SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
    if (layout_changed)
        InvalidateRect(hwnd, nullptr, TRUE);
    else
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (visual_changed[i]) InvalidateRect(hwnd, &ctx->entries[i]->rect, TRUE);
        }
    ctx->font = font;
    ctx->sizing_dirty = false;
    ctx->rects_dirty = false;
    return true;
}
} // namespace BetterStatusbar
