/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "RenderState.hpp"
#include <array>
#include <variant>
#include <vector>

struct SPVertex;

namespace BuiltinTAS::Graphics
{
using TextureMemory = std::array<u64, 512>;

struct DrawVertex
{
    float x, y, z, w;
    float r, g, b, a;
    float s, t;
};

struct DrawTriangles
{
    size_t first_vertex;
    size_t count;
    bool screen_space;
    bool disable_polygon_offset;
};

struct DrawLine
{
    DrawVertex vertices[2];
    float width;
};

struct DrawRectangle
{
    s32 left, top, right, bottom;
    std::array<float, 4> color;
};

struct DrawTexturedRectangle
{
    float left, top, right, bottom;
    float s, t, end_s, end_t;
    bool flip;
};

struct ClearDepth
{
};

struct DrawCommand
{
    u32 state;
    std::variant<DrawTriangles, DrawLine, DrawRectangle, DrawTexturedRectangle, ClearDepth> draw;
};

struct DrawList
{
    u32 width, height;
    u32 display_list;
    std::vector<RenderState> states;
    std::vector<TextureMemory> texture_memory;
    std::vector<DrawVertex> vertices;
    std::vector<DrawCommand> commands;
};

void begin_draw_list(u32 width, u32 height, u32 display_list);
const DrawList &end_draw_list();

void add_triangle(const SPVertex &a, const SPVertex &b, const SPVertex &c, bool screen_space = false,
    bool disable_polygon_offset = false);
void flush_triangles();
void add_line(const SPVertex &a, const SPVertex &b, float width);
void add_rectangle(DrawRectangle rectangle);
void add_textured_rectangle(DrawTexturedRectangle rectangle);
void clear_depth();
} // namespace BuiltinTAS::Graphics
