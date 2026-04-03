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

const auto OVERLAY_CLASS = L"lua_overlay";

static bool s_detached_overlays{};
static HBRUSH g_alpha_mask_brush;

static std::thread draw_thread;
static std::atomic<bool> draw_thread_running{false};

static void move_and_order_overlays(const std::optional<std::vector<HWND>> &hwnds = std::nullopt);

static void present_gdi_content(t_lua_environment *lua)
{
    SIZE size = {(LONG)lua->rctx.dc_size.width, (LONG)lua->rctx.dc_size.height};
    POINT src_pt = {0, 0};

    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = 0;
    UpdateLayeredWindow(lua->rctx.gdi_overlay_hwnd, nullptr, nullptr, &size, lua->rctx.gdi_back_dc, &src_pt,
                        LuaRenderer::LUA_GDI_COLOR_MASK, &bf, ULW_COLORKEY);
}

static void draw_lua(bool force)
{
    const auto now = std::chrono::steady_clock::now();

    std::vector<t_lua_environment *> to_destroy;
    for (const auto &lua : g_lua_environments)
    {
        const auto time_since_last_render =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lua->rctx.last_render_time).count();

        const auto fps = lua->rctx.target_fps.value_or(1000.0f);
        const auto target_frame_time = 1000.0f / fps;

        if (time_since_last_render < target_frame_time && !force) continue;

        bool success = true;

        // D2D Graphics
        if (!lua->rctx.presenter)
        {
            // NOTE: We have to invoke the callback because we're waiting for the script to issue a d2d call
            success &= LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATDRAWD2D);
        }
        else
        {
            const auto dc = lua->rctx.presenter->dc();
            dc->BeginDraw();
            dc->SetTransform(D2D1::Matrix3x2F::Identity());

            success &= LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATDRAWD2D);
            dc->EndDraw();

            lua->rctx.presenter->present();
        }

        // GDI Graphics. Ugh.
        success &= LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATUPDATESCREEN);

        if (lua->rctx.has_gdi_content)
        {
            present_gdi_content(lua);
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
        g_main_ctx.dispatcher->invoke([]() { draw_lua(false); });

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

        const UINT overlay_swp_flags = SWP_NOACTIVATE | SWP_NOMOVE | (s_detached_overlays ? SWP_NOZORDER : 0);
        SetWindowPos(lua->rctx.gdi_overlay_hwnd, HWND_TOP, 0, 0, width, height, overlay_swp_flags);
        SetWindowPos(lua->rctx.d2d_overlay_hwnd, HWND_TOP, 0, 0, width, height, overlay_swp_flags);
    }
}

static LRESULT CALLBACK overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

// Moves and orders the specified overlay windows to be on top of the main window.
// If no hwnds are provided, all overlay windows from all Lua environments are updated.
static void move_and_order_overlays(const std::optional<std::vector<HWND>> &hwnds)
{
    std::vector<HWND> wnds;
    if (hwnds.has_value())
        wnds = *hwnds;
    else
    {
        for (const auto &lua : g_lua_environments)
        {
            wnds.push_back(lua->rctx.gdi_overlay_hwnd);
            wnds.push_back(lua->rctx.d2d_overlay_hwnd);
        }
    }

    if (s_detached_overlays)
    {
        RECT rc;
        GetClientRect(g_main_ctx.hwnd, &rc);
        POINT pt = {rc.left, rc.top};
        ClientToScreen(g_main_ctx.hwnd, &pt);

        HWND above_main = GetWindow(g_main_ctx.hwnd, GW_HWNDPREV);
        HWND insert_after = above_main ? above_main : HWND_TOP;

        for (const auto &hwnd : wnds)
        {
            SetWindowPos(hwnd, insert_after, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
        }
    }
    else
    {
        for (const auto &hwnd : wnds)
        {
            SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        }
    }
}

void LuaRenderer::init()
{
    if (g_main_ctx.wine)
    {
        s_detached_overlays = true;
        g_view_logger->warn(L"Detected Wine environment, using detached Lua overlays");
    }

    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)overlay_wndproc;
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = OVERLAY_CLASS;
    RegisterClass(&wndclass);

    g_alpha_mask_brush = CreateSolidBrush(LUA_GDI_COLOR_MASK);

    Messenger::subscribe(Messenger::Message::SizeChanged, [](const std::any &data) {
        auto rect = std::any_cast<RECT>(data);
        resize(rect.right - rect.left, rect.bottom - rect.top);
    });

    Messenger::subscribe(Messenger::Message::MainWindowMoved, [](const auto &...) { move_and_order_overlays(); });

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
    draw_lua(true);
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

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    FillRect(ctx->gdi_back_dc, &window_rect, g_alpha_mask_brush);

    const auto ex_style =
        s_detached_overlays ? WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW : WS_EX_LAYERED | WS_EX_TRANSPARENT;
    const auto style = s_detached_overlays ? WS_POPUP | WS_VISIBLE : WS_CHILD | WS_VISIBLE;

    ctx->gdi_overlay_hwnd = CreateWindowEx(ex_style, OVERLAY_CLASS, L"", style, 0, 0, ctx->dc_size.width,
                                           ctx->dc_size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);

    ctx->d2d_overlay_hwnd = CreateWindowEx(ex_style, OVERLAY_CLASS, L"", style, 0, 0, ctx->dc_size.width,
                                           ctx->dc_size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);

    // This env isn't in g_lua_environments yet, so we provide these hwnds manually.
    move_and_order_overlays(std::vector<HWND>{ctx->gdi_overlay_hwnd, ctx->d2d_overlay_hwnd});

    present_gdi_content(env);

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
        DestroyWindow(ctx->d2d_overlay_hwnd);
    }

    if (ctx->presenter)
    {
        delete ctx->presenter;
        ctx->presenter = nullptr;
    }

    if (ctx->gdi_back_dc)
    {
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
        ctx->presenter = new DCompPresenter();
    else
        ctx->presenter = new GDIPresenter(LUA_GDI_COLOR_MASK);

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
