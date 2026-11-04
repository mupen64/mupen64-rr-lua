/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/IDialogService.hpp>
#include <components/Statusbar.hpp>
#include <lua/LuaRealmManager.hpp>
#include <lua/LuaRenderer.hpp>
#include <lua/LuaRendererManager.hpp>
#include <lua/modules/Painter.hpp>
#include <lua/presenters/DCompPresenter.hpp>
#include <lua/presenters/GDIPresenter.hpp>
#include <Common.Views/Messages.hpp>

LRESULT CALLBACK LuaRendererManager::main_window_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data)
{
    switch (msg)
    {
    case WM_MOVE:
    case WM_SIZE:
        for (const auto &realm : LuaRealmManager::instance().realms())
            for (const auto &window : realm->windows) window->reposition();
        [[fallthrough]];
    case WM_DISPLAYCHANGE:
        instance().m_refresh_rate_invalidated = true;
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, main_window_subclass_proc, id);
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

void LuaRendererManager::draw(bool force)
{
    const auto now = std::chrono::steady_clock::now();

    std::vector<std::shared_ptr<LuaRealm>> to_destroy;
    for (const auto &lua : LuaRealmManager::instance().realms())
    {
        const auto time_since_last_render =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lua->renderer.last_render_time).count();

        const auto fps = lua->renderer.target_fps().value_or(1000.0f);
        const auto target_frame_time = 1000.0f / fps;

        bool success = true;
        if (time_since_last_render >= target_frame_time || force)
        {

            if (lua->main_window)
                success &=
                    lua->main_window->call(LuaRealmManager::REG_ATPAINT, LuaCore::Painter::invoke_paint_callback);
            if (lua->renderer.presenter) lua->renderer.presenter->present();

            // GDI Graphics. Ugh.
            success &= LuaRealmManager::instance().call_by_key(lua.get(), LuaRealmManager::REG_ATUPDATESCREEN);

            if (lua->renderer.has_gdi_content())
            {
                lua->renderer.present_gdi_content();
            }

            lua->renderer.last_render_time = now;
        }

        std::vector<LuaWindow *> windows;
        for (const auto &window : lua->windows) windows.push_back(window.get());
        for (auto *window : windows)
        {
            if (window->closed) continue;
            auto &renderer = *window->renderer;
            const auto elapsed = std::chrono::duration<double, std::milli>(now - renderer.last_render_time).count();
            if (!force && elapsed < 1000.0 / renderer.target_fps().value_or(1000.0f)) continue;
            success &= window->call(LuaRealmManager::REG_ATPAINT, LuaCore::Painter::invoke_paint_callback);
            if (!window->closed && renderer.presenter) renderer.presenter->present();
            if (!window->closed && renderer.has_gdi_content()) renderer.present_gdi_content();
            renderer.last_render_time = now;
        }
        if (!success) to_destroy.push_back(lua);
    }

    for (const auto &lua : to_destroy)
    {
        lua->stop();
    }
}

UINT LuaRendererManager::get_screen_refresh_rate()
{

    if (!m_refresh_rate_invalidated.exchange(false)) return m_cached_refresh_rate;

    const HMONITOR monitor = MonitorFromWindow(g_main_ctx.hwnd, MONITOR_DEFAULTTONEAREST);

    m_cached_refresh_rate = 60;
    MONITORINFOEX monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    DEVMODE display_mode{};
    display_mode.dmSize = sizeof(display_mode);
    if (monitor && GetMonitorInfo(monitor, &monitor_info) &&
        EnumDisplaySettings(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &display_mode) &&
        display_mode.dmDisplayFrequency > 1)
        m_cached_refresh_rate = display_mode.dmDisplayFrequency;

    return m_cached_refresh_rate;
}

void LuaRendererManager::draw_clock_proc(std::stop_token stop_token)
{
    while (!stop_token.stop_requested())
    {
        g_main_ctx.dispatcher->invoke([]() { instance().draw(false); });
        std::this_thread::sleep_for(std::chrono::duration<double>(1.0 / instance().get_screen_refresh_rate()));
    }
}

