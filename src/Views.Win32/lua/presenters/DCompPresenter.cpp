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
    m_hwnd = hwnd;

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

D2D1_SIZE_U DCompPresenter::size()
{
    return m_size;
}

void DCompPresenter::resize(D2D1_SIZE_U size)
{
    if (size == m_size) return;

    m_size = size;

    // 1. Release size-dependent resources that must be recreated after a swapchain resize
    m_cmp.d2d_dc->SetTarget(nullptr);
    m_cmp.d2d1_bitmap->Release();
    m_cmp.dxgi_surface->Release();
    m_cmp.d3d11_front_buffer->Release();
    m_cmp.d3d11_surface->Release();
    m_cmp.d3d11_gdi_tex->Release();

    // 2. Resize the swapchain buffers
    m_cmp.dxgi_swapchain->ResizeBuffers(2, size.width, size.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

    // 3. Recreate the GDI-compatible texture at the new size
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = size.width;
    desc.Height = size.height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc = {.Count = 1, .Quality = 0};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

    m_cmp.d3d11_device->CreateTexture2D(&desc, nullptr, &m_cmp.d3d11_gdi_tex);
    m_cmp.d3d11_gdi_tex->QueryInterface(&m_cmp.dxgi_surface);

    // 4. Recreate the D2D bitmap target from the new DXGI surface
    const UINT dpi = GetDpiForWindow(m_hwnd);
    const D2D1_BITMAP_PROPERTIES1 props =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);

    m_cmp.d2d_dc->CreateBitmapFromDxgiSurface(m_cmp.dxgi_surface, props, &m_cmp.d2d1_bitmap);
    m_cmp.d2d_dc->SetTarget(m_cmp.d2d1_bitmap);

    // 5. Re-acquire the front buffer and surface references from the resized swapchain
    m_cmp.dxgi_swapchain->GetBuffer(1, IID_PPV_ARGS(&m_cmp.d3d11_front_buffer));
    m_cmp.dxgi_surface->QueryInterface(&m_cmp.d3d11_surface);
}

void DCompPresenter::present()
{
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
