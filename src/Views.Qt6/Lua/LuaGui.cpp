/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInternal.hpp"
#include "LuaCanvas.hpp"
#include "LuaPainter.hpp"

#include <QColor>

namespace LuaInternal
{

namespace
{

QColor parseColor(lua_State *L, int idx)
{
    // note: lua_isstring also returns true for numbers, so check integers first
    if (lua_isinteger(L, idx))
    {
        uint32_t val = static_cast<uint32_t>(lua_tointeger(L, idx));
        if ((val & 0xFF000000) != 0)
        {
            return QColor::fromRgba(val);
        }
        return QColor::fromRgb(val);
    }
    else if (lua_isstring(L, idx))
    {
        return QColor::fromString(QString::fromUtf8(lua_tostring(L, idx)));
    }
    else if (lua_istable(L, idx))
    {
        lua_rawgeti(L, idx, 1);
        int r = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 0;
        lua_pop(L, 1);
        lua_rawgeti(L, idx, 2);
        int g = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 0;
        lua_pop(L, 1);
        lua_rawgeti(L, idx, 3);
        int b = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 0;
        lua_pop(L, 1);
        lua_rawgeti(L, idx, 4);
        int a = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 255;
        lua_pop(L, 1);
        return QColor(r, g, b, a);
    }
    return QColor(Qt::white);
}

int wgui_text(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    const char *text = luaL_checkstring(L, 3);
    QColor col = parseColor(L, 4);
    inst->activeCanvas()->addText(x, y, QString::fromUtf8(text), col);
    return 0;
}

int wgui_rect(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addRect(x1, y1, x2 - x1, y2 - y1, col);
    return 0;
}

int wgui_fillrect(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addFillRect(x1, y1, x2 - x1, y2 - y1, col);
    return 0;
}

int wgui_line(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addLine(x1, y1, x2, y2, col);
    return 0;
}

int wgui_ellipse(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    qreal rx = luaL_checknumber(L, 3);
    qreal ry = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addEllipse(x, y, rx, ry, col);
    return 0;
}

int wgui_fillellipsea(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    qreal rx = luaL_checknumber(L, 3);
    qreal ry = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addFillEllipse(x, y, rx, ry, col);
    return 0;
}

int wgui_clear(lua_State *L)
{
    auto *inst = getInstance(L);
    if (inst && inst->activeCanvas())
    {
        inst->activeCanvas()->clear();
    }
    return 0;
}

int wgui_info(lua_State *L)
{
    auto *inst = getInstance(L);
    lua_newtable(L);
    if (inst && inst->activeCanvas())
    {
        lua_pushnumber(L, inst->activeCanvas()->width());
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, inst->activeCanvas()->height());
        lua_setfield(L, -2, "height");
    }
    else
    {
        lua_pushnumber(L, 640);
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, 480);
        lua_setfield(L, -2, "height");
    }
    return 1;
}

} // namespace

void registerGuiAPIs(lua_State *L)
{
    lua_newtable(L);
    const luaL_Reg wguiFuncs[] = {
        {"text", wgui_text},
        {"drawtext", wgui_text},
        {"rect", wgui_rect},
        {"fillrect", wgui_fillrect},
        {"line", wgui_line},
        {"ellipse", wgui_ellipse},
        {"fillellipsea", wgui_fillellipsea},
        {"fillrecta", wgui_fillrect},
        {"clear", wgui_clear},
        {"info", wgui_info},
        {nullptr, nullptr},
    };
    luaL_setfuncs(L, wguiFuncs, 0);
    lua_setglobal(L, "wgui");

    lua_newtable(L);
    static const luaL_Reg painterFuncs[] = {
        {"current", LuaPainter::mod_current},
        {"new_image", LuaPainter::mod_new_image},
        {"load_image", LuaPainter::mod_load_image},
        {"decode_image", LuaPainter::mod_decode_image},
        {"measure_text", LuaPainter::mod_measure_text},
        {"hittest_text_position", LuaPainter::mod_hittest_text_position},
        {"hittest_text_index", LuaPainter::mod_hittest_text_index},
        {"hitest_text_index", LuaPainter::mod_hittest_text_index}, // historical alias
        {"get_target_fps", LuaPainter::mod_get_target_fps},
        {"set_target_fps", LuaPainter::mod_set_target_fps},
        {nullptr, nullptr},
    };
    luaL_setfuncs(L, painterFuncs, 0);
    lua_setglobal(L, "painter");
}

} // namespace LuaInternal
