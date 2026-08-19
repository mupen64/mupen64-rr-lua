/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "Toasts.hpp"

namespace
{
using namespace Toasts;

constexpr int margin = 12;
constexpr int padding = 14;
constexpr int icon_size = 32;
constexpr int close_width = 24;
constexpr int gap = 8;
constexpr UINT close_button_id = 1;
constexpr UINT_PTR timer_id = 1;

struct ToastWindow
{
    std::string title;
    std::string content;
    HICON icon{};
    HWND icon_hwnd{};
    HWND title_hwnd{};
    HWND content_hwnd{};
    HWND close_hwnd{};
};

std::vector<HWND> toast_windows;
ATOM toast_class;

// TODO: Move this into some util file. There's really no appropriate place in our codebase for this as it stands
HICON icon_for_tone(const core_dialog_type tone)
{
    switch (tone)
    {
    case fsvc_error:
        return LoadIcon(nullptr, IDI_ERROR);
    case fsvc_information:
        return LoadIcon(nullptr, IDI_INFORMATION);
    case fsvc_warning:
    default:
        return LoadIcon(nullptr, IDI_WARNING);
    }
}

void relayout_impl()
{
    RECT client{};
    GetClientRect(g_main_ctx.hwnd, &client);

    const int client_width = static_cast<int>(client.right - client.left);
    const int max_width = std::max(1, client_width - 2 * margin);

    POINT origin{0, 0};
    ClientToScreen(g_main_ctx.hwnd, &origin);
    int y = origin.y + margin;

    for (const HWND hwnd : toast_windows)
    {
        if (!IsWindow(hwnd)) continue;

        RECT window_rect{};
        GetWindowRect(hwnd, &window_rect);

        const int width = std::min(static_cast<int>(window_rect.right - window_rect.left), max_width);
        const int height = static_cast<int>(window_rect.bottom - window_rect.top);
        const int x = origin.x + margin;

        SetWindowPos(hwnd, HWND_TOP, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        y += height + margin;
    }
}

void close_oldest_until_it_fits()
{
    RECT client{};
    GetClientRect(g_main_ctx.hwnd, &client);
    const int available_height = std::max(0, static_cast<int>(client.bottom - client.top) - 2 * margin);

    auto total_height = [] {
        int total = 0;
        for (const HWND hwnd : toast_windows)
        {
            RECT rect{};
            GetWindowRect(hwnd, &rect);
            total += rect.bottom - rect.top;
        }
        if (!toast_windows.empty()) total += static_cast<int>(toast_windows.size() - 1) * margin;
        return total;
    };

    while (toast_windows.size() > 1 && total_height() > available_height)
    {
        DestroyWindow(toast_windows.front());
    }
    relayout_impl();
}

LRESULT CALLBACK toast_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto *toast = reinterpret_cast<ToastWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_NCCREATE:
        toast = reinterpret_cast<ToastWindow *>(reinterpret_cast<CREATESTRUCT *>(lparam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(toast));
        return TRUE;

    case WM_CREATE: {
        const auto hinst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(g_main_ctx.hwnd, GWLP_HINSTANCE));
        toast->icon_hwnd = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_ICON, 0, 0, 0, 0, hwnd,
                                           nullptr, hinst, nullptr);
        SendMessage(toast->icon_hwnd, STM_SETICON, reinterpret_cast<WPARAM>(toast->icon), 0);

        toast->close_hwnd = CreateWindowEx(0, "BUTTON", "X", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0,
                                            hwnd, reinterpret_cast<HMENU>(close_button_id), hinst, nullptr);
        toast->content_hwnd = CreateWindowEx(0, "STATIC", toast->content.c_str(),
                                              WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 0, 0, hwnd,
                                              nullptr, hinst, nullptr);
        if (!toast->title.empty())
        {
            toast->title_hwnd = CreateWindowEx(0, "STATIC", toast->title.c_str(),
                                                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 0, 0, hwnd,
                                                nullptr, hinst, nullptr);
            SendMessage(toast->title_hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        }
        SendMessage(toast->content_hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        return 0;
    }

    case WM_SIZE: {
        if (!toast) break;
        const int width = LOWORD(lparam);
        const int height = HIWORD(lparam);
        const int text_x = padding + icon_size + gap;
        const int text_width = std::max(1, width - text_x - close_width - padding - gap);
        const int title_height = toast->title_hwnd ? 18 : 0;
        SetWindowPos(toast->icon_hwnd, nullptr, padding, padding, icon_size, icon_size,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(toast->close_hwnd, nullptr, width - close_width - padding, padding, close_width, 22,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        if (toast->title_hwnd)
            SetWindowPos(toast->title_hwnd, nullptr, text_x, padding, text_width, title_height,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(toast->content_hwnd, nullptr, text_x, padding + title_height, text_width,
                     std::max(1, height - 2 * padding - title_height), SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wparam) == close_button_id) DestroyWindow(hwnd);
        return 0;

    case WM_TIMER:
        if (wparam == timer_id) DestroyWindow(hwnd);
        return 0;

    case WM_NCDESTROY:
        KillTimer(hwnd, timer_id);
        std::erase(toast_windows, hwnd);
        delete toast;
        relayout_impl();
        return 0;
    }

    return DefWindowProc(hwnd, message, wparam, lparam);
}

void register_toast_class()
{
    if (toast_class) return;

    WNDCLASS klass{
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = toast_proc,
        .hInstance = g_main_ctx.hinst,
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        .lpszClassName = "Mupen64RRToast",
    };
    toast_class = RegisterClass(&klass);
}

void show_impl(const ToastData &data)
{
    register_toast_class();

    auto toast = std::make_unique<ToastWindow>();
    toast->title = data.title.value_or("");
    toast->content = data.content;
    toast->icon = icon_for_tone(data.tone);

    const HWND hwnd = CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_TOOLWINDOW, "Mupen64RRToast", "",
                                      WS_POPUP | WS_VISIBLE, 0, 0, 320, 80, g_main_ctx.hwnd, nullptr,
                                      g_main_ctx.hinst, toast.get());
    if (!hwnd) return;
    toast_windows.push_back(hwnd);
    toast.release();

    RECT client{};
    GetClientRect(g_main_ctx.hwnd, &client);
    const int max_width = std::max(1, static_cast<int>(client.right - client.left) - 2 * margin);
    HDC dc = GetDC(hwnd);
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    const auto *created = reinterpret_cast<ToastWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    SelectObject(dc, font);

    RECT content_measure{0, 0, 32767, 0};
    DrawText(dc, created->content.c_str(), -1, &content_measure,
              DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    SIZE title_size{};
    GetTextExtentPoint32(dc, created->title.c_str(), static_cast<int>(created->title.size()), &title_size);

    constexpr int text_left_padding = padding + icon_size + gap;
    constexpr int text_right_padding = close_width + padding + gap;
    const int desired_text_width = std::max(static_cast<int>(content_measure.right), static_cast<int>(title_size.cx));
    const int desired_width = desired_text_width + text_left_padding + text_right_padding;
    const int minimum_width = text_left_padding + text_right_padding;
    const int width = std::min(max_width, std::max(minimum_width, desired_width));
    const int text_width = std::max(1, width - text_left_padding - text_right_padding);
    RECT text_rect{0, 0, text_width, 0};
    DrawText(dc, created->content.c_str(), -1, &text_rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    ReleaseDC(hwnd, dc);

    const int title_height = created->title.empty() ? 0 : 18;
    const int height = std::max(icon_size + 2 * padding,
                                 title_height + static_cast<int>(text_rect.bottom) + 2 * padding);
    SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE);

    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(data.ttl).count();
    SetTimer(hwnd, timer_id, static_cast<UINT>(std::clamp<int64_t>(milliseconds, 1, UINT_MAX)), nullptr);
    close_oldest_until_it_fits();
}

} // namespace

namespace Toasts
{
void show(const ToastData &data)
{
    g_main_ctx.dispatcher->invoke([&] {
        show_impl(data);
    });
}

void relayout()
{
    relayout_impl();
}
} // namespace Toasts
