#include "stdafx.h"
#include <DialogService.h>
#include <components/Statusbar.h>
#include <lua/LuaManager.h>
#include <lua/LuaRenderer.h>
#include <lua/presenters/DCompPresenter.h>
#include <lua/presenters/GDIPresenter.h>
#include <lua/presenters/Presenter.h>
#include <lua/LuaCallbacks.h>
#include "LuaRenderer.h"
#include <Messenger.h>

const auto D2D_OVERLAY_CLASS = L"lua_d2d_overlay";
const auto GDI_OVERLAY_CLASS = L"lua_gdi_overlay";
const auto CTX_PROP = L"lua_ctx";

static bool d2d_drawing = false;
static HBRUSH g_alpha_mask_brush;
static MMRESULT render_timer{};

static std::thread draw_thread;
static std::atomic<bool> draw_thread_running{false};

static void draw_lua()
{
    const auto now = std::chrono::steady_clock::now();

    // TODO: Maybe UpdateLayeredWindow here directly?
    for (const auto &lua : g_lua_environments)
    {
        RedrawWindow(lua->rctx.gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE);
    }

    std::vector<t_lua_environment *> to_destroy;
    for (const auto &lua : g_lua_environments)
    {
        const auto time_since_last_render =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lua->rctx.last_render_time).count();

        const auto fps = lua->rctx.target_fps.value_or(1000.0f);
        const auto target_frame_time = 1000.0f / fps;

        if (time_since_last_render < target_frame_time) continue;

        bool success;
        if (!lua->rctx.presenter)
        {
            // NOTE: We have to invoke the callback because we're waiting for the script to issue a d2d call
            success = LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATDRAWD2D);
        }
        else
        {
            lua->rctx.presenter->begin_present();
            success = LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATDRAWD2D);
            lua->rctx.presenter->end_present();
        }

        lua->rctx.last_render_time = now;

        if (!success) to_destroy.push_back(lua);
    }

    for (const auto &lua : to_destroy)
    {
        LuaManager::destroy_environment(lua);
    }
}

static void draw_clock_proc()
{
    while (draw_thread_running)
    {
        g_main_ctx.dispatcher->invoke([]() { draw_lua(); });

        DwmFlush();
    }
}

static void stop_draw_clock()
{
    draw_thread_running = false;
    if (draw_thread.joinable()) draw_thread.join();
}

static void start_draw_clock()
{
    draw_thread_running = true;
    draw_thread = std::thread(draw_clock_proc);
}

static void create_loadscreen(t_lua_rendering_context *ctx)
{
    if (ctx->loadscreen_dc)
    {
        return;
    }
    auto gdi_dc = GetDC(g_main_ctx.hwnd);
    ctx->loadscreen_dc = CreateCompatibleDC(gdi_dc);
    ctx->loadscreen_bmp = CreateCompatibleBitmap(gdi_dc, ctx->dc_size.width, ctx->dc_size.height);
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

static void resize(uint32_t width, uint32_t height)
{
    width = std::max(width, 1u);
    height = std::max(height, 1u);

    for (const auto &lua : g_lua_environments)
    {
        if (lua->rctx.dc_size.width == width && lua->rctx.dc_size.height == height) continue;

        lua->rctx.dc_size = {width, height};
        RECT wnd_rect{0, 0, (LONG)width, (LONG)height};

        HDC gdi_dc = GetDC(g_main_ctx.hwnd);
        HDC new_back_dc = CreateCompatibleDC(gdi_dc);
        HBITMAP new_bmp = CreateCompatibleBitmap(gdi_dc, width, height);
        SelectObject(new_back_dc, new_bmp);
        ReleaseDC(g_main_ctx.hwnd, gdi_dc);
        SelectObject(lua->rctx.gdi_back_dc, nullptr);
        DeleteObject(lua->rctx.gdi_bmp);
        DeleteDC(lua->rctx.gdi_back_dc);
        lua->rctx.gdi_back_dc = new_back_dc;
        lua->rctx.gdi_bmp = new_bmp;

        FillRect(lua->rctx.gdi_back_dc, &wnd_rect, g_alpha_mask_brush);

        destroy_loadscreen(&lua->rctx);
        create_loadscreen(&lua->rctx);

        if (lua->rctx.presenter) lua->rctx.presenter->resize(lua->rctx.dc_size);

        SetWindowPos(lua->rctx.gdi_overlay_hwnd, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE);
        SetWindowPos(lua->rctx.d2d_overlay_hwnd, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE);
    }
}

static LRESULT CALLBACK d2d_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    RT_ASSERT(is_on_gui_thread(), L"LuaRenderer::d2d_overlay_wndproc called from non-GUI thread");

    switch (msg)
    {
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
            BitBlt(lua->rctx.gdi_front_dc, 0, 0, lua->rctx.dc_size.width, lua->rctx.dc_size.height,
                   lua->rctx.gdi_back_dc, 0, 0, SRCCOPY);
        }

        ValidateRect(hwnd, nullptr);

        if (!success)
        {
            LuaManager::destroy_environment(lua);
        }

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

    Messenger::subscribe(Messenger::Message::SizeChanged, [](const std::any &data) {
        auto rect = std::any_cast<RECT>(data);
        resize(rect.right - rect.left, rect.bottom - rect.top);
    });

    start_draw_clock();
}

