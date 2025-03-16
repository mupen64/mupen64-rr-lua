/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "presenters/Presenter.h"

constexpr uint32_t LUA_GDI_COLOR_MASK = RGB(255, 0, 255);
static HBRUSH g_alpha_mask_brush = CreateSolidBrush(LUA_GDI_COLOR_MASK);


/**
 * \brief Describes a Lua instance.
 */
typedef struct LuaEnvironment {

    HWND hwnd;
    lua_State* L;

    // The path to the current lua script
    std::filesystem::path path;

    // The current presenter, or null
    Presenter* presenter;

    // The Direct2D overlay control handle
    HWND d2d_overlay_hwnd;

    // The GDI/GDI+ overlay control handle
    HWND gdi_overlay_hwnd;

    // The DC for GDI/GDI+ drawings
    // This DC is special, since commands can be issued to it anytime and it's never cleared
    HDC gdi_back_dc = nullptr;

    // The bitmap for GDI/GDI+ drawings
    HBITMAP gdi_bmp;

    // Dimensions of the drawing surfaces
    D2D1_SIZE_U dc_size;

    // The DirectWrite factory, whose lifetime is the renderer's
    IDWriteFactory* dw_factory = nullptr;

    // The cache for DirectWrite text layouts
    MicroLRU::Cache<uint64_t, IDWriteTextLayout*> dw_text_layouts;

    // The cache for DirectWrite text size measurements
    MicroLRU::Cache<uint64_t, DWRITE_TEXT_METRICS> dw_text_sizes;

    // The stack of render targets. The top is used for D2D calls.
    std::stack<ID2D1RenderTarget*> d2d_render_target_stack;

    // Pool of GDI+ images
    std::unordered_map<size_t, Gdiplus::Bitmap*> image_pool;

    // Amount of generated images, just used to generate uids for image pool
    size_t image_pool_index;

    // Whether to ignore create_renderer() and ensure_d2d_renderer_created() calls. Used to avoid tearing down and re-creating a renderer when stopping a script.
    bool m_ignore_renderer_creation = false;

    HDC loadscreen_dc;
    HBITMAP loadscreen_bmp;

    HBRUSH brush;
    HPEN pen;
    HFONT font;
    COLORREF col, bkcol;
    int bkmode;
} t_lua_environment;

static int pcall_no_params(lua_State* L)
{
    return lua_pcall(L, 0, 0, 0);
}

/**
 * \brief Initializes the lua subsystem
 */
void lua_init();

/**
 * \brief Creates a lua window and runs the specified script
 * \param path The script's path
 */
void lua_create_and_run(const std::wstring& path);

/**
 * \brief Creates a lua window
 * \return The window's handle
 */
HWND lua_create();

/**
 * \brief Stops all lua scripts
 */
void lua_stop_all_scripts();

/**
 * \brief Stops all lua scripts and closes their windows
 */
void lua_close_all_scripts();

/**
 * \brief Creates a lua environment, adding it to the global map and running it if the operation succeeds
 * \param path The script path
 * \param wnd The associated window
 * \return An error string, or an empty string if the operation succeeded
 */
std::string create_lua_environment(const std::filesystem::path& path, HWND wnd);

/**
 * \brief Stops, destroys and removes a lua environment from the environment map
 */
void destroy_lua_environment(t_lua_environment*);

/**
 * \brief Prints text to a lua environment dialog's console
 * \param hwnd Handle to a lua environment dialog of IDD_LUAWINDOW
 * \param text The text to display
 */
void print_con(HWND hwnd, const std::wstring& text);

/**
 * \brief Creates the renderer for a Lua environment. Does nothing if the renderer already exists.
 */
void create_renderer(t_lua_environment*);

/**
 * \brief Destroys the renderer for a Lua environment. Does nothing if the renderer doesn't exist.
 */
void destroy_renderer(t_lua_environment*);

/**
 * \brief Creates the loadscreen DC and bitmap for a Lua environment. Does nothing if the loadscreen already exists.
 */
void create_loadscreen(t_lua_environment*);

/**
 * \brief Destroys the loadscreen DC and bitmap for a Lua environment. Does nothing if the loadscreen doesn't exist.
 */
void destroy_loadscreen(t_lua_environment*);

/**
 * \brief Ensures that the D2D renderer is created for a Lua environment. Does nothing if the renderer already exists.
 */
void ensure_d2d_renderer_created(t_lua_environment*);

/**
 * \brief Invalidates all visual layers of all running Lua scripts.
 * \remarks Must be called from the UI thread.
 */
void invalidate_visuals();

/**
 * \brief Forces an immediate repaint of all visual layers of all running Lua scripts.
 * \remarks Must be called from the UI thread.
 */
void repaint_visuals();

extern std::vector<t_lua_environment*> g_lua_environments;

/**
 * \brief The controller data at time of the last input poll
 */
extern core_buttons last_controller_data[4];

/**
 * \brief The modified control data to be pushed the next frame
 */
extern core_buttons new_controller_data[4];

/**
 * \brief Whether the <c>new_controller_data</c> of a controller should be pushed the next frame
 */
extern bool overwrite_controller_data[4];

/**
 * \brief Amount of call_input calls.
 */
extern size_t g_input_count;

/**
 * \brief Gets the Lua environment associated with a lua state.
 */
t_lua_environment* get_lua_class(lua_State* lua_state);
