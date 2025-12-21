/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <JoystickControl.h>

#undef max

#define WITH_VALID_CTX()            \
    if (!IsWindow(hwnd))            \
    {                               \
        return FALSE;               \
    }                               \
    const auto ctx = get_ctx(hwnd); \
    if (!ctx)                       \
    {                               \
        return FALSE;               \
    }

struct t_context {
    enum class Mode {
        None,
        Absolute,
        Sticky,
        Relative
    };

    double x{};
    double y{};
    double cursor_diff_x{};
    double cursor_diff_y{};
    Mode mode = Mode::None;
    HDC front_dc{};
    HDC back_dc{};
    HBITMAP back_bmp{};
    Gdiplus::Graphics* g{};
    Gdiplus::Color clear_color{};
    Gdiplus::Brush* bg_brush{};
    Gdiplus::Brush* tip_brush{};
    Gdiplus::Pen* outline_pen{};
    Gdiplus::Pen* line_pen{};
};

using Mode = t_context::Mode;

static double remap(const double value, const double from1, const double to1, const double from2, const double to2)
{
    return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
}

static void get_cursor_to_joystick_position(const HWND hwnd, double& x, double& y)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    POINT pt{};
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    x = remap(pt.x, 0, rc.right, -1.0, 1.0);
    y = remap(pt.y, 0, rc.bottom, -1.0, 1.0);

    if (std::abs(x) > 1.0 || std::abs(y) > 1.0)
    {
        const auto div = std::max(std::abs(x), std::abs(y));
        x /= div;
        y /= div;
    }
}

static void update_joystick_position(HWND hwnd, t_context* ctx)
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

    if (abs(ctx->x) <= 8.0 / rc.right)
        ctx->x = 0.0;
    if (abs(ctx->y) <= 8.0 / rc.bottom)
        ctx->y = 0.0;

    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
    SendMessage(GetParent(hwnd), JoystickControl::WM_JOYSTICK_POSITION_CHANGED, 0, 0);
}

static void destroy_dcs(const HWND hwnd, t_context* ctx)
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
    delete ctx->line_pen;
}

static void create_dcs(const HWND hwnd, t_context* ctx)
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

    ctx->bg_brush = new Gdiplus::SolidBrush(Gdiplus::Color::White);
    ctx->tip_brush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 0, 0));
    ctx->outline_pen = new Gdiplus::Pen(Gdiplus::Color(255, 0, 0, 0), 1.0f * scale);
    ctx->line_pen = new Gdiplus::Pen(Gdiplus::Color(255, 0, 0, 255), 3.0f * scale);

    // We get the clear color by asking the parent window for its background brush, so this works even with weird themes
    const auto parent_hwnd = GetParent(hwnd);
    const auto parent_dc = GetDC(parent_hwnd);
    const auto clear_brush = (HBRUSH)SendMessage(parent_hwnd, WM_CTLCOLORDLG, (WPARAM)parent_dc, 0);

    LOGBRUSH log_brush{};
    GetObject(clear_brush, sizeof(log_brush), &log_brush);

    ctx->clear_color.SetFromCOLORREF(log_brush.lbColor);

    ReleaseDC(parent_hwnd, parent_dc);
    DeleteObject(clear_brush);
}

