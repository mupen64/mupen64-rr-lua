/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaRenderer.h>
#include <lua/LuaManager.h>

namespace LuaCore::DGfx
{

// TODO: Brush caching
static void execute(const t_lua_environment &lua, const DGfxRectangleCommand &cmd)
{
    ID2D1SolidColorBrush *brush;
    lua.rctx.d2d_render_target_stack.top()->CreateSolidColorBrush(cmd.color, &brush);
    lua.rctx.d2d_render_target_stack.top()->DrawRectangle(&cmd.rectangle, brush, cmd.thickness);
    brush->Release();
}

// TODO: Brush caching
static void execute(const t_lua_environment &lua, const DGfxFilledRectangleCommand &cmd)
{
    ID2D1SolidColorBrush *brush;
    lua.rctx.d2d_render_target_stack.top()->CreateSolidColorBrush(cmd.color, &brush);
    lua.rctx.d2d_render_target_stack.top()->FillRectangle(&cmd.rectangle, brush);
    brush->Release();
}

static void draw(t_lua_environment &lua)
{
    auto &commands = lua.rctx.dgfx_commands;

    if (!commands.empty())
    {
        LuaRenderer::ensure_d2d_renderer_created(&lua.rctx);
    }

    for (const auto &cmd : commands)
    {
        switch (cmd.type)
        {
        case DGfxCommandType::Rectangle:
            execute(lua, cmd.rectangle_cmd);
            break;
        case DGfxCommandType::FilledRectangle:
            execute(lua, cmd.filled_rectangle_cmd);
            break;
        default:
            g_view_logger->warn("Unsupported DGfx command type {}", static_cast<uint8_t>(cmd.type));
            break;
        }
    }

    commands.clear();
}

static int add(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto type = luaL_checkinteger(L, 1);
    if (!lua_istable(L, 2))
    {
        return luaL_error(L, "dgfx.add: command must be a table");
    }

    const auto cmd_type = static_cast<DGfxCommandType>(type);

    lua_pushnil(L);
    while (lua_next(L, 2) != 0)
    {
        if (!lua_istable(L, -1))
        {
            return luaL_error(L, "dgfx.add: each command must be a table");
        }

        DGfxCommand cmd{};
        cmd.type = cmd_type;

        switch (cmd_type)
        {
        case DGfxCommandType::Rectangle: {
            lua_getfield(L, -1, "color");
            const auto color = lua_tointeger(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "thickness");
            const auto thickness = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "x");
            const auto x = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "y");
            const auto y = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "w");
            const auto w = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "h");
            const auto h = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            cmd.rectangle_cmd = {
                .color = D2D1::ColorF(
                    static_cast<float>((color >> 16) & 0xFF) / 255.0f, static_cast<float>((color >> 8) & 0xFF) / 255.0f,
                    static_cast<float>(color & 0xFF) / 255.0f, static_cast<float>((color >> 24) & 0xFF) / 255.0f),
                .rectangle = D2D1::RectF(x, y, x + w, y + h),
                .thickness = thickness,
            };
            break;
        }
        case DGfxCommandType::FilledRectangle: {
            lua_getfield(L, -1, "color");
            const auto color = lua_tointeger(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "x");
            const auto x = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "y");
            const auto y = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "w");
            const auto w = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "h");
            const auto h = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);

            cmd.filled_rectangle_cmd = {
                .color = D2D1::ColorF(
                    static_cast<float>((color >> 16) & 0xFF) / 255.0f, static_cast<float>((color >> 8) & 0xFF) / 255.0f,
                    static_cast<float>(color & 0xFF) / 255.0f, static_cast<float>((color >> 24) & 0xFF) / 255.0f),
                .rectangle = D2D1::RectF(x, y, x + w, y + h),
            };
            break;
        }
        default:
            lua_pop(L, 1);
            return luaL_error(L, "dgfx.add: unsupported or unknown command type");
        }

        lua->rctx.dgfx_commands.emplace_back(cmd);

        lua_pop(L, 1);
    }

    return 0;
}

static int add_range(lua_State *L)
{
    return 0;
}

} // namespace LuaCore::DGfx
