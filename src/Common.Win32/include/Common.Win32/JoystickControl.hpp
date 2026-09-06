/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <filesystem>
#include <numbers>
#include <Common/MiscHelpers.hpp>
#if defined(_WIN32)
#include <Common.Win32/Common.hpp>
#elif defined(__linux__)
#error JoystickControl is not supported on Linux
#endif

/**
 * \brief A joystick control for Win32.
 */
namespace JoystickControl
{
static constexpr UINT wm_joystick_position_changed = WM_APP + 200;
static constexpr UINT wm_joystick_drag_begin = WM_APP + 201;

namespace Internal
{
struct Context
{
    enum class Mode
    {
        None,
        Absolute,
        Sticky,
        Relative
    };

    int x{};
    int y{};
    int painted_x{};
    int painted_y{};
    int cursor_diff_x{};
    int cursor_diff_y{};
    Mode mode = Mode::None;
    HDC front_dc{};
    HDC back_dc{};
    HBITMAP back_bmp{};
    Gdiplus::Graphics *g{};
    Gdiplus::Color clear_color{};
    Gdiplus::SolidBrush *bg_brush{};
    Gdiplus::SolidBrush *tip_brush{};
    Gdiplus::Pen *outline_pen{};
    Gdiplus::Pen *border_pen{};
    Gdiplus::Pen *line_pen{};
};

using Mode = Context::Mode;

inline void get_cursor_to_joystick_position(const HWND hwnd, int &x, int &y)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    POINT pt{};
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    const int width = rc.right;
    const int height = rc.bottom;

    x = (pt.x - width / 2) * 128 / (width / 2);
    y = (pt.y - height / 2) * 128 / (height / 2);

    y = -y;

    x = std::clamp(x, -128, 127);
    y = std::clamp(y, -128, 127);
}

inline void update_joystick_position(HWND hwnd, Context *ctx)
{
    if (ctx->mode == Mode::None)
    {
        return;
    }

    get_cursor_to_joystick_position(hwnd, ctx->x, ctx->y);

    if (ctx->mode == Mode::Relative)
    {
        ctx->x -= ctx->cursor_diff_x;
        ctx->y -= ctx->cursor_diff_y;
    }

    RECT rc{};
    GetClientRect(hwnd, &rc);

    if (std::abs(ctx->x) < 8) ctx->x = 0;
    if (std::abs(ctx->y) < 8) ctx->y = 0;

    ctx->x = std::clamp(ctx->x, -128, 127);
    ctx->y = std::clamp(ctx->y, -128, 127);

    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
    SendMessage(GetParent(hwnd), JoystickControl::wm_joystick_position_changed, 0, 0);
}

inline void destroy_dcs(const HWND hwnd, Context *ctx)
{
    if (!ctx->front_dc)
    {
        return;
    }

    SelectObject(ctx->back_dc, nullptr);
    DeleteObject(ctx->back_bmp);
    DeleteDC(ctx->back_dc);

    ReleaseDC(hwnd, ctx->front_dc);
    ctx->front_dc = nullptr;

    delete ctx->g;
    delete ctx->bg_brush;
    delete ctx->tip_brush;
    delete ctx->outline_pen;
    delete ctx->border_pen;
    delete ctx->line_pen;
}

inline void update_clear_color(const HWND hwnd, Context *ctx)
{
    const auto parent_hwnd = GetParent(hwnd);
    const auto parent_dc = GetDC(parent_hwnd);
    const auto clear_brush =
        reinterpret_cast<HBRUSH>(SendMessage(parent_hwnd, WM_CTLCOLORDLG, reinterpret_cast<WPARAM>(parent_dc), 0));

    if (clear_brush)
    {
        LOGBRUSH log_brush{};
        if (GetObject(clear_brush, sizeof(log_brush), &log_brush) == sizeof(log_brush))
        {
            ctx->clear_color.SetFromCOLORREF(log_brush.lbColor);
        }
    }
    else
    {
        ctx->clear_color.SetFromCOLORREF(GetSysColor(COLOR_WINDOW));
    }

    ReleaseDC(parent_hwnd, parent_dc);
}

inline void create_dcs(const HWND hwnd, Context *ctx)
{
    if (ctx->front_dc)
    {
        return;
    }

    RECT rc{};
    GetClientRect(hwnd, &rc);

    ctx->front_dc = GetDC(hwnd);
    ctx->back_dc = CreateCompatibleDC(ctx->front_dc);
    ctx->back_bmp = CreateCompatibleBitmap(ctx->front_dc, rc.right, rc.bottom);
    SelectObject(ctx->back_dc, ctx->back_bmp);

    ctx->g = new Gdiplus::Graphics(ctx->back_dc);
    ctx->g->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

    const auto scale = (float)GetDpiForWindow(hwnd) / 96.0f;

    const auto corner_diameter = static_cast<int>(12.0f * scale);
    SetWindowRgn(hwnd, CreateRoundRectRgn(0, 0, rc.right, rc.bottom, corner_diameter, corner_diameter), TRUE);

    ctx->bg_brush = new Gdiplus::SolidBrush(Gdiplus::Color::White);
    ctx->tip_brush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 0, 0));
    ctx->outline_pen = new Gdiplus::Pen(Gdiplus::Color(255, 0, 0, 0), 1.0f * scale);
    ctx->border_pen = new Gdiplus::Pen(Gdiplus::Color(255, 0, 0, 0), 1.0f * scale);
    ctx->line_pen = new Gdiplus::Pen(Gdiplus::Color(255, 0, 0, 255), 3.0f * scale);
    ctx->line_pen->SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);

    update_clear_color(hwnd, ctx);
}

