/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include "LuaCanvas.hpp"

#include <QImage>
#include <QPainterPath>
#include <QRectF>
#include <QTransform>
#include <QVector>

#include <memory>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

class LuaInstance;

namespace LuaPainter
{
constexpr const char *PAINTER_MT = "mupen64.Painter";
constexpr const char *IMAGE_MT = "mupen64.PainterImage";

// Off-screen drawable image (painter.new_image/load_image), reference-counted like Win32's.
struct Image
{
    std::shared_ptr<QImage> bitmap;
};

// Per-draw state, resolved into commands when the script records them (like the Win32 port).
struct State
{
    QTransform transform;
    qreal alpha{1.0};
    int clipDepth{0};
    QPainterPath path;
    QVector<LuaTextRun> texts; // queued by text(), drawn by fill/stroke
};

struct Painter
{
    LuaInstance *instance;
    State state;
    // when set (image:paint), drawing goes here instead of the instance's active canvas
    LuaCanvas *targetCanvas{nullptr};
};

void registerTypes(lua_State *L);
// Pushes the painter bound to the instance's current canvas.
void pushCurrent(lua_State *L, LuaInstance *instance);

// painter.* module functions
int mod_current(lua_State *L);
int mod_new_image(lua_State *L);
int mod_load_image(lua_State *L);
int mod_decode_image(lua_State *L);
int mod_measure_text(lua_State *L);
int mod_hittest_text_position(lua_State *L);
int mod_hittest_text_index(lua_State *L);
int mod_get_target_fps(lua_State *L);
int mod_set_target_fps(lua_State *L);

// Shared helpers (also used for image:paint() painters).
int checkColor(lua_State *L, int index, QColor &out);
int checkRect(lua_State *L, int index, QRectF &out);
QFont parseTextStyle(lua_State *L, int index);
} // namespace LuaPainter
