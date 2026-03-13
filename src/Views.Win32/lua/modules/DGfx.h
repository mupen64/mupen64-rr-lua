/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Messenger.h>
#include <Plugin.h>
#include <components/Statusbar.h>
#include <lua/LuaCallbacks.h>

namespace LuaCore::DGfx
{

enum class DgfxCommandType : uint8_t
{
    Clear = 1,
    Text = 2,
};

struct DgfxCommand
{
    DgfxCommandType type{};
    virtual ~DgfxCommand() = default;
};

struct DgfxClearCommand final : DgfxCommand
{
    DGfxColor color{};
    DgfxClearCommand() { type = DgfxCommandType::Clear; }
};

struct DgfxTextCommand final : DgfxCommand
{
    DGfxColor color{};
    float x{};
    float y{};
    float w{};
    float h{};
    std::string text{};
    std::string font_name{};
    float font_size{};
    int32_t font_weight{};
    int32_t font_style{};
    int32_t horizontal_alignment{};
    int32_t vertical_alignment{};
    uint32_t options{};
    DgfxTextCommand() { type = DgfxCommandType::Text; }
};

using DgfxCommandVariant = std::variant<DgfxClearCommand, DgfxTextCommand>;

static DGfxColor parse_color(lua_State *L)
{
    DGfxColor color{};

    lua_rawgeti(L, -1, 1);
    color.r = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    color.g = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 3);
    color.b = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 4);
    color.a = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    return color;
}

static void parse_clear(lua_State *L, int i, DgfxClearCommand &out)
{
    lua_getfield(L, i, "color");
    out.color = parse_color(L);
    lua_pop(L, 1);
}

static void parse_text(lua_State *L, int i, DgfxTextCommand &out)
{
    lua_getfield(L, i, "color");
    out.color = parse_color(L);
    lua_pop(L, 1);

    lua_getfield(L, i, "x");
    out.x = static_cast<int32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "y");
    out.y = static_cast<int32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "w");
    out.w = static_cast<int32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "h");
    out.h = static_cast<int32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "text");
    out.text = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, i, "font_name");
    out.font_name = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, i, "font_size");
    out.font_size = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "font_weight");
    out.font_weight = static_cast<int32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "font_style");
    out.font_style = static_cast<int32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "horizontal_alignment");
    out.horizontal_alignment = static_cast<int32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "vertical_alignment");
    out.vertical_alignment = static_cast<int32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, i, "options");
    out.options = static_cast<uint32_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);
}

static void clear_cmd(t_lua_environment &lua, const DgfxClearCommand &cmd)
{
    lua.rctx.d2d_render_target_stack.top()->Clear(lua.rctx.presenter->adjust_clear_color(cmd.color.d2d_color()));
}

static void text_cmd(t_lua_environment &lua, const DgfxTextCommand &cmd)
{
    uint64_t font_name_hash = xxh64::hash(cmd.font_name.data(), cmd.font_name.size(), 0);
    uint64_t text_hash = xxh64::hash(cmd.text.data(), cmd.text.size(), 0);

    t_text_layout_params params = {
        .text_hash = text_hash,
        .font_name_hash = font_name_hash,
        .font_weight = cmd.font_weight,
        .font_style = cmd.font_style,
        .font_size = cmd.font_size,
        .horizontal_alignment = cmd.horizontal_alignment,
        .vertical_alignment = cmd.vertical_alignment,
        .width = cmd.w,
        .height = cmd.h,
    };

    if (params.width < 0.0f || params.height < 0.0f)
    {
        return;
    }

    uint64_t params_hash = xxh64::hash((const char *)&params, sizeof(params), 0);

    if (!lua.rctx.dw_text_layouts.contains(params_hash))
    {
        IDWriteTextFormat *text_format;

        lua.rctx.dw_factory->CreateTextFormat(IOUtils::to_wide_string(cmd.font_name).c_str(), nullptr,
                                              static_cast<DWRITE_FONT_WEIGHT>(cmd.font_weight),
                                              static_cast<DWRITE_FONT_STYLE>(cmd.font_style),
                                              DWRITE_FONT_STRETCH_NORMAL, cmd.font_size, L"", &text_format);

        text_format->SetTextAlignment(static_cast<DWRITE_TEXT_ALIGNMENT>(cmd.horizontal_alignment));
        text_format->SetParagraphAlignment(static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(cmd.vertical_alignment));

        IDWriteTextLayout *text_layout;

        auto wtext = IOUtils::to_wide_string(cmd.text);
        lua.rctx.dw_factory->CreateTextLayout(wtext.c_str(), wtext.length(), text_format, cmd.w, cmd.h, &text_layout);

        lua.rctx.dw_text_layouts.add(params_hash, text_layout);
        text_format->Release();
    }

    const auto brush = D2D::get_solid_color_brush(&lua, cmd.color);
    auto layout = lua.rctx.dw_text_layouts.get(params_hash);
    lua.rctx.d2d_render_target_stack.top()->DrawTextLayout(
        {
            .x = cmd.x,
            .y = cmd.y,
        },
        layout.value(), brush, static_cast<D2D1_DRAW_TEXT_OPTIONS>(cmd.options));
}

static int enqueue(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    luaL_checktype(L, 1, LUA_TTABLE);

    std::vector<DgfxCommandVariant> cmds;

    lua_pushnil(L);
    while (lua_next(L, 1) != 0)
    {
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            continue;
        }

        lua_getfield(L, -1, "type");
        const auto type = (DgfxCommandType)lua_tointeger(L, -1);
        lua_pop(L, 1);

        switch (type)
        {
        case DgfxCommandType::Clear: {
            DgfxClearCommand c;
            parse_clear(L, lua_gettop(L), c);
            cmds.emplace_back(std::move(c));
            break;
        }
        case DgfxCommandType::Text: {
            DgfxTextCommand t;
            parse_text(L, lua_gettop(L), t);
            cmds.emplace_back(std::move(t));
            break;
        }
        }

        lua_pop(L, 1);
    }

    for (auto &v : cmds)
    {
        if (auto *c = std::get_if<DgfxClearCommand>(&v))
            clear_cmd(*lua, *c);
        else if (auto *c = std::get_if<DgfxTextCommand>(&v))
            text_cmd(*lua, *c);
    }

    return 0;
}

} // namespace LuaCore::DGfx
