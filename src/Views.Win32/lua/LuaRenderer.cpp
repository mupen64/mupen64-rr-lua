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

        // FIXME: How do we destroy the environments that errored out?

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
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        bool success;
        for (const auto &env : g_lua_environments)
        {
            success = LuaCallbacks::invoke_callbacks_with_key(env, LuaCallbacks::REG_ATUPDATESCREEN);
        }

        // FIXME: We should have real transparency for the GDI ovleray.
        RECT rc = {0, 0, (LONG)g_rctx.size.width, (LONG)g_rctx.size.height};
        FillRect(hdc, &rc, g_alpha_mask_brush);

        for (const auto &env : g_lua_environments)
        {
            if (!env->rctx.has_gdi_content) continue;
            TransparentBlt(hdc, 0, 0, g_rctx.size.width, g_rctx.size.height, env->rctx.gdi_rt.dc, 0, 0,
                           g_rctx.size.width, g_rctx.size.height, LuaRenderer::LUA_GDI_COLOR_MASK);
        }

        // FIXME: How do we destroy the environments that errored out?

        EndPaint(hwnd, &ps);
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
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
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

static void create_loadscreen(t_lua_rendering_context *ctx)
{
    if (ctx->loadscreen_dc)
    {
        return;
    }
    auto gdi_dc = GetDC(g_main_ctx.hwnd);
    ctx->loadscreen_dc = CreateCompatibleDC(gdi_dc);
    ctx->loadscreen_bmp = CreateCompatibleBitmap(gdi_dc, g_rctx.size.width, g_rctx.size.height);
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

static void create_d2d_renderer_if_needed()
{
    if (IsWindow(g_rctx.d2d_overlay_hwnd)) return;

    auto hr = CoInitialize(nullptr);
    RT_ASSERT(hr == S_OK || hr == RPC_E_CHANGED_MODE, L"Failed to initialize COM.");

    g_rctx.d2d_overlay_hwnd =
        CreateWindowEx(WS_EX_LAYERED, D2D_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, g_rctx.size.width,
                       g_rctx.size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);
    RT_ASSERT(IsWindow(g_rctx.d2d_overlay_hwnd), L"Failed to create D2D overlay window.");

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(g_rctx.dw_factory),
                        reinterpret_cast<IUnknown **>(&g_rctx.dw_factory));

    g_rctx.dw_text_layouts = MicroLRU::Cache<uint64_t, IDWriteTextLayout *>(512, [&](auto value) { value->Release(); });
    g_rctx.dw_text_sizes = MicroLRU::Cache<uint64_t, DWRITE_TEXT_METRICS>(512, [&](auto value) {});

    // FIXME: How do we handle presenter_type changing if this is only ran once at startup? :P
    if (g_config.presenter_type != (int32_t)t_config::PresenterType::GDI)
        g_rctx.presenter = new DCompPresenter();
    else
        g_rctx.presenter = new GDIPresenter();

    const auto result = g_rctx.presenter->init(g_rctx.d2d_overlay_hwnd);
    RT_ASSERT(result,
              L"Failed to initialize Lua presenter.\r\nVerify that your system supports the selected presenter.");
}

static void create_gdi_renderer_if_needed()
{
    if (IsWindow(g_rctx.gdi_overlay_hwnd)) return;

    g_rctx.gdi_overlay_hwnd =
        CreateWindowEx(WS_EX_LAYERED, GDI_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, g_rctx.size.width,
                       g_rctx.size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);
    RT_ASSERT(IsWindow(g_rctx.gdi_overlay_hwnd), L"Failed to create GDI overlay window.");

    SetLayeredWindowAttributes(g_rctx.gdi_overlay_hwnd, LuaRenderer::LUA_GDI_COLOR_MASK, 0, LWA_COLORKEY);
}

