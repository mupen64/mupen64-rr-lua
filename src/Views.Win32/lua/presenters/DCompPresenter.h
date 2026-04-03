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
    void create_size_dependent_resources();

    HWND m_hwnd{};
    D2D1_SIZE_U m_size{};
    ComPtr<IDXGIFactory2> m_dxgi_factory;
    ComPtr<IDXGIAdapter1> m_dxgi_adapter;
    ComPtr<IDXGIDevice1> m_dxgi_device;
    ComPtr<IDXGISwapChain1> m_dxgi_swapchain;
    ComPtr<IDXGISurface1> m_dxgi_surface;

    ComPtr<ID3D11Device> m_d3d_device;
    ComPtr<ID3D11DeviceContext> m_d3d_dc;
    ComPtr<ID3D11Texture2D> m_d3d_gdi_tex;

    ComPtr<ID2D1Bitmap1> m_d2d_bitmap;
    ComPtr<ID2D1Factory3> m_d2d_factory;
    ComPtr<ID2D1Device2> m_d2d_device;
    ComPtr<ID2D1DeviceContext2> m_d2d_dc;

    ComPtr<IDCompositionVisual> m_comp_visual;
    ComPtr<IDCompositionDevice> m_comp_device;
    ComPtr<IDCompositionTarget> m_comp_target;
};
