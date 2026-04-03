/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Presenter.h"

class DCompPresenter : public Presenter
{
  public:
    ~DCompPresenter() override = default;
    bool init(HWND hwnd) override;
    ID2D1RenderTarget *dc() const override;
    D2D1_SIZE_U size() override;
    void resize(D2D1_SIZE_U size) override;
    void present() override;
    void blit(HDC hdc, RECT rect) override;

  private:
    HWND m_hwnd{};
    D2D1_SIZE_U m_size{};
    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgi_factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgi_adapter;
    Microsoft::WRL::ComPtr<IDXGIDevice1> dxgi_device;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgi_swapchain;
    Microsoft::WRL::ComPtr<IDXGISurface1> dxgi_surface;

    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_dc;
    Microsoft::WRL::ComPtr<ID3D11Resource> d3d11_surface;
    Microsoft::WRL::ComPtr<ID3D11Resource> d3d11_front_buffer;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_gdi_tex;

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2d1_bitmap;
    Microsoft::WRL::ComPtr<ID2D1Factory3> d2d_factory;
    Microsoft::WRL::ComPtr<ID2D1Device2> d2d_device;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext2> d2d_dc;

    Microsoft::WRL::ComPtr<IDCompositionVisual> comp_visual;
    Microsoft::WRL::ComPtr<IDCompositionDevice> comp_device;
    Microsoft::WRL::ComPtr<IDCompositionTarget> comp_target;
};
