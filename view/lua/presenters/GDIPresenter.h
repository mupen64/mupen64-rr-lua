#pragma once
#include "Presenter.h"

class GDIPresenter : public Presenter {
public:
  ~GDIPresenter() override;
  bool init(HWND hwnd) override;
  ID2D1RenderTarget *dc() const override;
  D2D1_SIZE_U size() override;
  void begin_present() override;
  void end_present() override;
  void blit(HDC hdc, RECT rect) override;
  D2D1::ColorF adjust_clear_color(D2D1::ColorF color) const override;

private:
  D2D1_SIZE_U m_size{};
  HWND m_hwnd = nullptr;
  ID2D1Factory *m_d2d_factory = nullptr;
  ID2D1DCRenderTarget *m_d2d_render_target = nullptr;
  HDC m_gdi_back_dc = nullptr;
  HBITMAP m_gdi_bmp = nullptr;
};
