/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "DCompPresenter.h"

DCompPresenter::~DCompPresenter()
{
    m_d3d_gdi_tex->Release();
    m_front_buffer->Release();
    m_dxgi_surface_resource->Release();
    m_dxgi_surface->Release();
    m_d2d_dc->Release();
    m_d3d_dc->Release();
    m_comp_device->Release();
    m_comp_target->Release();
    m_dxgi_swapchain->Release();
    m_d2d_factory->Release();
    m_d2d_device->Release();
    m_comp_visual->Release();
    m_bitmap->Release();
    m_dxdevice->Release();
    m_d3device->Release();
    m_dxgiadapter->Release();
    m_factory->Release();
}

bool DCompPresenter::init(HWND hwnd)
{
    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_size = {(UINT32)rect.right - rect.left, (UINT32)rect.bottom - rect.top};

    // Create the whole DComp <-> DXGI <-> D3D11 <-> D2D shabang
    if (!create_composition_surface(hwnd, m_size, &m_factory, &m_dxgiadapter, &m_d3device, &m_dxdevice, &m_bitmap, &m_comp_visual,
                                    &m_comp_device, &m_comp_target,
                                    &m_dxgi_swapchain, &m_d2d_factory, &m_d2d_device, &m_d3d_dc, &m_d2d_dc, &m_dxgi_surface, &m_dxgi_surface_resource, &m_front_buffer,
                                    &m_d3d_gdi_tex))
    {
        return false;
    }

    return true;
}

ID2D1RenderTarget* DCompPresenter::dc() const
{
    return m_d2d_dc;
}

void DCompPresenter::begin_present()
{
    m_d2d_dc->BeginDraw();
    m_d3d_dc->CopyResource(m_dxgi_surface_resource, m_front_buffer);
    m_d2d_dc->SetTransform(D2D1::Matrix3x2F::Identity());
}

void DCompPresenter::end_present()
{
    m_d2d_dc->EndDraw();

    ID3D11Resource* back_buffer = nullptr;
    m_dxgi_swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    
    m_d3d_dc->CopyResource(back_buffer, m_d3d_gdi_tex);
    back_buffer->Release();
    
    m_dxgi_swapchain->Present(0, 0);
    m_comp_device->Commit();
}

void DCompPresenter::blit(HDC hdc, RECT rect)
{
    // 1. Get our GDI-compatible D3D texture's DC
    HDC dc;
    IDXGISurface1* dxgi_surface;
    m_d3d_gdi_tex->QueryInterface(&dxgi_surface);
    dxgi_surface->GetDC(false, &dc);
    
    // 2. Blit our texture DC to the target DC, while preserving the alpha channel with AlphaBlend
    AlphaBlend(hdc, 0, 0, m_size.width, m_size.height, dc, 0, 0, m_size.width, m_size.height, {
                   .BlendOp = AC_SRC_OVER,
                   .BlendFlags = 0,
                   .SourceConstantAlpha = 255,
                   .AlphaFormat = AC_SRC_ALPHA
               });
    
    // 3. Cleanup
    dxgi_surface->ReleaseDC(nullptr);
    dxgi_surface->Release();
}

D2D1_SIZE_U DCompPresenter::size()
{
    return m_size;
}
