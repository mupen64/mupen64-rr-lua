/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaRenderer.hpp>

#include <atomic>
#include <optional>
#include <thread>
#include <vector>

/**
 * \brief Owns the Lua renderer subsystem and coordinates per-environment renderer lifecycles.
 */
class LuaRendererManager
{
  public:
    LuaRendererManager(const LuaRendererManager &) = delete;
    LuaRendererManager &operator=(const LuaRendererManager &) = delete;
    LuaRendererManager(LuaRendererManager &&) = delete;
    LuaRendererManager &operator=(LuaRendererManager &&) = delete;

    static LuaRendererManager &instance();

    void init();
    void stop();
    void draw_all();
    void blit_all(HDC hdc);

  private:
    LuaRendererManager() = default;

    friend class LuaRenderer;

    static constexpr auto m_overlay_class = "lua_overlay";

    bool m_detached_overlays{};
    HBRUSH m_alpha_mask_brush{};
    std::jthread m_draw_thread;
    std::atomic_bool m_refresh_rate_invalidated{true};
    UINT m_cached_refresh_rate{60};

    static LRESULT CALLBACK main_window_subclass_proc(
        HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data);
    static LRESULT CALLBACK overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static void draw_clock_proc(std::stop_token stop_token);

    void set_overlay_visibility(bool visible);
    void draw_lua(bool force);
    UINT get_screen_refresh_rate();
    void start_draw_clock();
    void stop_draw_clock();
    void resize(uint32_t width, uint32_t height);
    void move_and_order_overlays(const std::optional<std::vector<HWND>> &hwnds = std::nullopt);
};
