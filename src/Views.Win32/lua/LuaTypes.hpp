/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <action/ActionManager.hpp>

/**
 * \brief Represents a Lua rendering context.
 */
struct t_lua_rendering_context
{
    HWND overlay_hwnd{};
    void *bl_raw;
    BLImage bl_image;
    BLContext bl_ctx;
    HDC gdi_back_dc{};
    HBITMAP gdi_bmp{};
    D2D1_SIZE_U dc_size{};

    uint64_t brush_counter;
    std::unordered_map<uint64_t, BLRgba> brush_colors;

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
    t_lua_rendering_context rctx;
    bool started{};

    // All the actions registered by the script. Stored so we can remove them when the script is destroyed.
    std::vector<ActionManager::action_path> registered_actions{};

    std::unordered_map<std::wstring, std::vector<t_action_param_meta>> param_meta_map;

    // All the breakpoints registered by the script. Stored so we can remove them when the script is destroyed.
    std::vector<std::pair<CoreBreakpointId, uintptr_t *>> active_breakpoints;

    std::vector<uintptr_t *> step_callbacks;

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
