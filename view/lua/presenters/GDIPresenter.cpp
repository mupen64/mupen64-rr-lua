#include "GDIPresenter.h"

#include "view/lua/LuaConsole.h"

GDIPresenter::~GDIPresenter()
{
    SelectObject(m_gdi_back_dc, nullptr);
    DeleteObject(m_gdi_bmp);
    DeleteDC(m_gdi_back_dc);

    m_d2d_factory->Release();
    m_d2d_render_target->Release();
}

bool GDIPresenter::init(HWND hwnd)
{
    m_hwnd = hwnd;

    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_size = {(UINT32)rect.right - rect.left, (UINT32)rect.bottom - rect.top};

    // 1. Create a back-buffer DC pointed to a bitmap
    auto gdi_dc = GetDC(hwnd);
    m_gdi_back_dc = CreateCompatibleDC(gdi_dc);
    m_gdi_bmp = CreateCompatibleBitmap(gdi_dc, m_size.width, m_size.height);
    SelectObject(m_gdi_back_dc, m_gdi_bmp);
    ReleaseDC(hwnd, gdi_dc);

    // 2. Make the provided window (which must have WS_EX_LAYERED) mask out our
    // clear color, then fill the back DC with that color (effectively clearing
    // it)
    SetLayeredWindowAttributes(hwnd, lua_gdi_color_mask, 0, LWA_COLORKEY);
    FillRect(m_gdi_back_dc, &rect, alpha_mask_brush);

    // 3. Create a D2D1 RT and point it to our back DC
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2d_factory);
    m_d2d_factory->CreateDCRenderTarget(&props, &m_d2d_render_target);

    RECT dc_rect = {0, 0, (LONG)m_size.width, (LONG)m_size.height};
    m_d2d_render_target->BindDC(m_gdi_back_dc, &dc_rect);

    return true;
}

ID2D1RenderTarget* GDIPresenter::dc() const { return m_d2d_render_target; }

D2D1_SIZE_U GDIPresenter::size() { return m_size; }

void GDIPresenter::begin_present()
{
    m_d2d_render_target->BeginDraw();
    m_d2d_render_target->SetTransform(D2D1::Matrix3x2F::Identity());
}

void GDIPresenter::end_present()
{
    auto main_dc = GetDC(m_hwnd);

    m_d2d_render_target->EndDraw();
    // We want BitBlt, not TransparentBlt because of mask color preservation.
    // Those are dropped at composite-time and not blit-time.
    BitBlt(main_dc, 0, 0, m_size.width, m_size.height, m_gdi_back_dc, 0, 0, SRCCOPY);

    ReleaseDC(m_hwnd, main_dc);
}

void GDIPresenter::blit(HDC hdc, RECT rect) { TransparentBlt(hdc, 0, 0, m_size.width, m_size.height, m_gdi_back_dc, 0, 0, m_size.width, m_size.height, lua_gdi_color_mask); }

D2D1::ColorF GDIPresenter::adjust_clear_color(const D2D1::ColorF color) const { return D2D1::ColorF(lua_gdi_color_mask); }