void LuaRendererManager::stop_draw_clock()
{
    m_draw_thread.request_stop();
}

void LuaRendererManager::start_draw_clock()
{
    m_draw_thread = std::jthread(draw_clock_proc);
}

void LuaRendererManager::resize(uint32_t width, uint32_t height)
{
    for (const auto &lua : LuaRealmManager::instance().realms()) lua->renderer.resize(width, height);
}

void LuaRenderer::resize(uint32_t width, uint32_t height)
{
    if (!gdi_back_dc || m_ignore_create_renderer) return;
    width = std::max(width, 1u);
    height = std::max(height, 1u);
    if (dc_size.width == width && dc_size.height == height) return;

    dc_size = {width, height};
    RECT wnd_rect{0, 0, (LONG)width, (LONG)height};

    HDC gdi_dc = GetDC(m_parent);
    HDC new_back_dc = CreateCompatibleDC(gdi_dc);
    HBITMAP new_bmp = CreateCompatibleBitmap(gdi_dc, width, height);
    SelectObject(new_back_dc, new_bmp);
    ReleaseDC(m_parent, gdi_dc);
    SelectObject(gdi_back_dc, nullptr);
    DeleteObject(gdi_bmp);
    DeleteDC(gdi_back_dc);
    gdi_back_dc = new_back_dc;
    gdi_bmp = new_bmp;

    FillRect(gdi_back_dc, &wnd_rect, LuaRendererManager::instance().alpha_mask_brush());
    loadscreen_reset();
    if (presenter) presenter->resize(dc_size);

    const UINT flags = SWP_NOACTIVATE | SWP_NOMOVE |
                       (LuaRendererManager::instance().detached_overlays() ? SWP_NOZORDER | SWP_NOOWNERZORDER : 0);
    SetWindowPos(m_gdi_overlay_hwnd, HWND_TOP, 0, 0, width, height, flags);
    SetWindowPos(m_d2d_overlay_hwnd, HWND_TOP, 0, 0, width, height, flags);
    move_and_order_overlays();
}

LRESULT CALLBACK LuaRendererManager::overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void LuaRendererManager::move_and_order_overlays()
{
    for (const auto &realm : LuaRealmManager::instance().realms())
    {
        realm->renderer.move_and_order_overlays();
        for (const auto &window : realm->windows)
            if (!window->closed) window->renderer->move_and_order_overlays();
    }
}

