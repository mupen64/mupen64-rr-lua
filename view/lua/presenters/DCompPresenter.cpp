#include "DCompPresenter.h"

#include <view/helpers/WinHelpers.h>

DCompPresenter::~DCompPresenter() {
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

bool DCompPresenter::init(HWND hwnd) {
  RECT rect{};
  GetClientRect(hwnd, &rect);
  m_size = {(UINT32)rect.right - rect.left, (UINT32)rect.bottom - rect.top};

  // Create the whole DComp <-> DXGI <-> D3D11 <-> D2D shabang
  if (!create_composition_surface(
          hwnd, m_size, &m_factory, &m_dxgiadapter, &m_d3device, &m_dxdevice,
          &m_bitmap, &m_comp_visual, &m_comp_device, &m_comp_target,
          &m_dxgi_swapchain, &m_d2d_factory, &m_d2d_device, &m_d3d_dc,
          &m_d2d_dc, &m_dxgi_surface, &m_dxgi_surface_resource, &m_front_buffer,
          &m_d3d_gdi_tex)) {
    return false;
  }

  return true;
}

ID2D1RenderTarget *DCompPresenter::dc() const { return m_d2d_dc; }

void DCompPresenter::begin_present() {
  m_d2d_dc->BeginDraw();
  m_d3d_dc->CopyResource(m_dxgi_surface_resource, m_front_buffer);
  m_d2d_dc->SetTransform(D2D1::Matrix3x2F::Identity());
}

void DCompPresenter::end_present() {
  m_d2d_dc->EndDraw();
  m_dxgi_swapchain->Present(0, 0);
  m_comp_device->Commit();
}

void DCompPresenter::blit(HDC hdc, RECT rect) {
  // 1. Get an ID3D11Resource from our IDXGISurface1 to use as the source for
  // CopySubresourceRegion
  HDC dc;
  ID3D11Resource *rc;
  m_dxgi_surface->QueryInterface(&rc);

  // 2. Get an ID3D11DeviceContext from our ID3D11Device and use it to blit the
  // source (our dxgi surface, so the front buffer) to the GDI-compatible D3D
  // texture
  ID3D11DeviceContext *ctx;
  m_d3device->GetImmediateContext(&ctx);
  ctx->CopySubresourceRegion(m_d3d_gdi_tex, 0, 0, 0, 0, rc, 0, nullptr);

  // 3. Get our GDI-compatible D3D texture's DC
  IDXGISurface1 *dxgi_surface;
  m_d3d_gdi_tex->QueryInterface(&dxgi_surface);
  dxgi_surface->GetDC(false, &dc);

  // 4. Blit our texture DC to the target DC, while preserving the alpha
  // channel(!!!) with the alpha-aware AlphaBlend
  AlphaBlend(hdc, 0, 0, m_size.width, m_size.height, dc, 0, 0, m_size.width,
             m_size.height,
             {.BlendOp = AC_SRC_OVER,
              .BlendFlags = 0,
              .SourceConstantAlpha = 255,
              .AlphaFormat = AC_SRC_ALPHA});

  // 5. Cleanup
  dxgi_surface->ReleaseDC(nullptr);
  ctx->Release();
  dxgi_surface->Release();
  rc->Release();
}

D2D1_SIZE_U DCompPresenter::size() { return m_size; }