inline auto get_ctx(const HWND hwnd)
{
    return reinterpret_cast<Context *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

inline LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    const auto ctx = get_ctx(hwnd);

    switch (msg)
    {
    case WM_NCCREATE:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(new Context()));
        create_dcs(hwnd, get_ctx(hwnd));
        break;
    case WM_NCDESTROY:
        destroy_dcs(hwnd, ctx);
        delete ctx;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        break;
    case WM_SIZE:
        destroy_dcs(hwnd, ctx);
        create_dcs(hwnd, ctx);
        break;
    case WM_DPICHANGED:
        destroy_dcs(hwnd, ctx);
        create_dcs(hwnd, ctx);
        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
        break;
    case WM_MOUSEWHEEL: {
        const auto delta = GET_WHEEL_DELTA_WPARAM(wparam);
        RECT rc{};
        GetClientRect(hwnd, &rc);

        const auto increment_sign = delta > 0.0 ? 1.0 : -1.0;
        const auto increment_x = 1.0 * increment_sign;
        const auto increment_y = -1.0 * increment_sign;

        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            ctx->y += increment_y;
        }
        else if (GetKeyState(VK_SHIFT) & 0x8000)
        {
            const double angle = increment_sign * 5.0 * std::numbers::pi / 180.0;
            const double cos_angle = std::cos(angle);
            const double sin_angle = std::sin(angle);

            const double old_x = ctx->x;
            const double old_y = ctx->y;

            ctx->x = old_x * cos_angle - old_y * sin_angle;
            ctx->y = old_x * sin_angle + old_y * cos_angle;
        }
        else
        {
            ctx->x += increment_x;
        }

        ctx->x = MiscHelpers::wrapping_clamp(ctx->x, -127, 128);
        ctx->y = MiscHelpers::wrapping_clamp(ctx->y, -127, 128);

        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
        SendMessage(GetParent(hwnd), JoystickControl::wm_joystick_position_changed, 0, 0);
    }
    break;
    case WM_MBUTTONDOWN: {
        int x{}, y{};
        get_cursor_to_joystick_position(hwnd, x, y);

        ctx->cursor_diff_x = x - ctx->x;
        ctx->cursor_diff_y = y - ctx->y;

        ctx->mode = Mode::Relative;
        SendMessage(GetParent(hwnd), JoystickControl::wm_joystick_drag_begin, 0, 0);
        SetCapture(hwnd);
        break;
    }
    case WM_RBUTTONDOWN:
        if (ctx->mode == Mode::Sticky)
        {
            ctx->mode = Mode::None;
            ReleaseCapture();
            break;
        }
        ctx->mode = Mode::Sticky;
        SendMessage(GetParent(hwnd), JoystickControl::wm_joystick_drag_begin, 0, 0);
        SetCapture(hwnd);
        update_joystick_position(hwnd, ctx);
        break;
    case WM_LBUTTONDOWN:
        ctx->mode = Mode::Absolute;
        SendMessage(GetParent(hwnd), JoystickControl::wm_joystick_drag_begin, 0, 0);
        SetCapture(hwnd);
        update_joystick_position(hwnd, ctx);
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        if (ctx->mode == Mode::Sticky)
        {
            break;
        }
        ctx->mode = Mode::None;
        ReleaseCapture();
        break;
    case WM_MOUSEMOVE:
        update_joystick_position(hwnd, ctx);
        break;
    case WM_PAINT: {
        RECT rc{};
        GetClientRect(hwnd, &rc);

        rc.right -= 1;
        rc.bottom -= 1;
        const float mid_x = rc.right / 2.0;
        const float mid_y = rc.bottom / 2.0;
        const float stick_x = mid_x + ctx->x / 128.0f * (rc.right / 2.0f);
        const float stick_y = mid_y - ctx->y / 128.0f * (rc.bottom / 2.0f);
        update_clear_color(hwnd, ctx);
        ctx->g->Clear(ctx->clear_color);

        const auto luminance = 0.299 * ctx->clear_color.GetRed() + 0.587 * ctx->clear_color.GetGreen() +
                               0.114 * ctx->clear_color.GetBlue();
        const bool dark_background = luminance < 128.0;
        const auto contrastize = [dark_background](BYTE component, int amount) {
            if (dark_background)
            {
                return static_cast<BYTE>(component + (255 - component) * amount / 100);
            }
            return static_cast<BYTE>(component - component * amount / 100);
        };
        const Gdiplus::Color border_color(255, contrastize(ctx->clear_color.GetRed(), 25),
            contrastize(ctx->clear_color.GetGreen(), 25), contrastize(ctx->clear_color.GetBlue(), 25));
        const auto ellipse_color = dark_background ? Gdiplus::Color::Black : Gdiplus::Color::White;

        ctx->border_pen->SetColor(border_color);
        ctx->outline_pen->SetColor(border_color);
        ctx->bg_brush->SetColor(ellipse_color);

        const auto border_width = ctx->border_pen->GetWidth();
        const auto radius = 6.0f * border_width;
        const auto border_left = border_width / 2.0f;
        const auto border_top = border_width / 2.0f;
        const auto border_right = rc.right - border_width / 2.0f;
        const auto border_bottom = rc.bottom - border_width / 2.0f;
        Gdiplus::GraphicsPath border_path;
        border_path.AddArc(border_left, border_top, radius * 2.0f, radius * 2.0f, 180.0f, 90.0f);
        border_path.AddArc(border_right - radius * 2.0f, border_top, radius * 2.0f, radius * 2.0f, 270.0f, 90.0f);
        border_path.AddArc(
            border_right - radius * 2.0f, border_bottom - radius * 2.0f, radius * 2.0f, radius * 2.0f, 0.0f, 90.0f);
        border_path.AddArc(border_left, border_bottom - radius * 2.0f, radius * 2.0f, radius * 2.0f, 90.0f, 90.0f);
        border_path.CloseFigure();

        const auto tip_size = ctx->outline_pen->GetWidth() * 8.0f;

        ctx->g->FillEllipse(ctx->bg_brush, 0, 0, rc.right, rc.bottom);
        ctx->g->DrawEllipse(ctx->outline_pen, 0, 0, rc.right, rc.bottom);
        ctx->g->DrawLine(ctx->outline_pen, mid_x, 0.0f, mid_x, (float)rc.bottom);
        ctx->g->DrawLine(ctx->outline_pen, 0.0f, mid_y, (float)rc.right, mid_y);
        ctx->g->DrawLine(ctx->line_pen, mid_x, mid_y, stick_x, stick_y);
        ctx->g->FillEllipse(ctx->tip_brush, stick_x - tip_size / 2, stick_y - tip_size / 2, tip_size, tip_size);
        ctx->g->DrawPath(ctx->border_pen, &border_path);

        rc.right += 1;
        rc.bottom += 1;
        BitBlt(ctx->front_dc, 0, 0, rc.right, rc.bottom, ctx->back_dc, 0, 0, SRCCOPY);

        ValidateRect(hwnd, nullptr);

        ctx->painted_x = ctx->x;
        ctx->painted_y = ctx->y;
        return 0;
    }
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

}; // namespace Internal

