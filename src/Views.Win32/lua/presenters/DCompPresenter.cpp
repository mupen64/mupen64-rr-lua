/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "DCompPresenter.h"

bool DCompPresenter::init(HWND hwnd)
{
    m_hwnd = hwnd;

    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_size = {(UINT32)rect.right - rect.left, (UINT32)rect.bottom - rect.top};

    CreateDXGIFactory2(0, IID_PPV_ARGS(dxgi_factory.GetAddressOf()));
    dxgi_factory->EnumAdapters1(0, dxgi_adapter.GetAddressOf());

    D3D11CreateDevice(dxgi_adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0,
                      D3D11_SDK_VERSION, &d3d11_device, nullptr, d3d_dc.GetAddressOf());

    d3d11_device->QueryInterface(dxgi_device.GetAddressOf());
    dxgi_device->SetMaximumFrameLatency(1);

    DCompositionCreateDevice(dxgi_device.Get(), IID_PPV_ARGS(comp_device.GetAddressOf()));
    comp_device->CreateTargetForHwnd(hwnd, true, comp_target.GetAddressOf());
    comp_device->CreateVisual(comp_visual.GetAddressOf());

    DXGI_SWAP_CHAIN_DESC1 swapdesc{};
    swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapdesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    swapdesc.SampleDesc.Count = 1;
    swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapdesc.BufferCount = 2;
    swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapdesc.Width = m_size.width;
    swapdesc.Height = m_size.height;

    dxgi_factory->CreateSwapChainForComposition(d3d11_device.Get(), &swapdesc, nullptr, dxgi_swapchain.GetAddressOf());
    comp_visual->SetContent(dxgi_swapchain.Get());
    comp_target->SetRoot(comp_visual.Get());

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, {}, d2d_factory.GetAddressOf());
    d2d_factory->CreateDevice(dxgi_device.Get(), d2d_device.GetAddressOf());
    d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
                                    d2d_dc.GetAddressOf());

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = m_size.width;
    desc.Height = m_size.height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc = {.Count = 1, .Quality = 0};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

    d3d11_device->CreateTexture2D(&desc, nullptr, d3d11_gdi_tex.GetAddressOf());
    d3d11_gdi_tex->QueryInterface(dxgi_surface.GetAddressOf());

    const UINT dpi = GetDpiForWindow(hwnd);
    const D2D1_BITMAP_PROPERTIES1 props =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);

    d2d_dc->CreateBitmapFromDxgiSurface(dxgi_surface.Get(), props, d2d1_bitmap.GetAddressOf());
    d2d_dc->SetTarget(d2d1_bitmap.Get());

    dxgi_swapchain->GetBuffer(1, IID_PPV_ARGS(d3d11_front_buffer.GetAddressOf()));
    dxgi_surface->QueryInterface(d3d11_surface.GetAddressOf());

    return true;
}

ID2D1RenderTarget *DCompPresenter::dc() const
{
    return d2d_dc.Get();
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
    d2d_dc->SetTarget(nullptr);
    d2d1_bitmap.Reset();
    dxgi_surface.Reset();
    d3d11_front_buffer.Reset();
    d3d11_surface.Reset();
    d3d11_gdi_tex.Reset();

    // 2. Resize the swapchain buffers
    dxgi_swapchain->ResizeBuffers(2, size.width, size.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

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

    d3d11_device->CreateTexture2D(&desc, nullptr, d3d11_gdi_tex.GetAddressOf());
    d3d11_gdi_tex->QueryInterface(dxgi_surface.GetAddressOf());

    // 4. Recreate the D2D bitmap target from the new DXGI surface
    const UINT dpi = GetDpiForWindow(m_hwnd);
    const D2D1_BITMAP_PROPERTIES1 props =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);

    d2d_dc->CreateBitmapFromDxgiSurface(dxgi_surface.Get(), props, d2d1_bitmap.GetAddressOf());
    d2d_dc->SetTarget(d2d1_bitmap.Get());

    // 5. Re-acquire the front buffer and surface references from the resized swapchain
    dxgi_swapchain->GetBuffer(1, IID_PPV_ARGS(d3d11_front_buffer.GetAddressOf()));
    dxgi_surface->QueryInterface(d3d11_surface.GetAddressOf());
}

void DCompPresenter::present()
{
    // 1. Copy Direct2D graphics to the GDI-compatible texture
    Microsoft::WRL::ComPtr<ID3D11Resource> d2d_render_target;
    dxgi_surface->QueryInterface(d2d_render_target.GetAddressOf());
    d3d_dc->CopyResource(d3d11_gdi_tex.Get(), d2d_render_target.Get());

    // 2. Copy the GDI-compatible texture to the swapchain's back buffer
    Microsoft::WRL::ComPtr<ID3D11Resource> back_buffer;
    dxgi_swapchain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    d3d_dc->CopyResource(back_buffer.Get(), d3d11_gdi_tex.Get());

    dxgi_swapchain->Present(0, 0);
    comp_device->Commit();
}

void DCompPresenter::blit(HDC hdc, RECT rect)
{
    // 1. Get our GDI-compatible D3D texture's DC
    Microsoft::WRL::ComPtr<IDXGISurface1> dxgi_surface;
    d3d11_gdi_tex->QueryInterface(dxgi_surface.GetAddressOf());

    HDC dc;
    dxgi_surface->GetDC(false, &dc);

    // 2. Blit our texture DC to the target DC, while preserving the alpha channel with AlphaBlend
    AlphaBlend(hdc, 0, 0, m_size.width, m_size.height, dc, 0, 0, m_size.width, m_size.height,
               {.BlendOp = AC_SRC_OVER, .BlendFlags = 0, .SourceConstantAlpha = 255, .AlphaFormat = AC_SRC_ALPHA});

    // 3. Cleanup
    dxgi_surface->ReleaseDC(nullptr);
}
