/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <lua/LuaRenderer.h>
#include <lua/presenters/GDIPresenter.h>

GDIPresenter::~GDIPresenter()
{
    for (auto &ctx : m_render_target_contexts)
    {
        if (ctx.render_target)
        {
            ctx.render_target->Release();
        }
        if (ctx.gdi_dc)
        {
            SelectObject(ctx.gdi_dc, nullptr);
            DeleteDC(ctx.gdi_dc);
        }
        if (ctx.gdi_bmp)
        {
            DeleteObject(ctx.gdi_bmp);
        }
    }
    m_render_target_contexts.clear();

    if (m_d2d_factory)
    {
        m_d2d_factory->Release();
    }
}

bool GDIPresenter::init(HWND hwnd)
{
    m_hwnd = hwnd;

    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_size = {(UINT32)rect.right - rect.left, (UINT32)rect.bottom - rect.top};

    // 2. Make the provided window (which must have WS_EX_LAYERED) mask out our clear color, then fill the back DC with
    // that color (effectively clearing it)
    SetLayeredWindowAttributes(hwnd, LuaRenderer::LUA_GDI_COLOR_MASK, 0, LWA_COLORKEY);

    // Create a D2D1 factory for render target creation
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2d_factory);

    return true;
}

ID2D1RenderTarget *GDIPresenter::add_rt()
{
    // Create a new DC and bitmap for this render target
    auto window_dc = GetDC(m_hwnd);
    HDC gdi_dc = CreateCompatibleDC(window_dc);
    HBITMAP gdi_bmp = CreateCompatibleBitmap(window_dc, m_size.width, m_size.height);
    SelectObject(gdi_dc, gdi_bmp);
    ReleaseDC(m_hwnd, window_dc);

    // Fill with the mask color
    RECT rect = {0, 0, (LONG)m_size.width, (LONG)m_size.height};
    FillRect(gdi_dc, &rect, LuaRenderer::alpha_mask_brush());

    // Create D2D render target bound to this DC
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    ID2D1DCRenderTarget *render_target = nullptr;
    m_d2d_factory->CreateDCRenderTarget(&props, &render_target);
    render_target->BindDC(gdi_dc, &rect);

    // Store the context
    m_render_target_contexts.push_back({render_target, gdi_dc, gdi_bmp});

    return render_target;
}

void GDIPresenter::remove_rt(const ID2D1RenderTarget *dc)
{
    auto it = std::find_if(m_render_target_contexts.begin(), m_render_target_contexts.end(),
                           [dc](const RenderTargetContext &ctx) { return ctx.render_target == dc; });

    if (it != m_render_target_contexts.end())
    {
        if (it->render_target)
        {
            it->render_target->Release();
        }
        if (it->gdi_dc)
        {
            SelectObject(it->gdi_dc, nullptr);
            DeleteDC(it->gdi_dc);
        }
        if (it->gdi_bmp)
        {
            DeleteObject(it->gdi_bmp);
        }
        m_render_target_contexts.erase(it);
    }
}

D2D1_SIZE_U GDIPresenter::size()
{
    return m_size;
}

void GDIPresenter::begin_present()
{
    for (auto &ctx : m_render_target_contexts)
    {
        ctx.render_target->BeginDraw();
        ctx.render_target->SetTransform(D2D1::Matrix3x2F::Identity());
    }
}

void GDIPresenter::end_present()
{
    for (auto &ctx : m_render_target_contexts)
    {
        ctx.render_target->EndDraw();
    }

    // Composite all render targets sequentially to the window
    auto main_dc = GetDC(m_hwnd);
    for (auto &ctx : m_render_target_contexts)
    {
        TransparentBlt(main_dc, 0, 0, m_size.width, m_size.height, ctx.gdi_dc, 0, 0, m_size.width, m_size.height,
                       LuaRenderer::LUA_GDI_COLOR_MASK);
    }
    ReleaseDC(m_hwnd, main_dc);
}

void GDIPresenter::blit(HDC hdc, RECT rect)
{
    // Composite all render targets sequentially to the target DC
    for (auto &ctx : m_render_target_contexts)
    {
        TransparentBlt(hdc, 0, 0, m_size.width, m_size.height, ctx.gdi_dc, 0, 0, m_size.width, m_size.height,
                       LuaRenderer::LUA_GDI_COLOR_MASK);
    }
}

D2D1::ColorF GDIPresenter::adjust_clear_color(const D2D1::ColorF color) const
{
    return D2D1::ColorF(LuaRenderer::LUA_GDI_COLOR_MASK);
}
