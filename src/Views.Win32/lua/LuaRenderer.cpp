#include "stdafx.h"
#include <DialogService.hpp>
#include <components/Statusbar.hpp>
#include <lua/LuaManager.hpp>
#include <lua/LuaRenderer.hpp>
#include <lua/LuaCallbacks.hpp>
#include "LuaRenderer.hpp"
#include <Messenger.hpp>

const auto OVERLAY_CLASS = L"lua_overlay";

static bool s_detached_overlays{};

static std::thread draw_thread;
static std::atomic<bool> draw_thread_running{false};

static void move_and_order_overlays(const std::optional<std::vector<HWND>> &hwnds = std::nullopt);

static void set_overlay_visibility(bool visible)
{
    if (!s_detached_overlays) return;

    for (const auto &lua : g_lua_environments)
    {
        if (IsWindow(lua->rctx.overlay_hwnd)) ShowWindow(lua->rctx.overlay_hwnd, visible ? SW_SHOW : SW_HIDE);
    }
}

static LRESULT CALLBACK main_window_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                                  DWORD_PTR data)
{
    switch (msg)
    {
    case WM_ACTIVATE:
        switch (LOWORD(wparam))
        {
        case WA_ACTIVE:
        case WA_CLICKACTIVE:
            set_overlay_visibility(true);
            break;
        case WA_INACTIVE:
            set_overlay_visibility(false);
            break;
        default:
            break;
        }
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, main_window_subclass_proc, id);
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

static void present(t_lua_environment *lua)
{
    {
        BLContext ctx(lua->rctx.bl_image);
        BLRectI rect;
        rect.w = lua->rctx.dc_size.width;
        rect.h = lua->rctx.dc_size.height;

        ctx.clearRect(rect);

        ctx.end();
    }

    SIZE size = {(LONG)lua->rctx.dc_size.width, (LONG)lua->rctx.dc_size.height};
    POINT src_pt = {0, 0};

    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(lua->rctx.overlay_hwnd, nullptr, nullptr, &size, lua->rctx.gdi_back_dc, &src_pt, 0, &bf,
                        ULW_ALPHA);
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

        success &= LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATUPDATESCREEN);
        success &= LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATDRAWD2D);
        present(lua);

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

        destroy_loadscreen(&lua->rctx);
        create_loadscreen(&lua->rctx);

        lua->rctx.bl_image.createFromData(lua->rctx.dc_size.width, lua->rctx.dc_size.height, BL_FORMAT_PRGB32,
                                          lua->rctx.bl_raw, lua->rctx.dc_size.width * 4);

        const UINT overlay_swp_flags = SWP_NOACTIVATE | SWP_NOMOVE | (s_detached_overlays ? SWP_NOZORDER : 0);
        SetWindowPos(lua->rctx.overlay_hwnd, HWND_TOP, 0, 0, width, height, overlay_swp_flags);
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
    if (!s_detached_overlays) return;

    std::vector<HWND> wnds;
    if (hwnds.has_value())
        wnds = *hwnds;
    else
    {
        for (const auto &lua : g_lua_environments)
        {
            wnds.push_back(lua->rctx.overlay_hwnd);
        }
    }

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

void LuaRenderer::init()
{
    if (g_main_ctx.wine)
    {
        s_detached_overlays = true;
        SetWindowSubclass(g_main_ctx.hwnd, main_window_subclass_proc, 0, 0);
        g_view_logger->warn(L"Detected Wine environment, using detached Lua overlays");
    }

    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)overlay_wndproc;
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = OVERLAY_CLASS;
    RegisterClass(&wndclass);

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

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = ctx->dc_size.width;
    bmi.bmiHeader.biHeight = -ctx->dc_size.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    ctx->gdi_bmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &ctx->bl_raw, nullptr, 0);
    SelectObject(ctx->gdi_back_dc, ctx->gdi_bmp);
    ReleaseDC(g_main_ctx.hwnd, gdi_dc);

    const auto ex_style =
        s_detached_overlays ? WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW : WS_EX_LAYERED | WS_EX_TRANSPARENT;
    const auto style = s_detached_overlays ? WS_POPUP | WS_VISIBLE : WS_CHILD | WS_VISIBLE;

    ctx->overlay_hwnd = CreateWindowEx(ex_style, OVERLAY_CLASS, L"", style, 0, 0, ctx->dc_size.width,
                                       ctx->dc_size.height, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);

    // This env isn't in g_lua_environments yet, so we provide these hwnds manually.
    move_and_order_overlays(std::vector<HWND>{ctx->overlay_hwnd});

    // Put these over the MGE compositor.
    if (!s_detached_overlays)
    {
        SetWindowPos(ctx->overlay_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    ctx->bl_image.createFromData(ctx->dc_size.width, ctx->dc_size.height, BL_FORMAT_PRGB32, ctx->bl_raw,
                                 ctx->dc_size.width * 4);

    present(env);
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

    DestroyWindow(ctx->overlay_hwnd);
    SelectObject(ctx->gdi_back_dc, nullptr);
    DeleteObject(ctx->brush);
    DeleteObject(ctx->pen);
    DeleteObject(ctx->font);
    DeleteDC(ctx->gdi_back_dc);
    DeleteObject(ctx->gdi_bmp);
    ctx->bl_image.reset();

    for (const auto bmp : ctx->image_pool | std::views::values)
    {
        delete bmp;
    }
    ctx->image_pool.clear();

    ctx->gdi_back_dc = nullptr;

    destroy_loadscreen(ctx);
}

void LuaRenderer::mark_gdi_content_present(t_lua_rendering_context *ctx)
{
    // stub
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

void LuaRenderer::blit_all(HDC hdc)
{
    // FIXME
}
