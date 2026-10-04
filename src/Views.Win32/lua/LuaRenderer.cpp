/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/IDialogService.hpp>
#include <components/Statusbar.hpp>
#include <lua/LuaHost.hpp>
#include <lua/LuaRenderer.hpp>
#include <lua/modules/Painter.hpp>
#include <lua/presenters/DCompPresenter.hpp>
#include <lua/presenters/GDIPresenter.hpp>
#include <Common.Views/Messages.hpp>

const auto OVERLAY_CLASS = "lua_overlay";

static bool g_detached_overlays{};
static HBRUSH g_alpha_mask_brush;

static std::jthread s_draw_thread;
static std::atomic s_refresh_rate_invalidated{true};

static void move_and_order_overlays(const std::optional<std::vector<HWND>> &hwnds = std::nullopt);

static void set_overlay_visibility(bool visible)
{
    if (!g_detached_overlays) return;

    for (const auto &lua : LuaHost::instance().envs())
    {
        const auto set_window_visibility = [&](HWND hwnd) {
            if (!IsWindow(hwnd)) return;
            ShowWindow(hwnd, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
        };
        set_window_visibility(lua->renderer.gdi_overlay_hwnd);
        set_window_visibility(lua->renderer.d2d_overlay_hwnd);
    }
}

static LRESULT CALLBACK main_window_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data)
{
    switch (msg)
    {
    case WM_MOVE:
    case WM_DISPLAYCHANGE:
        s_refresh_rate_invalidated = true;
        break;
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

static void draw_lua(bool force)
{
    const auto now = std::chrono::steady_clock::now();

    std::vector<std::shared_ptr<LuaEnvironment>> to_destroy;
    for (const auto &lua : LuaHost::instance().envs())
    {
        const auto time_since_last_render =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lua->renderer.last_render_time).count();

        const auto fps = lua->renderer.target_fps.value_or(1000.0f);
        const auto target_frame_time = 1000.0f / fps;

        if (time_since_last_render < target_frame_time && !force) continue;

        bool success = true;

        success &=
            LuaHost::instance().call_by_key(lua.get(), LuaHost::REG_ATPAINT, LuaCore::Painter::invoke_paint_callback);
        if (lua->renderer.presenter) lua->renderer.presenter->present();

        // GDI Graphics. Ugh.
        success &= LuaHost::instance().call_by_key(lua.get(), LuaHost::REG_ATUPDATESCREEN);

        if (lua->renderer.has_gdi_content)
        {
            lua->renderer.present_gdi_content();
        }

        lua->renderer.last_render_time = now;

        if (!success) to_destroy.push_back(lua);
    }

    for (const auto &lua : to_destroy)
    {
        lua->stop();
    }
}

static UINT get_screen_refresh_rate()
{

    static UINT cached_refresh_rate = 60;

    if (!s_refresh_rate_invalidated.exchange(false)) return cached_refresh_rate;

    const HMONITOR monitor = MonitorFromWindow(g_main_ctx.hwnd, MONITOR_DEFAULTTONEAREST);

    cached_refresh_rate = 60;
    MONITORINFOEX monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    DEVMODE display_mode{};
    display_mode.dmSize = sizeof(display_mode);
    if (monitor && GetMonitorInfo(monitor, &monitor_info) &&
        EnumDisplaySettings(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &display_mode) &&
        display_mode.dmDisplayFrequency > 1)
        cached_refresh_rate = display_mode.dmDisplayFrequency;

    return cached_refresh_rate;
}

static void draw_clock_proc(std::stop_token stop_token)
{
    while (!stop_token.stop_requested())
    {
        g_main_ctx.dispatcher->invoke([]() { draw_lua(false); });
        std::this_thread::sleep_for(std::chrono::duration<double>(1.0 / get_screen_refresh_rate()));
    }
}

static void stop_draw_clock()
{
    s_draw_thread.request_stop();
}

static void start_draw_clock()
{
    s_draw_thread = std::jthread(draw_clock_proc);
}

static void resize(uint32_t width, uint32_t height)
{
    width = std::max(width, 1u);
    height = std::max(height, 1u);

    for (const auto &lua : LuaHost::instance().envs())
    {
        if (lua->renderer.dc_size.width == width && lua->renderer.dc_size.height == height) continue;

        lua->renderer.dc_size = {width, height};
        RECT wnd_rect{0, 0, (LONG)width, (LONG)height};

        HDC gdi_dc = GetDC(g_main_ctx.hwnd);
        HDC new_back_dc = CreateCompatibleDC(gdi_dc);
        HBITMAP new_bmp = CreateCompatibleBitmap(gdi_dc, width, height);
        SelectObject(new_back_dc, new_bmp);
        ReleaseDC(g_main_ctx.hwnd, gdi_dc);
        SelectObject(lua->renderer.gdi_back_dc, nullptr);
        DeleteObject(lua->renderer.gdi_bmp);
        DeleteDC(lua->renderer.gdi_back_dc);
        lua->renderer.gdi_back_dc = new_back_dc;
        lua->renderer.gdi_bmp = new_bmp;

        FillRect(lua->renderer.gdi_back_dc, &wnd_rect, g_alpha_mask_brush);

        lua->renderer.loadscreen_reset();

        if (lua->renderer.presenter) lua->renderer.presenter->resize(lua->renderer.dc_size);

        const UINT overlay_swp_flags = SWP_NOACTIVATE | SWP_NOMOVE | (g_detached_overlays ? SWP_NOZORDER : 0);
        SetWindowPos(lua->renderer.gdi_overlay_hwnd, HWND_TOP, 0, 0, width, height, overlay_swp_flags);
        SetWindowPos(lua->renderer.d2d_overlay_hwnd, HWND_TOP, 0, 0, width, height, overlay_swp_flags);
    }
}

static LRESULT CALLBACK overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

// Moves and orders the specified overlay windows to be on top of the main window.
// If no hwnds are provided, all overlay windows from all Lua environments are updated.
static void move_and_order_overlays(const std::optional<std::vector<HWND>> &hwnds)
{
    if (!g_detached_overlays) return;

    std::vector<HWND> wnds;
    if (hwnds.has_value())
        wnds = *hwnds;
    else
    {
        for (const auto &lua : LuaHost::instance().envs())
        {
            wnds.push_back(lua->renderer.gdi_overlay_hwnd);
            wnds.push_back(lua->renderer.d2d_overlay_hwnd);
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

void LuaRenderer::present_gdi_content()
{
    SIZE size = {(LONG)dc_size.width, (LONG)dc_size.height};
    POINT src_pt = {0, 0};

    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = 0;
    UpdateLayeredWindow(gdi_overlay_hwnd, nullptr, nullptr, &size, gdi_back_dc, &src_pt,
        LuaRenderer::lua_gdi_color_mask, &bf, ULW_COLORKEY);
}

void LuaRenderer::create_loadscreen()
{
    if (loadscreen_dc)
    {
        return;
    }
    auto gdi_dc = GetDC(g_main_ctx.hwnd);
    loadscreen_dc = CreateCompatibleDC(gdi_dc);
    loadscreen_bmp = CreateCompatibleBitmap(gdi_dc, dc_size.width, dc_size.height);
    SelectObject(loadscreen_dc, loadscreen_bmp);
    ReleaseDC(g_main_ctx.hwnd, gdi_dc);
}

void LuaRenderer::destroy_loadscreen()
{
    if (!loadscreen_dc)
    {
        return;
    }
    SelectObject(loadscreen_dc, nullptr);
    DeleteDC(loadscreen_dc);
    DeleteObject(loadscreen_bmp);
    loadscreen_dc = nullptr;
}

void LuaRenderer::init()
{
    SetWindowSubclass(g_main_ctx.hwnd, main_window_subclass_proc, 0, 0);
    if (g_main_ctx.wine)
    {
        g_detached_overlays = true;
        g_view_logger->warn("Detected Wine environment, using detached Lua overlays");
    }

    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)overlay_wndproc;
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = OVERLAY_CLASS;
    RegisterClass(&wndclass);

    g_alpha_mask_brush = CreateSolidBrush(lua_gdi_color_mask);

    Messenger::subscribe<Messenger::Message::SizeChanged>(
        [](const std::pair<int32_t, int32_t> &size) { resize(size.first, size.second); });

    Messenger::subscribe<Messenger::Message::MainWindowMoved>([] { move_and_order_overlays(); });

    start_draw_clock();
}

void LuaRenderer::stop()
{
    stop_draw_clock();
    DeleteObject(g_alpha_mask_brush);
}

LuaRenderer::LuaRenderer()
{
    brush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    pen = static_cast<HPEN>(GetStockObject(BLACK_PEN));
    font = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));
    col = bkcol = 0;
    bkmode = TRANSPARENT;
}

void LuaRenderer::draw_all()
{
    need(is_on_gui_thread(), "must be on GUI thread");
    draw_lua(true);
}

void LuaRenderer::initialize()
{
    if (gdi_back_dc != nullptr || ignore_create_renderer)
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
    dc_size = {(UINT32)std::max(1, (int32_t)window_rect.right), (UINT32)std::max(1, (int32_t)window_rect.bottom)};
    g_view_logger->info("Lua dc size: {} {}", dc_size.width, dc_size.height);

    // Key 0 is reserved for clearing the image pool, too late to change it now...
    image_pool_index = 1;

    auto gdi_dc = GetDC(g_main_ctx.hwnd);
    gdi_back_dc = CreateCompatibleDC(gdi_dc);
    gdi_bmp = CreateCompatibleBitmap(gdi_dc, dc_size.width, dc_size.height);
    SelectObject(gdi_back_dc, gdi_bmp);
    ReleaseDC(g_main_ctx.hwnd, gdi_dc);

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    FillRect(gdi_back_dc, &window_rect, g_alpha_mask_brush);

    const auto ex_style =
        g_detached_overlays ? WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW : WS_EX_LAYERED | WS_EX_TRANSPARENT;
    const auto style = g_detached_overlays ? WS_POPUP | WS_VISIBLE : WS_CHILD | WS_VISIBLE;

    gdi_overlay_hwnd = CreateWindowEx(ex_style, OVERLAY_CLASS, "", style, 0, 0, dc_size.width, dc_size.height,
        g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);

    d2d_overlay_hwnd = CreateWindowEx(ex_style, OVERLAY_CLASS, "", style, 0, 0, dc_size.width, dc_size.height,
        g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, nullptr);

    // This renderer's environment isn't in LuaHost::instance().envs() yet, so provide its hwnds manually.
    move_and_order_overlays(std::vector<HWND>{gdi_overlay_hwnd, d2d_overlay_hwnd});

    // Put these over the MGE compositor.
    if (!g_detached_overlays)
    {
        SetWindowPos(gdi_overlay_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        SetWindowPos(d2d_overlay_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    present_gdi_content();

    if (!g_config.lazy_renderer_init)
    {
        mark_d2d_content_present();
        mark_gdi_content_present();
    }

    create_loadscreen();
}

void LuaRenderer::pre_shutdown()
{
    g_view_logger->info("Pre-destroying Lua renderer...");
    ignore_create_renderer = true;
}

void LuaRenderer::shutdown()
{
    g_view_logger->info("Destroying Lua renderer...");

    SelectObject(gdi_back_dc, nullptr);
    DeleteObject(brush);
    DeleteObject(pen);
    DeleteObject(font);

    for (const auto bmp : image_pool | std::views::values)
    {
        delete bmp;
    }

    painter_text_layouts.reset();
    painter_text_measurements.reset();
    painter_text_factory.reset();
    image_pool.clear();
    d2d_render_target_stack = {};

    if (IsWindow(d2d_overlay_hwnd))
    {
        DestroyWindow(d2d_overlay_hwnd);
    }

    if (presenter)
    {
        delete presenter;
        presenter = nullptr;
    }

    if (gdi_back_dc)
    {
        DestroyWindow(gdi_overlay_hwnd);
        SelectObject(gdi_back_dc, nullptr);
        DeleteDC(gdi_back_dc);
        DeleteObject(gdi_bmp);
        gdi_back_dc = nullptr;
        destroy_loadscreen();
    }
}

void LuaRenderer::mark_d2d_content_present()
{
    if (presenter || ignore_create_renderer)
    {
        return;
    }

    g_view_logger->trace("[Lua] Creating D2D renderer...");

    if (g_config.presenter_type != (int32_t)Config::PresenterType::GDI)
        presenter = new DCompPresenter();
    else
        presenter = new GDIPresenter(lua_gdi_color_mask);

    if (!presenter->init(d2d_overlay_hwnd))
    {
        DialogService::show_dialog(
            "Failed to initialize presenter.\r\nVerify that your system supports the selected presenter.", "Lua",
            CoreMessageTone::Error);
        return;
    }

    d2d_render_target_stack.push(presenter->dc());
}

void LuaRenderer::mark_gdi_content_present()
{
    has_gdi_content = true;
}

void LuaRenderer::loadscreen_reset()
{
    destroy_loadscreen();
    create_loadscreen();
}

void LuaRenderer::set_target_fps(std::optional<float> fps)
{
    if (target_fps == fps) return;
    if (fps.has_value())
    {
        if (!std::isfinite(fps.value()) || fps.value() <= 0.0f) return;
    }

    target_fps = fps;
}

void LuaRenderer::blit_all(HDC hdc)
{
    for (const auto &lua : LuaHost::instance().envs())
    {
        if (!lua->renderer.presenter) continue;

        const auto presenter_size = lua->renderer.presenter->size();
        lua->renderer.presenter->blit(hdc, {0, 0, (LONG)presenter_size.width, (LONG)presenter_size.height});
    }

    for (const auto &lua : LuaHost::instance().envs())
    {
        if (!lua->renderer.has_gdi_content) continue;

        TransparentBlt(hdc, 0, 0, lua->renderer.dc_size.width, lua->renderer.dc_size.height, lua->renderer.gdi_back_dc,
            0, 0, lua->renderer.dc_size.width, lua->renderer.dc_size.height, LuaRenderer::lua_gdi_color_mask);
    }
}