void LuaRenderer::move_and_order_overlays()
{
    if (!LuaRendererManager::instance().detached_overlays() || !IsWindow(m_parent) || IsIconic(m_parent)) return;

    POINT position{};
    ClientToScreen(m_parent, &position);

    // Keep the surfaces immediately above their own window, without inserting a
    // surface relative to itself when it is already directly above the owner.
    HWND insert_after = GetWindow(m_parent, GW_HWNDPREV);
    while (insert_after && (insert_after == m_gdi_overlay_hwnd || insert_after == m_d2d_overlay_hwnd))
        insert_after = GetWindow(insert_after, GW_HWNDPREV);
    if (!insert_after) insert_after = HWND_TOP;

    for (const auto overlay : {m_gdi_overlay_hwnd, m_d2d_overlay_hwnd})
    {
        if (!IsWindow(overlay)) continue;
        SetWindowPos(
            overlay, insert_after, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
}

void LuaRenderer::set_overlay_visibility(bool visible)
{
    if (!LuaRendererManager::instance().detached_overlays()) return;
    visible = visible && IsWindowVisible(m_parent) && !IsIconic(m_parent);
    for (const auto overlay : {m_gdi_overlay_hwnd, m_d2d_overlay_hwnd})
    {
        if (!IsWindow(overlay) || (IsWindowVisible(overlay) != FALSE) == visible) continue;
        SetWindowPos(overlay, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE |
                (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
    }
    if (visible) move_and_order_overlays();
}

LRESULT CALLBACK LuaRenderer::parent_window_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data)
{
    auto *renderer = reinterpret_cast<LuaRenderer *>(data);
    switch (msg)
    {
    case WM_WINDOWPOSCHANGED: {
        renderer->move_and_order_overlays();
        const auto *position = reinterpret_cast<const WINDOWPOS *>(lparam);
        if (position->flags & SWP_HIDEWINDOW)
            renderer->set_overlay_visibility(false);
        else if (position->flags & SWP_SHOWWINDOW)
            renderer->set_overlay_visibility(true);
        break;
    }
    case WM_SHOWWINDOW:
        renderer->set_overlay_visibility(wparam != 0);
        break;
    case WM_SIZE:
        renderer->set_overlay_visibility(wparam != SIZE_MINIMIZED);
        break;
    case WM_ACTIVATE:
        renderer->set_overlay_visibility(HIWORD(wparam) == 0);
        break;
    case WM_NCDESTROY:
        renderer->set_overlay_visibility(false);
        RemoveWindowSubclass(hwnd, parent_window_subclass_proc, id);
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

void LuaRenderer::present_gdi_content()
{
    SIZE size = {(LONG)dc_size.width, (LONG)dc_size.height};
    POINT src_pt = {0, 0};

    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = 0;
    UpdateLayeredWindow(m_gdi_overlay_hwnd, nullptr, nullptr, &size, gdi_back_dc, &src_pt,
        LuaRenderer::lua_gdi_color_mask(), &bf, ULW_COLORKEY);
}

void LuaRenderer::create_loadscreen()
{
    if (loadscreen_dc)
    {
        return;
    }
    auto gdi_dc = GetDC(m_parent);
    loadscreen_dc = CreateCompatibleDC(gdi_dc);
    loadscreen_bmp = CreateCompatibleBitmap(gdi_dc, dc_size.width, dc_size.height);
    SelectObject(loadscreen_dc, loadscreen_bmp);
    ReleaseDC(m_parent, gdi_dc);
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

void LuaRendererManager::init()
{
    SetWindowSubclass(g_main_ctx.hwnd, main_window_subclass_proc, 0, 0);
    if (g_main_ctx.wine)
    {
        m_detached_overlays = true;
        g_view_logger->warn("Detected Wine environment, using detached Lua overlays");
    }

    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)overlay_wndproc;
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = LuaRendererManager::overlay_class();
    RegisterClass(&wndclass);

    m_alpha_mask_brush = CreateSolidBrush(LuaRenderer::lua_gdi_color_mask());

    Messenger::subscribe<Messenger::Message::SizeChanged>(
        [](const std::pair<int32_t, int32_t> &size) { instance().resize(size.first, size.second); });

    Messenger::subscribe<Messenger::Message::MainWindowMoved>([] { instance().move_and_order_overlays(); });

    start_draw_clock();
}

void LuaRendererManager::stop()
{
    stop_draw_clock();
    DeleteObject(m_alpha_mask_brush);
}

LuaRendererManager &LuaRendererManager::instance()
{
    static LuaRendererManager manager;
    return manager;
}

LuaRenderer::LuaRenderer()
{
    brush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    pen = static_cast<HPEN>(GetStockObject(BLACK_PEN));
    font = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));
    col = bkcol = 0;
    bkmode = TRANSPARENT;
}

void LuaRenderer::initialize(HWND parent)
{
    if (gdi_back_dc != nullptr || m_ignore_create_renderer)
    {
        return;
    }

    g_view_logger->info("Creating multi-target renderer for Lua...");

    m_parent = parent ? parent : g_main_ctx.hwnd;
    const bool detached = LuaRendererManager::instance().detached_overlays();
    RECT window_rect;
    GetClientRect(m_parent, &window_rect);
    if (!parent && Statusbar::hwnd())
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

    auto gdi_dc = GetDC(m_parent);
    gdi_back_dc = CreateCompatibleDC(gdi_dc);
    gdi_bmp = CreateCompatibleBitmap(gdi_dc, dc_size.width, dc_size.height);
    SelectObject(gdi_back_dc, gdi_bmp);
    ReleaseDC(m_parent, gdi_dc);

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    FillRect(gdi_back_dc, &window_rect, LuaRendererManager::instance().alpha_mask_brush());

    const auto ex_style = detached ? WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE
                                   : WS_EX_LAYERED | WS_EX_TRANSPARENT;

    const auto style = detached ? WS_POPUP | WS_SYSMENU : WS_CHILD | WS_VISIBLE;
    POINT position{};
    if (detached) ClientToScreen(m_parent, &position);

    m_gdi_overlay_hwnd = CreateWindowEx(ex_style, LuaRendererManager::overlay_class(), "", style, position.x,
        position.y, dc_size.width, dc_size.height, m_parent, nullptr, g_main_ctx.hinst, nullptr);

    m_d2d_overlay_hwnd = CreateWindowEx(ex_style, LuaRendererManager::overlay_class(), "", style, position.x,
        position.y, dc_size.width, dc_size.height, m_parent, nullptr, g_main_ctx.hinst, nullptr);

    if (detached)
    {
        SetWindowSubclass(
            m_parent, parent_window_subclass_proc, reinterpret_cast<UINT_PTR>(this), reinterpret_cast<DWORD_PTR>(this));
        move_and_order_overlays();
        set_overlay_visibility(IsWindowVisible(m_parent));
    }

    // Put these over the MGE compositor.
    if (!detached)
    {
        SetWindowPos(m_gdi_overlay_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        SetWindowPos(m_d2d_overlay_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    present_gdi_content();

    if (!g_config.lazy_renderer_init)
    {
        ensure_d2d_renderer_created();
        mark_gdi_content_present();
    }

    create_loadscreen();
}

void LuaRenderer::pre_shutdown()
{
    g_view_logger->info("Pre-destroying Lua renderer...");
    m_ignore_create_renderer = true;
}

void LuaRenderer::shutdown()
{
    g_view_logger->info("Destroying Lua renderer...");

    RemoveWindowSubclass(m_parent, parent_window_subclass_proc, reinterpret_cast<UINT_PTR>(this));

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

    if (IsWindow(m_d2d_overlay_hwnd))
    {
        DestroyWindow(m_d2d_overlay_hwnd);
    }

    presenter.reset();

    if (gdi_back_dc)
    {
        DestroyWindow(m_gdi_overlay_hwnd);
        SelectObject(gdi_back_dc, nullptr);
        DeleteDC(gdi_back_dc);
        DeleteObject(gdi_bmp);
        gdi_back_dc = nullptr;
        destroy_loadscreen();
    }
}

void LuaRenderer::ensure_d2d_renderer_created()
{
    if (presenter || m_ignore_create_renderer)
    {
        return;
    }

    g_view_logger->trace("[Lua] Creating D2D renderer...");

    if (g_config.presenter_type != (int32_t)Config::PresenterType::GDI)
        presenter = std::make_unique<DCompPresenter>();
    else
        presenter = std::make_unique<GDIPresenter>(m_lua_gdi_color_mask);

    if (!presenter->init(m_d2d_overlay_hwnd))
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
    m_has_gdi_content = true;
}

void LuaRenderer::loadscreen_reset()
{
    destroy_loadscreen();
    create_loadscreen();
}

void LuaRenderer::set_target_fps(std::optional<float> fps)
{
    if (m_target_fps == fps) return;
    if (fps.has_value())
    {
        if (!std::isfinite(fps.value()) || fps.value() <= 0.0f) return;
    }

    m_target_fps = fps;
}

void LuaRendererManager::blit_all(HDC hdc)
{
    for (const auto &lua : LuaRealmManager::instance().realms())
    {
        if (!lua->renderer.presenter) continue;

        const auto presenter_size = lua->renderer.presenter->size();
        lua->renderer.presenter->blit(hdc, {0, 0, (LONG)presenter_size.width, (LONG)presenter_size.height});
    }

    for (const auto &lua : LuaRealmManager::instance().realms())
    {
        if (!lua->renderer.has_gdi_content()) continue;

        TransparentBlt(hdc, 0, 0, lua->renderer.dc_size.width, lua->renderer.dc_size.height, lua->renderer.gdi_back_dc,
            0, 0, lua->renderer.dc_size.width, lua->renderer.dc_size.height, LuaRenderer::lua_gdi_color_mask());
    }
}
