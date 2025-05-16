#include "stdafx.h"
#include <DialogService.h>
#include <components/Statusbar.h>
#include <lua/LuaConsole.h>
#include <lua/LuaRenderer.h>
#include <lua/presenters/DCompPresenter.h>
#include <lua/presenters/GDIPresenter.h>
#include <lua/presenters/Presenter.h>
#include <lua/LuaCallbacks.h>

const auto D2D_OVERLAY_CLASS = L"lua_d2d_overlay";
const auto GDI_OVERLAY_CLASS = L"lua_gdi_overlay";

static bool d2d_drawing = false;

static LRESULT CALLBACK d2d_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_PAINT:
        {
            // NOTE: Sometimes, this control receives a WM_PAINT message while execution is already in WM_PAINT, causing us to call begin_present twice in a row...
            // Usually this shouldn't happen, but the shell file dialog API causes this by messing with the parent window's message loop.
            if (d2d_drawing)
            {
                g_view_logger->warn("Tried to clobber a D2D drawing section!");
                break;
            }

            d2d_drawing = true;

            auto lua = (t_lua_environment*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            if (!lua)
            {
                d2d_drawing = false;
                break;
            }

            bool success;
            if (!lua->rctx.presenter)
            {
                // NOTE: We have to invoke the callback because we're waiting for the script to issue a d2d call
                success = LuaCallbacks::invoke_callbacks_with_key(*lua, LuaCallbacks::REG_ATDRAWD2D);
            }
            else
            {
                lua->rctx.presenter->begin_present();
                success = LuaCallbacks::invoke_callbacks_with_key(*lua, LuaCallbacks::REG_ATDRAWD2D);
                lua->rctx.presenter->end_present();
            }

            ValidateRect(hwnd, nullptr);
            d2d_drawing = false;
            
            if (!success)
            {
                destroy_lua_environment(lua);
            }

            return 0;
        }
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK gdi_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_PAINT:
        {
            auto lua = (t_lua_environment*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            if (!lua)
            {
                break;
            }

            const bool success = LuaCallbacks::invoke_callbacks_with_key(*lua, LuaCallbacks::REG_ATUPDATESCREEN);

            BitBlt(lua->rctx.gdi_front_dc, 0, 0, lua->rctx.dc_size.width, lua->rctx.dc_size.height, lua->rctx.gdi_back_dc, 0, 0, SRCCOPY);

            ValidateRect(hwnd, nullptr);

            if (!success)
            {
                destroy_lua_environment(lua);
            }

            return 0;
        }
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void LuaRenderer::init()
{
    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.lpfnWndProc = (WNDPROC)d2d_overlay_wndproc;
    wndclass.hInstance = g_app_instance;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = D2D_OVERLAY_CLASS;
    RegisterClass(&wndclass);

    wndclass.lpfnWndProc = (WNDPROC)gdi_overlay_wndproc;
    wndclass.lpszClassName = GDI_OVERLAY_CLASS;
    RegisterClass(&wndclass);
}

static void create_loadscreen(t_lua_rendering_context* ctx)
{
    if (ctx->loadscreen_dc)
    {
        return;
    }
    auto gdi_dc = GetDC(g_main_hwnd);
    ctx->loadscreen_dc = CreateCompatibleDC(gdi_dc);
    ctx->loadscreen_bmp = CreateCompatibleBitmap(gdi_dc, ctx->dc_size.width, ctx->dc_size.height);
    SelectObject(ctx->loadscreen_dc, ctx->loadscreen_bmp);
    ReleaseDC(g_main_hwnd, gdi_dc);
}

static void destroy_loadscreen(t_lua_rendering_context* ctx)
{
    if (!ctx->loadscreen_dc)
    {
        return;
    }
    SelectObject(ctx->loadscreen_dc, nullptr);
    DeleteDC(ctx->loadscreen_dc);
    DeleteObject(ctx->loadscreen_bmp);
    ctx->loadscreen_dc = nullptr;
}

t_lua_rendering_context LuaRenderer::default_rendering_context()
{
    t_lua_rendering_context ctx{};
    ctx.brush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    ctx.pen = static_cast<HPEN>(GetStockObject(BLACK_PEN));
    ctx.font = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));
    ctx.col = ctx.bkcol = 0;
    ctx.bkmode = TRANSPARENT;
    return ctx;
}

void LuaRenderer::invalidate_visuals()
{
    assert(is_on_gui_thread());

    for (const auto& lua : g_lua_environments)
    {
        RedrawWindow(lua->rctx.d2d_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE);
        RedrawWindow(lua->rctx.gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE);
    }
}

