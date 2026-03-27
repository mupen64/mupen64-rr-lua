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
    ~GDIPresenter() override;
    bool init(HWND hwnd) override;
    ID2D1RenderTarget *add_rt() override;
    void remove_rt(const ID2D1RenderTarget *rt) override;
    D2D1_SIZE_U size() override;
    void begin_present() override;
    void end_present() override;
    void blit(HDC hdc, RECT rect) override;
    D2D1::ColorF adjust_clear_color(D2D1::ColorF color) const override;

  private:
    struct RenderTargetContext
    {
        ID2D1DCRenderTarget *render_target;
        HDC gdi_dc;
        HBITMAP gdi_bmp;
    };

    D2D1_SIZE_U m_size{};
    HWND m_hwnd = nullptr;
    ID2D1Factory *m_d2d_factory = nullptr;
    std::vector<RenderTargetContext> m_render_target_contexts;
};
