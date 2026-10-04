/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/presenters/Presenter.hpp>
#include <memory>

namespace LuaCore::Painter::Detail
{
class TextLayoutCache;
class TextMeasurementCache;
class TextFactoryCache;
} // namespace LuaCore::Painter::Detail

/**
 * \brief Holds the rendering state for a Lua realm.
 */
class LuaRenderer
{
  public:
    LuaRenderer();

    LuaRenderer(const LuaRenderer &) = delete;
    LuaRenderer &operator=(const LuaRenderer &) = delete;
    LuaRenderer(LuaRenderer &&) = delete;
    LuaRenderer &operator=(LuaRenderer &&) = delete;

    std::unique_ptr<Presenter> presenter;
    D2D1_SIZE_U dc_size{};

    HDC gdi_back_dc{};
    HBITMAP gdi_bmp{};

    std::shared_ptr<LuaCore::Painter::Detail::TextLayoutCache> painter_text_layouts;
    std::shared_ptr<LuaCore::Painter::Detail::TextMeasurementCache> painter_text_measurements;
    std::shared_ptr<LuaCore::Painter::Detail::TextFactoryCache> painter_text_factory;
    std::stack<ID2D1RenderTarget *> d2d_render_target_stack;

    // GDI+ images and drawing state
    std::unordered_map<size_t, Gdiplus::Bitmap *> image_pool{};
    size_t image_pool_index{};
    HDC loadscreen_dc{};
    HBITMAP loadscreen_bmp{};
    HBRUSH brush{};
    HPEN pen{};
    HFONT font{};
    COLORREF col, bkcol{};
    int bkmode{};


    std::chrono::steady_clock::time_point last_render_time;

    void initialize();
    void pre_shutdown();
    void shutdown();
    void present_gdi_content();
    void mark_gdi_content_present();
    void ensure_d2d_renderer_created();
    void loadscreen_reset();
    void set_target_fps(std::optional<float> fps);
    const std::optional<float> &target_fps() const { return m_target_fps; }

    HWND d2d_overlay_hwnd() const { return m_d2d_overlay_hwnd; }
    HWND gdi_overlay_hwnd() const { return m_gdi_overlay_hwnd; }
    bool has_gdi_content() const { return m_has_gdi_content; }
    bool ignore_create_renderer() const { return m_ignore_create_renderer; }
    static constexpr uint32_t lua_gdi_color_mask() { return m_lua_gdi_color_mask; }

  private:
    static constexpr uint32_t m_lua_gdi_color_mask = RGB(255, 0, 255);
    std::optional<float> m_target_fps;

    HWND m_d2d_overlay_hwnd{};
    HWND m_gdi_overlay_hwnd{};
    bool m_has_gdi_content{};
    bool m_ignore_create_renderer{};

    void create_loadscreen();
    void destroy_loadscreen();
};
