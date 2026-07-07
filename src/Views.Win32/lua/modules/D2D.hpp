/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaRenderer.hpp>
#include <lua/LuaManager.hpp>

namespace LuaCore::D2D
{
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

typedef struct
{
    float r;
    float g;
    float b;
    float a;
} t_d2d_color;

typedef struct
{
    ID2D1Bitmap *bmp;
    D2D1_RECT_F destination_rectangle;
    D2D1_RECT_F source_rectangle;
    t_d2d_color color;
    int interpolation;
} t_draw_image_params;

#define D2D_GET_RECT(L, idx)                                                                                           \
    BLRect(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1),                                                     \
           luaL_checknumber(L, idx + 2) - luaL_checknumber(L, idx),                                                    \
           luaL_checknumber(L, idx + 3) - luaL_checknumber(L, idx + 1))

#define D2D_GET_COLOR(L, idx)                                                                                          \
    BLRgba(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1), luaL_checknumber(L, idx + 2),                       \
           luaL_checknumber(L, idx + 3))

#define D2D_GET_POINT(L, idx) BLPoint(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1))

#define D2D_GET_ELLIPSE(L, idx)                                                                                        \
    BLEllipse(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1), luaL_checknumber(L, idx + 2),                    \
              luaL_checknumber(L, idx + 3))

#define D2D_GET_ROUNDED_RECT(L, idx)                                                                                   \
    BLRoundRect(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1), luaL_checknumber(L, idx + 2),                  \
                luaL_checknumber(L, idx + 3), luaL_checknumber(L, idx + 4), luaL_checknumber(L, idx + 5))

static t_draw_image_params check_draw_image_params(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);

    t_draw_image_params params{};

    lua_getfield(L, index, "identifier");
    params.bmp = (ID2D1Bitmap *)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    // D2D1_SIZE_U bmp_size = params.bmp->GetPixelSize();
    D2D1_SIZE_U bmp_size = {1, 1};

    lua_getfield(L, index, "destx1");
    float destx1 = luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "desty1");
    float desty1 = luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "destx2");
    float destx2 = lua_isnoneornil(L, -1) ? destx1 + bmp_size.width : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "desty2");
    float desty2 = lua_isnoneornil(L, -1) ? desty1 + bmp_size.height : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "srcx1");
    float srcx1 = lua_isnoneornil(L, -1) ? 0 : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "srcy1");
    float srcy1 = lua_isnoneornil(L, -1) ? 0 : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "srcx2");
    float srcx2 = lua_isnoneornil(L, -1) ? srcx1 + bmp_size.width : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "srcy2");
    float srcy2 = lua_isnoneornil(L, -1) ? srcy1 + bmp_size.height : lua_tonumber(L, -1);
    lua_pop(L, 1);

    params.destination_rectangle = D2D1::RectF(destx1, desty1, destx2, desty2);
    params.source_rectangle = D2D1::RectF(srcx1, srcy1, srcx2, srcy2);

    lua_getfield(L, index, "interpolation");
    params.interpolation = lua_isnoneornil(L, -1) ? 1 : lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "color");
    params.color = {1.0f, 1.0f, 1.0f, 1.0f};
    if (!lua_isnoneornil(L, -1))
    {
        lua_getfield(L, -1, "r");
        params.color.r = lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "g");
        params.color.g = lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "b");
        params.color.b = lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "a");
        params.color.a = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return params;
}

static int get_target_fps(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    if (lua->rctx.target_fps.has_value())
        lua_pushnumber(L, lua->rctx.target_fps.value());
    else
        lua_pushnil(L);

    return 1;
}

static int set_target_fps(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    std::optional<float> fps;
    if (!lua_isnoneornil(L, 1)) fps = (float)luaL_checknumber(L, 1);

    LuaRenderer::set_target_fps(&lua->rctx, fps);

    return 0;
}

static int create_brush(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto color = D2D_GET_COLOR(L, 1);

    uint64_t brush_id = ++lua->rctx.brush_counter;
    lua->rctx.brush_colors[brush_id] = color;

    lua_pushinteger(L, brush_id);
    return 1;
}

static int free_brush(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    auto id = (uint64_t)luaL_checkinteger(L, 1);

    return 0;
}

