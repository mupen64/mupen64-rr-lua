/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "DCompPresenter.h"

void DCompPresenter::create_size_dependent_resources()
{
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

    m_d3d_device->CreateTexture2D(&desc, nullptr, m_d3d_gdi_tex.GetAddressOf());
    m_d3d_gdi_tex->QueryInterface(m_dxgi_surface.GetAddressOf());

    const auto dpi = (float)GetDpiForWindow(m_hwnd);
    const D2D1_BITMAP_PROPERTIES1 props =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);

    m_d2d_dc->CreateBitmapFromDxgiSurface(m_dxgi_surface.Get(), props, m_d2d_bitmap.GetAddressOf());
    m_d2d_dc->SetTarget(m_d2d_bitmap.Get());
}

bool DCompPresenter::init(HWND hwnd)
{
    m_hwnd = hwnd;

    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_size = {(UINT32)rect.right - rect.left, (UINT32)rect.bottom - rect.top};

    CreateDXGIFactory2(0, IID_PPV_ARGS(m_dxgi_factory.GetAddressOf()));
    m_dxgi_factory->EnumAdapters1(0, m_dxgi_adapter.GetAddressOf());

    D3D11CreateDevice(m_dxgi_adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0,
                      D3D11_SDK_VERSION, &m_d3d_device, nullptr, m_d3d_dc.GetAddressOf());

    m_d3d_device->QueryInterface(m_dxgi_device.GetAddressOf());
    m_dxgi_device->SetMaximumFrameLatency(1);

    DCompositionCreateDevice(m_dxgi_device.Get(), IID_PPV_ARGS(m_comp_device.GetAddressOf()));
    m_comp_device->CreateTargetForHwnd(hwnd, true, m_comp_target.GetAddressOf());
    m_comp_device->CreateVisual(m_comp_visual.GetAddressOf());

    DXGI_SWAP_CHAIN_DESC1 swapdesc{};
    swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapdesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    swapdesc.SampleDesc.Count = 1;
    swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapdesc.BufferCount = 2;
    swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapdesc.Width = m_size.width;
    swapdesc.Height = m_size.height;

    m_dxgi_factory->CreateSwapChainForComposition(m_d3d_device.Get(), &swapdesc, nullptr,
                                                  m_dxgi_swapchain.GetAddressOf());
    m_comp_visual->SetContent(m_dxgi_swapchain.Get());
    m_comp_target->SetRoot(m_comp_visual.Get());

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, {}, m_d2d_factory.GetAddressOf());
    m_d2d_factory->CreateDevice(m_dxgi_device.Get(), m_d2d_device.GetAddressOf());
    m_d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
                                      m_d2d_dc.GetAddressOf());

    create_size_dependent_resources();

    return true;
}

ID2D1RenderTarget *DCompPresenter::dc() const
{
    return m_d2d_dc.Get();
}

D2D1_SIZE_U DCompPresenter::size()
{
    return m_size;
}

void DCompPresenter::resize(D2D1_SIZE_U size)
{
    if (size == m_size) return;

    m_size = size;

    // Release size-dependent resources before resizing the swapchain
    m_d2d_dc->SetTarget(nullptr);
    m_d2d_bitmap.Reset();
    m_dxgi_surface.Reset();
    m_d3d_gdi_tex.Reset();

    m_dxgi_swapchain->ResizeBuffers(2, size.width, size.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

    create_size_dependent_resources();
}

void DCompPresenter::present()
{
    // Copy the D3D texture to the swapchain back buffer.
    ComPtr<ID3D11Resource> back_buffer;
    m_dxgi_swapchain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    m_d3d_dc->CopyResource(back_buffer.Get(), m_d3d_gdi_tex.Get());

    m_dxgi_swapchain->Present(0, 0);
    m_comp_device->Commit();
}

void DCompPresenter::blit(HDC hdc, RECT rect)
{
    HDC dc;
    m_dxgi_surface->GetDC(false, &dc);

    AlphaBlend(hdc, 0, 0, m_size.width, m_size.height, dc, 0, 0, m_size.width, m_size.height,
               {.BlendOp = AC_SRC_OVER, .BlendFlags = 0, .SourceConstantAlpha = 255, .AlphaFormat = AC_SRC_ALPHA});

    m_dxgi_surface->ReleaseDC(nullptr);
}
