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

class LuaEnvironment;
class LuaHost;
class LuaRendererManager;

/**
 * \brief Holds the rendering state for a Lua environment.
 */
class LuaRenderer
{
    friend class LuaEnvironment;
    friend class LuaHost;
    friend class LuaRendererManager;

  public:
    LuaRenderer(const LuaRenderer &) = delete;
    LuaRenderer &operator=(const LuaRenderer &) = delete;
    LuaRenderer(LuaRenderer &&) = delete;
    LuaRenderer &operator=(LuaRenderer &&) = delete;

    // The current presenter, or null
    Presenter *presenter{};

    // The DC for GDI/GDI+ drawings. Commands can be issued to it anytime and it is never cleared.
    HDC gdi_back_dc{};

    // Dimensions of the drawing surfaces
    D2D1_SIZE_U dc_size{};

    // Painter caches and render-target stack
    std::shared_ptr<LuaCore::Painter::Detail::TextLayoutCache> painter_text_layouts{};
    std::shared_ptr<LuaCore::Painter::Detail::TextMeasurementCache> painter_text_measurements{};
    std::shared_ptr<LuaCore::Painter::Detail::TextFactoryCache> painter_text_factory{};
    std::stack<ID2D1RenderTarget *> d2d_render_target_stack{};

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

    std::optional<float> target_fps{};

    /**
     * \brief Tells the renderer that GDI content is present.
     */
    void mark_gdi_content_present();
    void ensure_d2d_renderer_created();

    /**
     * \brief Resets the loadscreen graphics.
     */
    void loadscreen_reset();

    /**
     * \brief Sets the target FPS. If std::nullopt, an FPS equal to the monitor refresh rate will be used.
     */
    void set_target_fps(std::optional<float> fps);

  private:
    LuaRenderer();

    static constexpr uint32_t m_lua_gdi_color_mask = RGB(255, 0, 255);

    HWND m_d2d_overlay_hwnd{};
    HWND m_gdi_overlay_hwnd{};
    bool m_has_gdi_content{};
    HBITMAP m_gdi_bmp{};

    bool m_ignore_create_renderer{};
    std::chrono::steady_clock::time_point m_last_render_time{};

    void initialize();
    void pre_shutdown();
    void shutdown();
    void present_gdi_content();
    void create_loadscreen();
    void destroy_loadscreen();
};