static int clear(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto color = D2D_GET_COLOR(L, 1);

    BLRectI rect{0, 0, (int)lua->rctx.dc_size.width, (int)lua->rctx.dc_size.height};

    if (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0)
        lua->rctx.bl_ctx.clearRect(rect);
    else
        lua->rctx.bl_ctx.fillRect(rect, color);

    return 0;
}

static int fill_rectangle(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto rectangle = D2D_GET_RECT(L, 1);
    auto brush_id = (uint64_t)luaL_checkinteger(L, 5);

    const auto &color = lua->rctx.brush_colors.at(brush_id);
    lua->rctx.bl_ctx.setFillStyle(color);
    lua->rctx.bl_ctx.fillRect(rectangle);

    return 0;
}

static int draw_rectangle(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto rectangle = D2D_GET_RECT(L, 1);
    float thickness = luaL_checknumber(L, 5);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 6);

    return 0;
}

static int fill_ellipse(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto ellipse = D2D_GET_ELLIPSE(L, 1);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 5);

    return 0;
}

static int draw_ellipse(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto ellipse = D2D_GET_ELLIPSE(L, 1);
    float thickness = luaL_checknumber(L, 5);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 6);

    return 0;
}

static int draw_line(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto point_a = D2D_GET_POINT(L, 1);
    const auto point_b = D2D_GET_POINT(L, 3);
    float thickness = luaL_checknumber(L, 5);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 6);

    return 0;
}

static int draw_text(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto rectangle = D2D_GET_RECT(L, 1);
    auto text = std::string(luaL_checkstring(L, 5));
    auto font_name = std::string(luaL_checkstring(L, 6));
    auto font_size = static_cast<float>(luaL_checknumber(L, 7));
    auto font_weight = static_cast<int>(luaL_checknumber(L, 8));
    auto font_style = static_cast<int>(luaL_checkinteger(L, 9));
    auto horizontal_alignment = static_cast<int>(luaL_checkinteger(L, 10));
    auto vertical_alignment = static_cast<int>(luaL_checkinteger(L, 11));
    int options = luaL_checkinteger(L, 12);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 13);

    return 0;
}

static int set_text_antialias_mode(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    float mode = luaL_checkinteger(L, 1);

    return 0;
}

static int set_antialias_mode(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    float mode = luaL_checkinteger(L, 1);

    return 0;
}

static int measure_text(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    std::wstring text = IOUtils::to_wide_string(std::string(luaL_checkstring(L, 1)));
    std::string font_name = std::string(luaL_checkstring(L, 2));
    float font_size = luaL_checknumber(L, 3);
    float max_width = luaL_checknumber(L, 4);
    float max_height = luaL_checknumber(L, 5);

    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "height");

    return 1;
}

static int push_clip(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto rectangle = D2D_GET_RECT(L, 1);

    return 0;
}

static int pop_clip(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    return 0;
}

static int fill_rounded_rectangle(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto rounded_rectangle = D2D_GET_ROUNDED_RECT(L, 1);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 7);

    return 0;
}

static int draw_rounded_rectangle(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto rounded_rectangle = D2D_GET_ROUNDED_RECT(L, 1);
    float thickness = luaL_checknumber(L, 7);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 8);

    return 0;
}

static int load_image(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    std::string path(luaL_checkstring(L, 1));

    lua_pushinteger(L, (uint64_t)0);
    return 1;
}

static int free_image(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    return 0;
}

static int draw_image2(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    auto params = check_draw_image_params(L, 1);
    const auto color = params.color;

    return 0;
}

static int get_image_info(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    auto bmp = (ID2D1Bitmap *)luaL_checkinteger(L, 1);

    D2D1_SIZE_U size = bmp->GetPixelSize();
    lua_newtable(L);
    lua_pushinteger(L, size.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, size.height);
    lua_setfield(L, -2, "height");

    return 1;
}

static int draw_to_image(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    float width = std::max((float)luaL_checknumber(L, 1), 1.0f);
    float height = std::max((float)luaL_checknumber(L, 2), 1.0f);

    lua_pushinteger(L, (uint64_t)0);
    return 1;
}

#undef D2D_GET_RECT
#undef D2D_GET_COLOR
#undef D2D_GET_POINT
#undef D2D_GET_ELLIPS
#undef D2D_GET_ROUNDED_RECT
} // namespace LuaCore::D2D
