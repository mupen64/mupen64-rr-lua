#include "stdafx.h"
#include <DialogService.h>
#include <components/Statusbar.h>
#include <lua/LuaManager.h>
#include <lua/LuaRenderer.h>
#include <lua/presenters/DCompPresenter.h>
#include <lua/presenters/GDIPresenter.h>
#include <lua/presenters/Presenter.h>
#include <lua/LuaCallbacks.h>

const auto D2D_OVERLAY_CLASS = L"lua_d2d_overlay";
const auto GDI_OVERLAY_CLASS = L"lua_gdi_overlay";
const auto CTX_PROP = L"lua_ctx";

static bool d2d_drawing = false;
static HBRUSH g_alpha_mask_brush;

LuaRendererContext g_rctx;

static LRESULT CALLBACK d2d_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    RT_ASSERT(is_on_gui_thread(), L"LuaRenderer::d2d_overlay_wndproc called from non-GUI thread");

    switch (msg)
    {
    case WM_PAINT: {
        // NOTE: Sometimes, this control receives a WM_PAINT message while execution is already in WM_PAINT, causing us
        // to call begin_present twice in a row... Usually this shouldn't happen, but the shell file dialog API causes
        // this by messing with the parent window's message loop.
        if (d2d_drawing)
        {
            g_view_logger->warn("Tried to clobber a D2D drawing section!");
            break;
        }

        d2d_drawing = true;

        bool success;
        g_rctx.presenter->begin_present();
        for (const auto &env : g_lua_environments)
        {
            success = LuaCallbacks::invoke_callbacks_with_key(env, LuaCallbacks::REG_ATDRAWD2D);
        }
        g_rctx.presenter->end_present();

        ValidateRect(hwnd, nullptr);
        d2d_drawing = false;

        return 0;
    }
    case WM_NCDESTROY:
        RemoveProp(hwnd, CTX_PROP);
        break;
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK gdi_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_PAINT: {
        auto lua = (t_lua_environment *)GetProp(hwnd, CTX_PROP);

        if (!lua)
        {
            break;
        }

        const bool success = LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATUPDATESCREEN);

        if (lua->rctx.has_gdi_content)
        {
            BitBlt(lua->rctx.gdi_front_dc, 0, 0, g_rctx.dc_size.width, g_rctx.dc_size.height, lua->rctx.gdi_back_dc, 0,
                   0, SRCCOPY);
        }

        ValidateRect(hwnd, nullptr);

        if (!success)
        {
            LuaManager::destroy_environment(lua);
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
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = D2D_OVERLAY_CLASS;
    RegisterClass(&wndclass);

    wndclass.lpfnWndProc = (WNDPROC)gdi_overlay_wndproc;
    wndclass.lpszClassName = GDI_OVERLAY_CLASS;
    RegisterClass(&wndclass);

    g_alpha_mask_brush = CreateSolidBrush(LUA_GDI_COLOR_MASK);
}

void LuaRenderer::resize(uint32_t width, uint32_t height)
{
    width = std::max(width, 1u);
    height = std::max(height, 1u);

    if (g_rctx.dc_size.width == width && g_rctx.dc_size.height == height) return;

    g_rctx.dc_size = {width, height};

    if (!IsWindow(g_rctx.d2d_overlay_hwnd))
    {
        auto hr = CoInitialize(nullptr);
        RT_ASSERT(hr == S_OK || hr == RPC_E_CHANGED_MODE, L"Failed to initialize COM.");

        g_rctx.d2d_overlay_hwnd =
            CreateWindowEx(WS_EX_LAYERED, D2D_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, g_rctx.dc_size.width,
                           g_rctx.dc_size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);
        const auto error = GetLastError();

        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(g_rctx.dw_factory),
                            reinterpret_cast<IUnknown **>(&g_rctx.dw_factory));

        g_rctx.dw_text_layouts =
            MicroLRU::Cache<uint64_t, IDWriteTextLayout *>(512, [&](auto value) { value->Release(); });
        g_rctx.dw_text_sizes = MicroLRU::Cache<uint64_t, DWRITE_TEXT_METRICS>(512, [&](auto value) {});
    }

    SetWindowPos(g_rctx.d2d_overlay_hwnd, nullptr, 0, 0, g_rctx.dc_size.width, g_rctx.dc_size.height,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    if (g_rctx.presenter)
    {
        delete g_rctx.presenter;
        g_rctx.presenter = nullptr;
    }

    if (g_config.presenter_type != (int32_t)t_config::PresenterType::GDI)
        g_rctx.presenter = new DCompPresenter();
    else
        g_rctx.presenter = new GDIPresenter();

    const auto result = g_rctx.presenter->init(g_rctx.d2d_overlay_hwnd);
    RT_ASSERT(result,
              L"Failed to initialize Lua presenter.\r\nVerify that your system supports the selected presenter.");

    for (const auto &env : g_lua_environments)
    {
        env->rctx.d2d_render_target_stack = {};
        env->rctx.d2d_render_target_stack.push(g_rctx.presenter->dc());
        BringWindowToTop(env->rctx.gdi_overlay_hwnd);
    }

    BringWindowToTop(g_rctx.d2d_overlay_hwnd);
}

static void create_loadscreen(t_lua_rendering_context *ctx)
{
    if (ctx->loadscreen_dc)
    {
        return;
    }
    auto gdi_dc = GetDC(g_main_ctx.hwnd);
    ctx->loadscreen_dc = CreateCompatibleDC(gdi_dc);
    ctx->loadscreen_bmp = CreateCompatibleBitmap(gdi_dc, g_rctx.dc_size.width, g_rctx.dc_size.height);
    SelectObject(ctx->loadscreen_dc, ctx->loadscreen_bmp);
    ReleaseDC(g_main_ctx.hwnd, gdi_dc);
}

static void destroy_loadscreen(t_lua_rendering_context *ctx)
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

static void CALLBACK invalidate_callback(UINT, UINT, DWORD_PTR user, DWORD_PTR, DWORD_PTR)
{
    const auto hwnd = (HWND)user;
    if (IsWindow(hwnd)) RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
}

static void stop_invalidation_timers(t_lua_rendering_context *ctx)
{
    timeKillEvent(ctx->d2d_timer);
    timeKillEvent(ctx->gdi_timer);
}

static void restart_invalidation_timers(t_lua_rendering_context *ctx)
{
    stop_invalidation_timers(ctx);

    const auto fps = ctx->target_fps.value_or(1000.0f);
    const auto ms = (UINT)std::round(1000.0f / fps);

    ctx->d2d_timer = timeSetEvent(ms, 1, invalidate_callback, (DWORD_PTR)g_rctx.d2d_overlay_hwnd,
                                  TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
    ctx->gdi_timer = timeSetEvent(ms, 1, invalidate_callback, (DWORD_PTR)ctx->gdi_overlay_hwnd,
                                  TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
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

void LuaRenderer::repaint_visuals()
{
    assert(is_on_gui_thread());

    for (const auto &lua : g_lua_environments)
    {
        RedrawWindow(lua->rctx.gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        RedrawWindow(g_rctx.d2d_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void LuaRenderer::create_renderer(t_lua_rendering_context *ctx, t_lua_environment *env)
{
    if (ctx->gdi_back_dc != nullptr || ctx->ignore_create_renderer)
    {
        return;
    }

    g_view_logger->info("Creating multi-target renderer for Lua...");

    // Key 0 is reserved for clearing the image pool, too late to change it now...
    ctx->image_pool_index = 1;

    auto gdi_dc = GetDC(g_main_ctx.hwnd);
    ctx->gdi_back_dc = CreateCompatibleDC(gdi_dc);
    ctx->gdi_bmp = CreateCompatibleBitmap(gdi_dc, g_rctx.dc_size.width, g_rctx.dc_size.height);
    SelectObject(ctx->gdi_back_dc, ctx->gdi_bmp);
    ReleaseDC(g_main_ctx.hwnd, gdi_dc);

    ctx->gdi_overlay_hwnd =
        CreateWindowEx(WS_EX_LAYERED, GDI_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, g_rctx.dc_size.width,
                       g_rctx.dc_size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);
    SetLayeredWindowAttributes(ctx->gdi_overlay_hwnd, LUA_GDI_COLOR_MASK, 0, LWA_COLORKEY);

    ctx->gdi_front_dc = GetDC(ctx->gdi_overlay_hwnd);

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    FillRect(ctx->gdi_back_dc, &g_rctx.window_rect, g_alpha_mask_brush);

    // Bring the windows to top so they are above the MGE compositor
    BringWindowToTop(ctx->gdi_overlay_hwnd);
    BringWindowToTop(g_rctx.d2d_overlay_hwnd);

    SetProp(ctx->gdi_overlay_hwnd, CTX_PROP, env);

    ctx->d2d_render_target_stack.push(g_rctx.presenter->dc());

    if (!g_config.lazy_renderer_init)
    {
        ensure_d2d_renderer_created(ctx);
        mark_gdi_content_present(ctx);
    }

    create_loadscreen(ctx);
    restart_invalidation_timers(ctx);
}

void LuaRenderer::pre_destroy_renderer(t_lua_rendering_context *ctx)
{
    g_view_logger->info("Pre-destroying Lua renderer...");
    ctx->ignore_create_renderer = true;
    SetProp(ctx->gdi_overlay_hwnd, CTX_PROP, nullptr);
    stop_invalidation_timers(ctx);
}

void LuaRenderer::destroy_renderer(t_lua_rendering_context *ctx)
{
    g_view_logger->info("Destroying Lua renderer...");

    SelectObject(ctx->gdi_back_dc, nullptr);
    DeleteObject(ctx->brush);
    DeleteObject(ctx->pen);
    DeleteObject(ctx->font);

    for (const auto bmp : ctx->image_pool | std::views::values)
    {
        delete bmp;
    }

    ctx->image_pool.clear();
    ctx->d2d_render_target_stack = {};

    if (ctx->gdi_back_dc)
    {
        SetProp(ctx->gdi_overlay_hwnd, CTX_PROP, nullptr);

        ReleaseDC(ctx->gdi_overlay_hwnd, ctx->gdi_front_dc);
        DestroyWindow(ctx->gdi_overlay_hwnd);
        SelectObject(ctx->gdi_back_dc, nullptr);
        DeleteDC(ctx->gdi_back_dc);
        DeleteObject(ctx->gdi_bmp);
        ctx->gdi_back_dc = nullptr;
        destroy_loadscreen(ctx);
    }
}

void LuaRenderer::ensure_d2d_renderer_created(t_lua_rendering_context *ctx)
{
}

void LuaRenderer::mark_gdi_content_present(t_lua_rendering_context *ctx)
{
    ctx->has_gdi_content = true;
}

void LuaRenderer::loadscreen_reset(t_lua_rendering_context *ctx)
{
    destroy_loadscreen(ctx);
    create_loadscreen(ctx);
}

void LuaRenderer::set_target_fps(t_lua_rendering_context *rctx, std::optional<float> fps)
{
    if (rctx->target_fps == fps) return;
    if (fps.has_value())
    {
        if (!std::isfinite(fps.value()) || fps.value() <= 0.0f) return;
    }

    rctx->target_fps = fps;
    restart_invalidation_timers(rctx);
}

HBRUSH LuaRenderer::alpha_mask_brush()
{
    return g_alpha_mask_brush;
}