void LuaRenderer::resize(uint32_t width, uint32_t height)
{
    width = std::max(width, 1u);
    height = std::max(height, 1u);

    if (g_rctx.size.width == width && g_rctx.size.height == height) return;

    g_rctx.size = {width, height};

    create_d2d_renderer_if_needed();
    create_gdi_renderer_if_needed();

    // Resize D2D window and RTs.
    SetWindowPos(g_rctx.d2d_overlay_hwnd, nullptr, 0, 0, g_rctx.size.width, g_rctx.size.height,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    g_rctx.presenter->resize(g_rctx.size);

    // Resize GDI window and RTs.
    SetWindowPos(g_rctx.gdi_overlay_hwnd, nullptr, 0, 0, g_rctx.size.width, g_rctx.size.height,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    // Pretty slow... is there a quicker way?
    for (const auto &env : g_lua_environments)
    {
        if (!env->rctx.gdi_rt.dc) return;

        // Destroy GDI render target
        SelectObject(env->rctx.gdi_rt.dc, nullptr);
        DeleteDC(env->rctx.gdi_rt.dc);
        DeleteObject(env->rctx.gdi_rt.bmp);
        env->rctx.gdi_rt.dc = nullptr;
        env->rctx.gdi_rt.bmp = nullptr;
        destroy_loadscreen(&env->rctx);

        // Recreate GDI render target
        auto gdi_dc = GetDC(g_main_ctx.hwnd);
        env->rctx.gdi_rt.dc = CreateCompatibleDC(gdi_dc);
        env->rctx.gdi_rt.bmp = CreateCompatibleBitmap(gdi_dc, g_rctx.size.width, g_rctx.size.height);
        SelectObject(env->rctx.gdi_rt.dc, env->rctx.gdi_rt.bmp);
        ReleaseDC(g_main_ctx.hwnd, gdi_dc);
        const RECT rc = {0, 0, (LONG)g_rctx.size.width, (LONG)g_rctx.size.height};
        FillRect(env->rctx.gdi_rt.dc, &rc, g_alpha_mask_brush);
    }
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

// FIXME: Consider that we have only two windows...
static void restart_invalidation_timers(t_lua_rendering_context *ctx)
{
    stop_invalidation_timers(ctx);

    const auto fps = ctx->target_fps.value_or(1000.0f);
    const auto ms = (UINT)std::round(1000.0f / fps);

    ctx->d2d_timer = timeSetEvent(ms, 1, invalidate_callback, (DWORD_PTR)g_rctx.d2d_overlay_hwnd,
                                  TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
    ctx->gdi_timer = timeSetEvent(ms, 1, invalidate_callback, (DWORD_PTR)g_rctx.gdi_overlay_hwnd,
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
        RedrawWindow(g_rctx.gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        RedrawWindow(g_rctx.d2d_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void LuaRenderer::create_renderer(t_lua_rendering_context *ctx, t_lua_environment *env)
{
    if (ctx->gdi_rt.dc || ctx->ignore_create_renderer) return;

    g_view_logger->info("Creating multi-target renderer for Lua...");

    // Key 0 is reserved for clearing the image pool, too late to change it now...
    ctx->image_pool_index = 1;

    auto gdi_dc = GetDC(g_main_ctx.hwnd);
    ctx->gdi_rt.dc = CreateCompatibleDC(gdi_dc);
    ctx->gdi_rt.bmp = CreateCompatibleBitmap(gdi_dc, g_rctx.size.width, g_rctx.size.height);
    SelectObject(ctx->gdi_rt.dc, ctx->gdi_rt.bmp);
    ReleaseDC(g_main_ctx.hwnd, gdi_dc);

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    const RECT rc = {0, 0, (LONG)g_rctx.size.width, (LONG)g_rctx.size.height};
    FillRect(ctx->gdi_rt.dc, &rc, g_alpha_mask_brush);

    const auto dc = g_rctx.presenter->add_rt();
    ctx->d2d_rts.emplace_back(dc);

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
    stop_invalidation_timers(ctx);
}

void LuaRenderer::destroy_renderer(t_lua_rendering_context *ctx)
{
    g_view_logger->info("Destroying Lua renderer...");

    if (ctx->gdi_rt.dc)
    {
        SelectObject(ctx->gdi_rt.dc, nullptr);

        DeleteObject(ctx->brush);
        DeleteObject(ctx->pen);
        DeleteObject(ctx->font);

        DeleteDC(ctx->gdi_rt.dc);
        DeleteObject(ctx->gdi_rt.bmp);
        ctx->gdi_rt.dc = nullptr;
        ctx->gdi_rt.bmp = nullptr;

        destroy_loadscreen(ctx);
    }

    for (const auto bmp : ctx->image_pool | std::views::values)
    {
        delete bmp;
    }
    ctx->image_pool.clear();

    RT_ASSERT(ctx->d2d_rts.size() == 1, L"Unexpected D2D render target stack size during renderer "
                                        L"destruction. Was this called during `d2d.draw_to_image`?");
    g_rctx.presenter->remove_rt(ctx->d2d_rts[0]);
    ctx->d2d_rts.erase(ctx->d2d_rts.begin());
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