void LuaRenderer::repaint_visuals()
{
    assert(is_on_gui_thread());

    for (const auto& lua : g_lua_environments)
    {
        RedrawWindow(lua->rctx.d2d_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        RedrawWindow(lua->rctx.gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void LuaRenderer::create_renderer(t_lua_rendering_context* ctx, t_lua_environment* env)
{
    if (ctx->gdi_back_dc != nullptr || ctx->ignore_create_renderer)
    {
        return;
    }

    g_view_logger->info("Creating multi-target renderer for Lua...");

    RECT window_rect;
    GetClientRect(g_main_hwnd, &window_rect);
    if (Statusbar::hwnd())
    {
        // We don't want to paint over statusbar
        RECT rc{};
        GetWindowRect(Statusbar::hwnd(), &rc);
        window_rect.bottom -= (WORD)(rc.bottom - rc.top);
    }

    // NOTE: We don't want negative or zero size on any axis, as that messes up comp surface creation
    ctx->dc_size = {(UINT32)std::max(1, (int32_t)window_rect.right), (UINT32)std::max(1, (int32_t)window_rect.bottom)};
    g_view_logger->info("Lua dc size: {} {}", ctx->dc_size.width, ctx->dc_size.height);

    // Key 0 is reserved for clearing the image pool, too late to change it now...
    ctx->image_pool_index = 1;

    auto gdi_dc = GetDC(g_main_hwnd);
    ctx->gdi_back_dc = CreateCompatibleDC(gdi_dc);
    ctx->gdi_bmp = CreateCompatibleBitmap(gdi_dc, ctx->dc_size.width, ctx->dc_size.height);
    SelectObject(ctx->gdi_back_dc, ctx->gdi_bmp);
    ReleaseDC(g_main_hwnd, gdi_dc);

    ctx->gdi_overlay_hwnd = CreateWindowEx(WS_EX_LAYERED, GDI_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, ctx->dc_size.width, ctx->dc_size.height, g_main_hwnd, nullptr, g_app_instance, nullptr);
    SetLayeredWindowAttributes(ctx->gdi_overlay_hwnd, LUA_GDI_COLOR_MASK, 0, LWA_COLORKEY);

    ctx->gdi_front_dc = GetDC(ctx->gdi_overlay_hwnd);

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    FillRect(ctx->gdi_back_dc, &window_rect, g_alpha_mask_brush);

    ctx->d2d_overlay_hwnd = CreateWindowEx(WS_EX_LAYERED, D2D_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, ctx->dc_size.width, ctx->dc_size.height, g_main_hwnd, nullptr, g_app_instance, nullptr);

    SetWindowLongPtr(ctx->d2d_overlay_hwnd, GWLP_USERDATA, (LONG_PTR)env);
    SetWindowLongPtr(ctx->gdi_overlay_hwnd, GWLP_USERDATA, (LONG_PTR)env);

    if (!g_config.lazy_renderer_init)
    {
        LuaRenderer::ensure_d2d_renderer_created(ctx);
    }

    create_loadscreen(ctx);
}

void LuaRenderer::pre_destroy_renderer(t_lua_rendering_context* ctx)
{
    g_view_logger->info("Pre-destroying Lua renderer...");
    ctx->ignore_create_renderer = true;
    SetWindowLongPtr(ctx->gdi_overlay_hwnd, GWLP_USERDATA, 0);
    SetWindowLongPtr(ctx->d2d_overlay_hwnd, GWLP_USERDATA, 0);
}

void LuaRenderer::destroy_renderer(t_lua_rendering_context* ctx)
{
    g_view_logger->info("Destroying Lua renderer...");

    SelectObject(ctx->gdi_back_dc, nullptr);
    DeleteObject(ctx->brush);
    DeleteObject(ctx->pen);
    DeleteObject(ctx->font);

    if (ctx->presenter)
    {
        ctx->dw_text_layouts.clear();
        ctx->dw_text_sizes.clear();

        while (!ctx->d2d_render_target_stack.empty())
        {
            ctx->d2d_render_target_stack.pop();
        }

        for (auto& [_, val] : ctx->image_pool)
        {
            delete val;
        }
        ctx->image_pool.clear();

        DestroyWindow(ctx->d2d_overlay_hwnd);

        delete ctx->presenter;
        ctx->presenter = nullptr;
        CoUninitialize();
    }

    if (ctx->gdi_back_dc)
    {
        ReleaseDC(ctx->gdi_overlay_hwnd, ctx->gdi_front_dc);
        DestroyWindow(ctx->gdi_overlay_hwnd);
        SelectObject(ctx->gdi_back_dc, nullptr);
        DeleteDC(ctx->gdi_back_dc);
        DeleteObject(ctx->gdi_bmp);
        ctx->gdi_back_dc = nullptr;
        destroy_loadscreen(ctx);
    }
}

void LuaRenderer::ensure_d2d_renderer_created(t_lua_rendering_context* ctx)
{
    if (ctx->presenter || ctx->ignore_create_renderer)
    {
        return;
    }

    g_view_logger->trace("[Lua] Creating D2D renderer...");

    auto hr = CoInitialize(nullptr);
    if (hr != S_OK && hr != S_FALSE && hr != RPC_E_CHANGED_MODE)
    {
        DialogService::show_dialog(L"Failed to initialize COM.\r\nVerify that your system is up-to-date.", L"Lua", fsvc_error);
        return;
    }

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(ctx->dw_factory), reinterpret_cast<IUnknown**>(&ctx->dw_factory));

    if (g_config.presenter_type != (int32_t)t_config::PresenterType::GDI)
    {
        ctx->presenter = new DCompPresenter();
    }
    else
    {
        ctx->presenter = new GDIPresenter();
    }

    if (!ctx->presenter->init(ctx->d2d_overlay_hwnd))
    {
        DialogService::show_dialog(L"Failed to initialize presenter.\r\nVerify that your system supports the selected presenter.", L"Lua", fsvc_error);
        return;
    }

    ctx->d2d_render_target_stack.push(ctx->presenter->dc());
    ctx->dw_text_layouts = MicroLRU::Cache<uint64_t, IDWriteTextLayout*>(512, [&](auto value) {
        value->Release();
    });
    ctx->dw_text_sizes = MicroLRU::Cache<uint64_t, DWRITE_TEXT_METRICS>(512, [&](auto value) {
    });
}

void LuaRenderer::ensure_gdi_renderer_created(t_lua_rendering_context* ctx)
{
}

void LuaRenderer::loadscreen_reset(t_lua_rendering_context* ctx)
{
    destroy_loadscreen(ctx);
    create_loadscreen(ctx);
}
