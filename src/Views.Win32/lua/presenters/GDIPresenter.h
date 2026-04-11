/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Presenter.h"

class GDIPresenter : public Presenter
{
  public:
    /**
     * Constructs a GDIPresenter with the given mask color.
     *
     * \param mask_color The color to use as the alpha mask.
     */
    GDIPresenter(COLORREF mask_color) : m_mask_color(mask_color) {};
    ~GDIPresenter() override;
    bool init(HWND hwnd) override;
    ID2D1RenderTarget *dc() const override;
    D2D1_SIZE_U size() override;
    void resize(D2D1_SIZE_U size) override;
    void present() override;
    void blit(HDC hdc, RECT rect) override;

  private:
    D2D1_SIZE_U m_size{};
    HWND m_hwnd = nullptr;
    HDC m_gdi_back_dc = nullptr;
    HBITMAP m_gdi_bmp = nullptr;
    COLORREF m_mask_color{};
    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2d_factory;
    Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> m_d2d_render_target;
};
