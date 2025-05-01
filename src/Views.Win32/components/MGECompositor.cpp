/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "MGECompositor.h"
#include <Messenger.h>

#include <Main.h>

constexpr auto CONTROL_CLASS_NAME = L"game_control";

struct t_mge_context {
    int32_t last_width{};
    int32_t last_height{};
    int32_t width{};
    int32_t height{};
    void* buffer{};
    BITMAPINFO bmp_info{};
    HBITMAP dib{};
    HDC dc{};
};

static HWND mge_hwnd;
static t_mge_context mge_context{};

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rect{};
            GetClientRect(hwnd, &rect);

            BitBlt(hdc, 0, 0, mge_context.bmp_info.bmiHeader.biWidth, mge_context.bmp_info.bmiHeader.biHeight, mge_context.dc, 0, 0, SRCCOPY);

            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void recreate_mge_context()
{
    g_view_logger->info("Creating MGE context with size {}x{}...", mge_context.width, mge_context.height);

    mge_context.bmp_info.bmiHeader.biWidth = mge_context.width;
    mge_context.bmp_info.bmiHeader.biHeight = mge_context.height;

    if (mge_context.dib)
    {
        SelectObject(mge_context.dc, nullptr);
        DeleteObject(mge_context.dc);
        DeleteObject(mge_context.dib);
    }

    const auto mge_dc = GetDC(mge_hwnd);

    mge_context.dc = CreateCompatibleDC(mge_dc);
    mge_context.dib = CreateDIBSection(mge_context.dc, &mge_context.bmp_info, DIB_RGB_COLORS, &mge_context.buffer, nullptr, 0);
    SelectObject(mge_context.dc, mge_context.dib);

    ReleaseDC(mge_hwnd, mge_dc);
}

void MGECompositor::create(HWND hwnd)
{
    mge_hwnd = CreateWindow(CONTROL_CLASS_NAME, L"", WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hwnd, nullptr, g_app_instance, nullptr);
}

void MGECompositor::init()
{
    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)wndproc;
    wndclass.hInstance = g_app_instance;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = CONTROL_CLASS_NAME;
    RegisterClass(&wndclass);

    mge_context.bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    mge_context.bmp_info.bmiHeader.biPlanes = 1;
    mge_context.bmp_info.bmiHeader.biBitCount = 24;
    mge_context.bmp_info.bmiHeader.biCompression = BI_RGB;

    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, [](const std::any& data) {
        const auto value = std::any_cast<bool>(data);
        ShowWindow(mge_hwnd, value && core_vr_get_mge_available() ? SW_SHOW : SW_HIDE);
    });
}

void MGECompositor::update_screen()
{
    g_core.plugin_funcs.video_get_video_size(&mge_context.width, &mge_context.height);

    if (mge_context.width != mge_context.last_width || mge_context.height != mge_context.last_height)
    {
        recreate_mge_context();

        MoveWindow(mge_hwnd, 0, 0, mge_context.width, mge_context.height, true);
    }

    g_core.plugin_funcs.video_read_video(&mge_context.buffer);

    mge_context.last_width = mge_context.width;
    mge_context.last_height = mge_context.height;

    RedrawWindow(mge_hwnd, nullptr, nullptr, RDW_INVALIDATE);
}

void MGECompositor::get_video_size(int32_t* width, int32_t* height)
{
    if (width)
    {
        *width = mge_context.width;
    }
    if (height)
    {
        *height = mge_context.height;
    }
}

void MGECompositor::copy_video(void* buffer)
{
    memcpy(buffer, mge_context.buffer, mge_context.width * mge_context.height * 3);
}

void MGECompositor::load_screen(void* data)
{
    memcpy(mge_context.buffer, data, mge_context.width * mge_context.height * 3);
    RedrawWindow(mge_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}