void LuaRenderer::stop()
{
    stop_draw_clock();
    DeleteObject(g_alpha_mask_brush);
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
        RedrawWindow(lua->rctx.d2d_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        RedrawWindow(lua->rctx.gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void LuaRenderer::create_renderer(t_lua_rendering_context *ctx, t_lua_environment *env)
{
    if (ctx->gdi_back_dc != nullptr || ctx->ignore_create_renderer)
    {
        return;
    }

    g_view_logger->info("Creating multi-target renderer for Lua...");

    RECT window_rect;
    GetClientRect(g_main_ctx.hwnd, &window_rect);
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

    auto gdi_dc = GetDC(g_main_ctx.hwnd);
    ctx->gdi_back_dc = CreateCompatibleDC(gdi_dc);
    ctx->gdi_bmp = CreateCompatibleBitmap(gdi_dc, ctx->dc_size.width, ctx->dc_size.height);
    SelectObject(ctx->gdi_back_dc, ctx->gdi_bmp);
    ReleaseDC(g_main_ctx.hwnd, gdi_dc);

    ctx->gdi_overlay_hwnd =
        CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, GDI_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0,
                       ctx->dc_size.width, ctx->dc_size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);
    SetLayeredWindowAttributes(ctx->gdi_overlay_hwnd, LUA_GDI_COLOR_MASK, 0, LWA_COLORKEY);

    ctx->gdi_front_dc = GetDC(ctx->gdi_overlay_hwnd);

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    FillRect(ctx->gdi_back_dc, &window_rect, g_alpha_mask_brush);

    ctx->d2d_overlay_hwnd =
        CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, D2D_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0,
                       ctx->dc_size.width, ctx->dc_size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);

    SetWindowPos(ctx->gdi_overlay_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetWindowPos(ctx->d2d_overlay_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    SetProp(ctx->d2d_overlay_hwnd, CTX_PROP, env);
    SetProp(ctx->gdi_overlay_hwnd, CTX_PROP, env);

    if (!g_config.lazy_renderer_init)
    {
        ensure_d2d_renderer_created(ctx);
        mark_gdi_content_present(ctx);
    }

    create_loadscreen(ctx);
}

void LuaRenderer::pre_destroy_renderer(t_lua_rendering_context *ctx)
{
    g_view_logger->info("Pre-destroying Lua renderer...");
    ctx->ignore_create_renderer = true;
    SetProp(ctx->gdi_overlay_hwnd, CTX_PROP, nullptr);
    SetProp(ctx->d2d_overlay_hwnd, CTX_PROP, nullptr);
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

    ctx->dw_text_layouts.clear();
    ctx->dw_text_sizes.clear();
    ctx->image_pool.clear();
    ctx->d2d_render_target_stack = {};

    if (IsWindow(ctx->d2d_overlay_hwnd))
    {
        SetProp(ctx->d2d_overlay_hwnd, CTX_PROP, nullptr);
        DestroyWindow(ctx->d2d_overlay_hwnd);
    }

    if (ctx->presenter)
    {
        delete ctx->presenter;
        ctx->presenter = nullptr;
    }

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
    if (ctx->presenter || ctx->ignore_create_renderer)
    {
        return;
    }

    g_view_logger->trace("[Lua] Creating D2D renderer...");

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(ctx->dw_factory),
                        reinterpret_cast<IUnknown **>(&ctx->dw_factory));

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
        DialogService::show_dialog(
            L"Failed to initialize presenter.\r\nVerify that your system supports the selected presenter.", L"Lua",
            fsvc_error);
        return;
    }

    ctx->d2d_render_target_stack.push(ctx->presenter->dc());
    ctx->dw_text_layouts = MicroLRU::Cache<uint64_t, IDWriteTextLayout *>(512, [&](auto value) { value->Release(); });
    ctx->dw_text_sizes = MicroLRU::Cache<uint64_t, DWRITE_TEXT_METRICS>(512, [&](auto value) {});
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
}

HBRUSH LuaRenderer::alpha_mask_brush()
{
    return g_alpha_mask_brush;
}