/**
 * \brief Registers the joystick control window class.
 * \param hinstance The instance handle of the application.
 * \param name The class name to register.
 */
inline void register_class(HINSTANCE hinstance, std::string_view name)
{
    WNDCLASS wndclass{};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.lpfnWndProc = (WNDPROC)Internal::wndproc;
    wndclass.hInstance = hinstance;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = name.data();
    RegisterClass(&wndclass);
}

/**
 * \brief Retrieves the position of the joystick.
 * \param hwnd The handle of the joystick control.
 * \param x Out pointer to the x coordinate.
 * \param y Out pointer to the y coordinate.
 * \return Whether the operation succeeded.
 */
inline bool get_position(HWND hwnd, int *x, int *y)
{
    if (!IsWindow(hwnd)) return false;
    const auto ctx = Internal::get_ctx(hwnd);
    if (!ctx) return false;

    if (x) *x = ctx->x;
    if (y) *y = ctx->y;
    return true;
}

/**
 * \brief Sets the position of the joystick.
 * \param hwnd The handle of the joystick control.
 * \param x The x coordinate to set.
 * \param y The y coordinate to set.
 * \return Whether the operation succeeded.
 */
inline bool set_position(HWND hwnd, int x, int y)
{
    if (!IsWindow(hwnd)) return false;
    const auto ctx = Internal::get_ctx(hwnd);
    if (!ctx) return false;

    // Fast path: joystick already painted at this position
    if (ctx->painted_x == x && ctx->painted_y == y) return true;

    ctx->x = x;
    ctx->y = y;
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
    SendMessage(GetParent(hwnd), wm_joystick_position_changed, 1, 0);
    return true;
}

} // namespace JoystickControl