static auto get_ctx(const HWND hwnd)
{
    return reinterpret_cast<t_context*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    const auto ctx = get_ctx(hwnd);

    switch (msg)
    {
    case WM_NCCREATE:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG) new t_context());
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
    case WM_MOUSEWHEEL:
        {
            const auto delta = GET_WHEEL_DELTA_WPARAM(wparam);
            RECT rc{};
            GetClientRect(hwnd, &rc);

            const auto increment_sign = delta > 0.0 ? 1.0 : -1.0;
            const auto increment_x = 1.0 / rc.right * increment_sign;
            const auto increment_y = -1.0 / rc.bottom * increment_sign;

            if (GetKeyState(VK_CONTROL) & 0x8000)
            {
                ctx->y += increment_y;
            }
            else if (GetKeyState(VK_SHIFT) & 0x8000)
            {
                const auto angle = increment_sign * M_PI / 180.0;
                const auto cos_angle = cos(angle);
                const auto sin_angle = sin(angle);
                ctx->x = ctx->x * cos_angle - ctx->y * sin_angle;
                ctx->y = ctx->x * sin_angle + ctx->y * cos_angle;
            }
            else
            {
                ctx->x += increment_x;
            }

            ctx->x = wrapping_clamp(ctx->x, -1.0, 1.0);
            ctx->y = wrapping_clamp(ctx->y, -1.0, 1.0);

            RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
            SendMessage(GetParent(hwnd), JoystickControl::WM_JOYSTICK_POSITION_CHANGED, 0, 0);
        }
        break;
    case WM_MBUTTONDOWN:
        {
            double x{}, y{};
            get_cursor_to_joystick_position(hwnd, x, y);

            ctx->cursor_diff_x = x - ctx->x;
            ctx->cursor_diff_y = y - ctx->y;

            ctx->mode = Mode::Relative;
            SendMessage(GetParent(hwnd), JoystickControl::WM_JOYSTICK_DRAG_BEGIN, 0, 0);
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
        SendMessage(GetParent(hwnd), JoystickControl::WM_JOYSTICK_DRAG_BEGIN, 0, 0);
        SetCapture(hwnd);

        update_joystick_position(hwnd, ctx);
        break;
    case WM_LBUTTONDOWN:
        ctx->mode = Mode::Absolute;
        SendMessage(GetParent(hwnd), JoystickControl::WM_JOYSTICK_DRAG_BEGIN, 0, 0);
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
    case WM_PAINT:
        {
            RECT rc{};
            GetClientRect(hwnd, &rc);

            rc.right -= 1;
            rc.bottom -= 1;
            const float mid_x = rc.right / 2.0;
            const float mid_y = rc.bottom / 2.0;
            const float stick_x = remap(ctx->x, -1.0, 1.0, 0.0, rc.right);
            const float stick_y = remap(ctx->y, -1.0, 1.0, 0.0, rc.bottom);

            ctx->g->Clear(ctx->clear_color);

            const auto tip_size = ctx->outline_pen->GetWidth() * 8.0f;

            ctx->g->FillEllipse(ctx->bg_brush, 0, 0, rc.right, rc.bottom);
            ctx->g->DrawEllipse(ctx->outline_pen, 0, 0, rc.right, rc.bottom);
            ctx->g->DrawLine(ctx->outline_pen, mid_x, 0.0f, mid_x, (float)rc.bottom);
            ctx->g->DrawLine(ctx->outline_pen, 0.0f, mid_y, (float)rc.right, mid_y);
            ctx->g->DrawLine(ctx->line_pen, mid_x, mid_y, stick_x, stick_y);
            ctx->g->FillEllipse(ctx->tip_brush, stick_x - tip_size / 2, stick_y - tip_size / 2, tip_size, tip_size);

            rc.right += 1;
            rc.bottom += 1;
            BitBlt(ctx->front_dc, 0, 0, rc.right, rc.bottom, ctx->back_dc, 0, 0, SRCCOPY);

            ValidateRect(hwnd, nullptr);
            return 0;
        }
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void JoystickControl::register_class(HINSTANCE hinst)
{
    WNDCLASS wndclass{};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.lpfnWndProc = (WNDPROC)wndproc;
    wndclass.hInstance = hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = CLASS_NAME;
    RegisterClass(&wndclass);
}

BOOL JoystickControl::get_position(HWND hwnd, int* x, int* y)
{
    WITH_VALID_CTX()

    RECT rc{};
    GetClientRect(hwnd, &rc);

    if (x)
    {
        *x = (int)std::round(remap(ctx->x, -1.0, 1.0, -128, 127));
    }
    if (y)
    {
        *y = (int)std::round(remap(ctx->y, 1.0, -1.0, -128, 127));
    }

    return TRUE;
}

BOOL JoystickControl::set_position(HWND hwnd, int x, int y)
{
    WITH_VALID_CTX()

    ctx->x = remap(x, -128, 127, -1.0, 1.0);
    ctx->y = -remap(y, -128, 127, -1.0, 1.0);

    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
    SendMessage(GetParent(hwnd), WM_JOYSTICK_POSITION_CHANGED, 1, 0);

    return TRUE;
}
