/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <lua/presenters/GDIPresenter.h>

GDIPresenter::~GDIPresenter()
{
    SelectObject(m_gdi_back_dc, nullptr);
    DeleteObject(m_gdi_bmp);
    DeleteDC(m_gdi_back_dc);
}

bool GDIPresenter::init(HWND hwnd)
{
    m_hwnd = hwnd;

    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_size = {(UINT32)rect.right - rect.left, (UINT32)rect.bottom - rect.top};

    // 1. Create a back-buffer DC pointed to a bitmap
    auto gdi_dc = GetDC(hwnd);
    m_gdi_back_dc = CreateCompatibleDC(gdi_dc);
    m_gdi_bmp = CreateCompatibleBitmap(gdi_dc, m_size.width, m_size.height);
    SelectObject(m_gdi_back_dc, m_gdi_bmp);
    ReleaseDC(hwnd, gdi_dc);

    // 2. Create a D2D1 RT and point it to our back DC
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2d_factory.GetAddressOf());
    m_d2d_factory->CreateDCRenderTarget(&props, m_d2d_render_target.GetAddressOf());

    RECT dc_rect = {0, 0, (LONG)m_size.width, (LONG)m_size.height};
    m_d2d_render_target->BindDC(m_gdi_back_dc, &dc_rect);

    return true;
}

ID2D1RenderTarget *GDIPresenter::dc() const
{
    return m_d2d_render_target.Get();
}

D2D1_SIZE_U GDIPresenter::size()
{
    return m_size;
}

void GDIPresenter::resize(D2D1_SIZE_U size)
{
    if (size == m_size) return;

    m_size = size;

    SelectObject(m_gdi_back_dc, nullptr);
    DeleteObject(m_gdi_bmp);

    auto gdi_dc = GetDC(m_hwnd);
    m_gdi_bmp = CreateCompatibleBitmap(gdi_dc, size.width, size.height);
    ReleaseDC(m_hwnd, gdi_dc);
    SelectObject(m_gdi_back_dc, m_gdi_bmp);

    RECT rect = {0, 0, (LONG)size.width, (LONG)size.height};
    const auto alpha_mask_brush = CreateSolidBrush(m_mask_color);
    FillRect(m_gdi_back_dc, &rect, alpha_mask_brush);
    DeleteObject(alpha_mask_brush);

    m_d2d_render_target->BindDC(m_gdi_back_dc, &rect);
}

void GDIPresenter::present()
{
    SIZE size = {(LONG)m_size.width, (LONG)m_size.height};
    POINT src_pt = {0, 0};

    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = 0;
    UpdateLayeredWindow(m_hwnd, nullptr, nullptr, &size, m_gdi_back_dc, &src_pt, m_mask_color, &bf, ULW_COLORKEY);
}

void GDIPresenter::blit(HDC hdc, RECT rect)
{
    TransparentBlt(hdc, 0, 0, m_size.width, m_size.height, m_gdi_back_dc, 0, 0, m_size.width, m_size.height,
                   m_mask_color);
}
