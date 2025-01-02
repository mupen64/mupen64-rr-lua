#pragma once
#include "Presenter.h"

#include <d2d1.h>
#include <d2d1_3.h>
#include <d3d11.h>
#include <dcomp.h>

class DCompPresenter : public Presenter {
public:
    ~DCompPresenter() override;
    bool init(HWND hwnd) override;
    ID2D1RenderTarget* dc() const override;
    void begin_present() override;
    void end_present() override;
    void blit(HDC hdc, RECT rect) override;
    D2D1_SIZE_U size() override;

private:
    D2D1_SIZE_U m_size{};
    IDXGIFactory2* m_factory = nullptr;
    IDXGIAdapter1* m_dxgiadapter = nullptr;
    ID3D11Device* m_d3device = nullptr;
    IDXGIDevice1* m_dxdevice = nullptr;
    ID2D1Bitmap1* m_bitmap = nullptr;
    IDCompositionVisual* m_comp_visual = nullptr;
    ID3D11DeviceContext* m_d3d_dc = nullptr;
    IDCompositionDevice* m_comp_device = nullptr;
    IDCompositionTarget* m_comp_target = nullptr;
    IDXGISwapChain1* m_dxgi_swapchain = nullptr;
    ID2D1Factory3* m_d2d_factory = nullptr;
    ID2D1Device2* m_d2d_device = nullptr;
    ID2D1DeviceContext2* m_d2d_dc = nullptr;
    IDXGISurface1* m_dxgi_surface = nullptr;
    ID3D11Resource* m_dxgi_surface_resource = nullptr;
    ID3D11Resource* m_front_buffer = nullptr;
    ID3D11Texture2D* m_d3d_gdi_tex = nullptr;
};
