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

    CreateDXGIFactory2(0, IID_PPV_ARGS(m_cmp.dxgi_factory.GetAddressOf()));
    m_cmp.dxgi_factory->EnumAdapters1(0, m_cmp.dxgi_adapter.GetAddressOf());

    D3D11CreateDevice(m_cmp.dxgi_adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0,
                      D3D11_SDK_VERSION, m_cmp.d3d11_device.GetAddressOf(), nullptr, m_cmp.d3d_dc.GetAddressOf());

    m_cmp.d3d11_device->QueryInterface(m_cmp.dxgi_device.GetAddressOf());

    DCompositionCreateDevice(m_cmp.dxgi_device.Get(), IID_PPV_ARGS(m_cmp.comp_device.GetAddressOf()));
    m_cmp.comp_device->CreateTargetForHwnd(hwnd, true, m_cmp.comp_target.GetAddressOf());

    m_cmp.comp_device->CreateVisual(m_cmp.comp_visual.GetAddressOf());
    m_cmp.comp_target->SetRoot(m_cmp.comp_visual.Get());

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, {}, m_cmp.d2d_factory.GetAddressOf());
    m_cmp.d2d_factory->CreateDevice(m_cmp.dxgi_device.Get(), m_cmp.d2d_device.GetAddressOf());

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

    m_cmp.d3d11_device->CreateTexture2D(&desc, nullptr, m_cmp.d3d11_gdi_tex.GetAddressOf());

    return true;
}

ID2D1RenderTarget *DCompPresenter::add_rt()
{
    auto rt = std::make_unique<RenderTarget>();

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = m_size.width;
    desc.Height = m_size.height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc = {.Count = 1, .Quality = 0};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = 0;

    m_cmp.d3d11_device->CreateTexture2D(&desc, nullptr, rt->d3d_texture.GetAddressOf());
    rt->d3d_texture->QueryInterface(rt->dxgi_surface.GetAddressOf());

    const UINT dpi = GetDpiForWindow(m_hwnd);
    const D2D1_BITMAP_PROPERTIES1 props =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);

    m_cmp.d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
                                          rt->device_context.GetAddressOf());

    rt->device_context->CreateBitmapFromDxgiSurface(rt->dxgi_surface.Get(), props, rt->d2d_bitmap.GetAddressOf());
    rt->device_context->SetTarget(rt->d2d_bitmap.Get());

    DXGI_SWAP_CHAIN_DESC1 swapdesc{};
    swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapdesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    swapdesc.SampleDesc.Count = 1;
    swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapdesc.BufferCount = 2;
    swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapdesc.Width = m_size.width;
    swapdesc.Height = m_size.height;

    m_cmp.dxgi_factory->CreateSwapChainForComposition(m_cmp.d3d11_device.Get(), &swapdesc, nullptr,
                                                      rt->comp_swapchain.GetAddressOf());

    m_cmp.comp_device->CreateVisual(rt->comp_visual.GetAddressOf());
    rt->comp_visual->SetContent(rt->comp_swapchain.Get());
    m_cmp.comp_visual->AddVisual(rt->comp_visual.Get(), TRUE, nullptr);
    m_cmp.comp_device->Commit();

    auto *ptr = rt.get();
    m_rts.push_back(std::move(rt));
    return ptr->device_context.Get();
}

void DCompPresenter::remove_rt(const ID2D1RenderTarget *rt)
{
    auto it =
        std::find_if(m_rts.begin(), m_rts.end(), [&](const auto &ctx) { return ctx->device_context.Get() == rt; });

    RT_ASSERT(it != m_rts.end(), L"Render target not found");

    m_cmp.comp_visual->RemoveVisual((*it)->comp_visual.Get());
    m_cmp.comp_device->Commit();

    m_rts.erase(it);
}

void DCompPresenter::resize(D2D1_SIZE_U size)
{
    if (size == m_size) return;

    m_size = size;

    // Recreate the GDI texture with the new size.
    m_cmp.d3d11_gdi_tex.Reset();
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
        m_cmp.d3d11_device->CreateTexture2D(&desc, nullptr, m_cmp.d3d11_gdi_tex.GetAddressOf());
    }

    for (auto &rt : m_rts)
    {
        // All this stuff is invalidated...
        rt->device_context->SetTarget(nullptr);
        rt->d2d_bitmap.Reset();
        rt->dxgi_surface.Reset();
        rt->d3d_texture.Reset();

        rt->comp_swapchain->ResizeBuffers(2, m_size.width, m_size.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

        // Recreate the rt texture with the new size.
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = m_size.width;
        desc.Height = m_size.height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc = {.Count = 1, .Quality = 0};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        desc.MiscFlags = 0;
        m_cmp.d3d11_device->CreateTexture2D(&desc, nullptr, rt->d3d_texture.GetAddressOf());
        rt->d3d_texture->QueryInterface(rt->dxgi_surface.GetAddressOf());

        // Recreate the rt bitmap with the new size. Note that we keep device_context as-is (see above).
        const UINT dpi = GetDpiForWindow(m_hwnd);
        const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);
        rt->device_context->CreateBitmapFromDxgiSurface(rt->dxgi_surface.Get(), props, rt->d2d_bitmap.GetAddressOf());
        rt->device_context->SetTarget(rt->d2d_bitmap.Get());
    }
}

void DCompPresenter::begin_present()
{
    for (auto &ctx : m_rts)
    {
        ctx->device_context->BeginDraw();
        ctx->device_context->SetTransform(D2D1::Matrix3x2F::Identity());
    }
}

void DCompPresenter::end_present()
{
    for (auto &rt : m_rts)
    {
        rt->device_context->EndDraw();

        Microsoft::WRL::ComPtr<ID3D11Resource> back_buffer;
        rt->comp_swapchain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
        m_cmp.d3d_dc->CopyResource(back_buffer.Get(), rt->d3d_texture.Get());

        rt->comp_swapchain->Present(0, 0);
    }

    m_cmp.comp_device->Commit();
}

void DCompPresenter::blit(HDC hdc, RECT rect)
{
    for (auto &rt : m_rts)
    {
        // 1. Blit rt content to the GDI-compatible texture
        m_cmp.d3d_dc->CopyResource(m_cmp.d3d11_gdi_tex.Get(), rt->d3d_texture.Get());

        // 2. Blit our texture DC to the target DC
        HDC dc;
        Microsoft::WRL::ComPtr<IDXGISurface1> dxgi_surface;
        m_cmp.d3d11_gdi_tex->QueryInterface(dxgi_surface.GetAddressOf());
        dxgi_surface->GetDC(false, &dc);

        AlphaBlend(hdc, 0, 0, m_size.width, m_size.height, dc, 0, 0, m_size.width, m_size.height,
                   {.BlendOp = AC_SRC_OVER, .BlendFlags = 0, .SourceConstantAlpha = 255, .AlphaFormat = AC_SRC_ALPHA});

        // 3. Cleanup
        dxgi_surface->ReleaseDC(nullptr);
    }
}

D2D1_SIZE_U DCompPresenter::size()
{
    return m_size;
}
