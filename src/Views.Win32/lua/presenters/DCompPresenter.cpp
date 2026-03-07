/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "DCompPresenter.h"

DCompPresenter::~DCompPresenter()
{
    m_cmp.d3d11_gdi_tex->Release();
    m_cmp.d3d11_front_buffer->Release();
    m_cmp.d3d11_surface->Release();
    m_cmp.dxgi_surface->Release();
    m_cmp.d2d_dc->Release();
    m_cmp.d3d_dc->Release();
    m_cmp.comp_device->Release();
    m_cmp.comp_target->Release();
    m_cmp.dxgi_swapchain->Release();
    m_cmp.d2d_factory->Release();
    m_cmp.d2d_device->Release();
    m_cmp.comp_visual->Release();
    m_cmp.d2d1_bitmap->Release();
    m_cmp.dxgi_device->Release();
    m_cmp.d3d11_device->Release();
    m_cmp.dxgi_adapter->Release();
    m_cmp.dxgi_factory->Release();
}

bool DCompPresenter::init(HWND hwnd)
{
    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_size = {(UINT32)rect.right - rect.left, (UINT32)rect.bottom - rect.top};

    const auto cmp = create_composition_context(hwnd, m_size);

    if (!cmp.has_value())
    {
        return false;
    }

    m_cmp = cmp.value();

    return true;
}

ID2D1RenderTarget *DCompPresenter::dc() const
{
    return m_cmp.d2d_dc;
}

void DCompPresenter::begin_present()
{
    m_cmp.d2d_dc->BeginDraw();
    m_cmp.d3d_dc->CopyResource(m_cmp.d3d11_surface, m_cmp.d3d11_front_buffer);
    m_cmp.d2d_dc->SetTransform(D2D1::Matrix3x2F::Identity());
}

void DCompPresenter::end_present()
{
    m_cmp.d2d_dc->EndDraw();

    // 1. Copy Direct2D graphics to the GDI-compatible texture
    ID3D11Resource *d2d_render_target = nullptr;
    m_cmp.dxgi_surface->QueryInterface(&d2d_render_target);
    m_cmp.d3d_dc->CopyResource(m_cmp.d3d11_gdi_tex, d2d_render_target);
    d2d_render_target->Release();

    // 2. Copy the GDI-compatible texture to the swapchain's back buffer
    ID3D11Resource *back_buffer = nullptr;
    m_cmp.dxgi_swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    m_cmp.d3d_dc->CopyResource(back_buffer, m_cmp.d3d11_gdi_tex);
    back_buffer->Release();

    m_cmp.dxgi_swapchain->Present(0, 0);
    m_cmp.comp_device->Commit();
}

void DCompPresenter::blit(HDC hdc, RECT rect)
{
    // 1. Get our GDI-compatible D3D texture's DC
    HDC dc;
    IDXGISurface1 *dxgi_surface;
    m_cmp.d3d11_gdi_tex->QueryInterface(&dxgi_surface);
    dxgi_surface->GetDC(false, &dc);

    // 2. Blit our texture DC to the target DC, while preserving the alpha channel with AlphaBlend
    AlphaBlend(hdc, 0, 0, m_size.width, m_size.height, dc, 0, 0, m_size.width, m_size.height,
               {.BlendOp = AC_SRC_OVER, .BlendFlags = 0, .SourceConstantAlpha = 255, .AlphaFormat = AC_SRC_ALPHA});

    // 3. Cleanup
    dxgi_surface->ReleaseDC(nullptr);
    dxgi_surface->Release();
}

D2D1_SIZE_U DCompPresenter::size()
{
    return m_size;
}
