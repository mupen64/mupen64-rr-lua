/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ActionManager.h>
#include <lua/presenters/Presenter.h>

struct DGfxColor {
    float r;
    float g;
    float b;
    float a;

    bool operator==(const DGfxColor& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    D2D1::ColorF d2d_color() const {
        return D2D1::ColorF(r, g, b, a);
    }
};

  template <> struct std::hash<DGfxColor>
  {
    size_t operator()(const DGfxColor & x) const
    {
        size_t h1 = std::hash<float>()(x.r);
        size_t h2 = std::hash<float>()(x.g);
        size_t h3 = std::hash<float>()(x.b);
        size_t h4 = std::hash<float>()(x.a);
        return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1) ^ (h4 << 2);
    }
  };

/**
 * \brief Represents a Lua rendering context.
 */
struct t_lua_rendering_context
{
    // The current presenter, or null
    Presenter *presenter{};

    // The Direct2D overlay control handle
    HWND d2d_overlay_hwnd{};

    // The GDI/GDI+ overlay control handle
    HWND gdi_overlay_hwnd{};

    bool has_gdi_content{};

    HDC gdi_front_dc{};

    // The DC for GDI/GDI+ drawings
    // This DC is special, since commands can be issued to it anytime and it's never cleared
    HDC gdi_back_dc{};

    // The bitmap for GDI/GDI+ drawings
    HBITMAP gdi_bmp{};

    // Dimensions of the drawing surfaces
    D2D1_SIZE_U dc_size{};

    // The DirectWrite factory, whose lifetime is the renderer's
    IDWriteFactory *dw_factory{};

    // The cache for DirectWrite text size measurements
    lru11::Cache<DGfxColor, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> d2d_brushes{512};

    // The cache for DirectWrite text layouts
    lru11::Cache<uint64_t, Microsoft::WRL::ComPtr<IDWriteTextLayout>> dw_text_layouts{512};

    // The cache for DirectWrite text size measurements
    lru11::Cache<uint64_t, DWRITE_TEXT_METRICS> dw_text_sizes{512};

    // The stack of render targets. The top is used for D2D calls.
    std::stack<ID2D1RenderTarget *> d2d_render_target_stack{};

    // Pool of GDI+ images
    std::unordered_map<size_t, Gdiplus::Bitmap *> image_pool{};

    // Amount of generated images, just used to generate uids for image pool
    size_t image_pool_index{};

    // Whether to ignore create_renderer() and ensure_d2d_renderer_created() calls. Used to avoid tearing down and
    // re-creating a renderer when stopping a script.
    bool ignore_create_renderer{};

    HDC loadscreen_dc{};
    HBITMAP loadscreen_bmp{};

    HBRUSH brush{};
    HPEN pen{};
    HFONT font{};
    COLORREF col, bkcol{};
    int bkmode{};
};

struct t_action_param_meta
{
    uintptr_t *validator{};
    uintptr_t *get_initial_value{};
    uintptr_t *get_hints{};
};

/**
 * \brief Describes a Lua instance.
 */
struct t_lua_environment
{
    using destroying_func = std::function<void(const t_lua_environment *env)>;
    using print_func = std::function<void(const t_lua_environment *env, const std::wstring &text)>;

    std::filesystem::path path;
    lua_State *L;
    std::shared_ptr<t_lua_rendering_context> rctx;
    bool started{};

    // All the actions registered by the script. Stored so we can remove them when the script is destroyed.
    std::vector<ActionManager::action_path> registered_actions{};

    std::unordered_map<std::wstring, std::vector<t_action_param_meta>> param_meta_map;

    destroying_func destroying{};

    print_func print{};
};

/**
 * \brief Represents the arguments for a key event callback. See `KeyEventArgs` in `api.lua`.
 */
struct t_lua_key_event_args
{
    std::optional<uint64_t> keycode;
    bool ctrl{};
    bool alt{};
    bool shift{};
    bool meta{};
    std::optional<bool> pressed;
    std::optional<std::wstring> text;
    bool repeat{};
};

typedef struct
{
    uint64_t text_hash;
    uint64_t font_name_hash;
    int font_weight;
    int font_style;
    float font_size;
    int horizontal_alignment;
    int vertical_alignment;
    float width;
    float height;
} t_text_layout_params;

typedef struct
{
    uint64_t text_hash;
    uint64_t font_name_hash;
    float font_size;
    float max_width;
    float max_height;
} t_text_measure_params;

#define D2D_GET_RECT(L, idx)                                                                                           \
    D2D1::RectF(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1), luaL_checknumber(L, idx + 2),                  \
                luaL_checknumber(L, idx + 3))

#define D2D_GET_COLOR(L, idx)                                                                                          \
    D2D1::ColorF(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1), luaL_checknumber(L, idx + 2),                 \
                 luaL_checknumber(L, idx + 3))

#define D2D_GET_POINT(L, idx)                                                                                          \
    D2D1_POINT_2F                                                                                                      \
    {                                                                                                                  \
        .x = (float)luaL_checknumber(L, idx), .y = (float)luaL_checknumber(L, idx + 1)                                 \
    }

#define D2D_GET_ELLIPSE(L, idx)                                                                                        \
    D2D1_ELLIPSE                                                                                                       \
    {                                                                                                                  \
        .point = D2D_GET_POINT(L, idx), .radiusX = (float)luaL_checknumber(L, idx + 2),                                \
        .radiusY = (float)luaL_checknumber(L, idx + 3)                                                                 \
    }

#define D2D_GET_ROUNDED_RECT(L, idx)                                                                                   \
    D2D1_ROUNDED_RECT(D2D_GET_RECT(L, idx), luaL_checknumber(L, idx + 5), luaL_checknumber(L, idx + 6))
