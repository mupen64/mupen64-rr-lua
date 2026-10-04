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
 * \brief Implements rendering-related functionality for a Lua environment.
 */
class LuaRenderer
{
  public:
    LuaRenderer();

    // The current presenter, or null
    Presenter *presenter{};

    // The Direct2D overlay control handle
    HWND d2d_overlay_hwnd{};

    // The GDI/GDI+ overlay control handle
    HWND gdi_overlay_hwnd{};

    bool has_gdi_content{};

    // The DC for GDI/GDI+ drawings
    // This DC is special, since commands can be issued to it anytime and it's never cleared
    HDC gdi_back_dc{};

    // The bitmap for GDI/GDI+ drawings
    HBITMAP gdi_bmp{};

    // Dimensions of the drawing surfaces
    D2D1_SIZE_U dc_size{};

    // The LRU cache for painter text layouts
    std::shared_ptr<LuaCore::Painter::Detail::TextLayoutCache> painter_text_layouts{};

    // The LRU cache for painter text measurements
    std::shared_ptr<LuaCore::Painter::Detail::TextMeasurementCache> painter_text_measurements{};

    // The shared DirectWrite factory
    std::shared_ptr<LuaCore::Painter::Detail::TextFactoryCache> painter_text_factory{};

    // The stack of render targets. The top is used for D2D calls.
    std::stack<ID2D1RenderTarget *> d2d_render_target_stack{};

    // Pool of GDI+ images
    std::unordered_map<size_t, Gdiplus::Bitmap *> image_pool{};

    // Amount of generated images, just used to generate uids for image pool
    size_t image_pool_index{};

    // Whether to ignore create_renderer() and ensure_d2d_renderer_created() calls. Used to avoid tearing down and
    // re-creating a renderer when stopping a script.
    bool ignore_create_renderer{};

    std::optional<float> target_fps{};
    std::chrono::steady_clock::time_point last_render_time{};

    HDC loadscreen_dc{};
    HBITMAP loadscreen_bmp{};

    HBRUSH brush{};
    HPEN pen{};
    HFONT font{};
    COLORREF col, bkcol{};
    int bkmode{};

    static constexpr uint32_t lua_gdi_color_mask = RGB(255, 0, 255);

    /**
     * \brief Initializes the subsystem.
     */
    static void init();

    /**
     * \brief Stops the subsystem.
     */
    static void stop();

    /**
     * \brief Forces an immediate repaint of all visual layers of all running Lua scripts.
     * \remarks Must be called from the UI thread.
     */
    static void draw_all();

    /**
     * \brief Blits the graphics contents of all active Lua instances to the given HDC.
     */
    static void blit_all(HDC hdc);

    /**
     * \brief Initializes this renderer. Does nothing if the renderer is initialized.
     */
    void initialize();

    /**
     * \brief Prepares this renderer for deinitialization. Does nothing if the renderer isn't initialized.
     */
    void pre_shutdown();

    /**
     * \brief Deinitializes this renderer. Does nothing if the renderer isn't initialized.
     */
    void shutdown();

    /**
     * \brief Ensures that the D2D renderer is created for a Lua environment. Does nothing if the renderer already
     * exists.
     */
    void mark_d2d_content_present();

    /**
     * \brief Tells the renderer that GDI content is present in the rendering context.
     */
    void mark_gdi_content_present();

    /**
     * \brief Resets the loadscreen graphics.
     */
    void loadscreen_reset();

    /**
     * \brief Sets the target FPS.
     * \param fps The target FPS. If std::nullopt, an FPS equal to the monitor refresh rate will be used.
     */
    void set_target_fps(std::optional<float> fps);

    /**
     * \brief Presents this renderer's GDI content to its overlay window.
     */
    void present_gdi_content();

  private:
    void create_loadscreen();
    void destroy_loadscreen();
};
