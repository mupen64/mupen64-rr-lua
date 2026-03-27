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
    ID2D1RenderTarget *add_rt() override;
    void remove_rt(const ID2D1RenderTarget *rt) override;
    void begin_present() override;
    void end_present() override;
    void blit(HDC hdc, RECT rect) override;
    D2D1_SIZE_U size() override;

  private:
    struct RenderTarget
    {
        Microsoft::WRL::ComPtr<ID2D1DeviceContext> device_context;
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2d_bitmap;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d_texture;
        Microsoft::WRL::ComPtr<IDXGISurface1> dxgi_surface;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> comp_swapchain;
        Microsoft::WRL::ComPtr<IDCompositionVisual> comp_visual;
    };

    struct CompositionContext
    {
        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgi_factory;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgi_adapter;
        Microsoft::WRL::ComPtr<IDXGIDevice1> dxgi_device;
        Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_dc;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_gdi_tex;
        Microsoft::WRL::ComPtr<ID2D1Factory3> d2d_factory;
        Microsoft::WRL::ComPtr<ID2D1Device2> d2d_device;
        Microsoft::WRL::ComPtr<IDCompositionVisual> comp_visual;
        Microsoft::WRL::ComPtr<IDCompositionDevice> comp_device;
        Microsoft::WRL::ComPtr<IDCompositionTarget> comp_target;
    };

    D2D1_SIZE_U m_size{};
    HWND m_hwnd{};
    CompositionContext m_cmp{};
    std::vector<std::unique_ptr<RenderTarget>> m_rts;
};
