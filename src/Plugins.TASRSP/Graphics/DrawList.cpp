/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DrawList.hpp"
#include "N64.hpp"
#include "gSP.hpp"

#include <algorithm>

namespace BuiltinTAS::Graphics
{
static DrawList g_draw_list{};
static bool g_batch_open;

void begin_draw_list(u32 width, u32 height, u32 display_list)
{
    g_draw_list.width = width;
    g_draw_list.height = height;
    g_draw_list.display_list = display_list;
    g_draw_list.states.clear();
    g_draw_list.texture_memory.clear();
    g_draw_list.vertices.clear();
    g_draw_list.commands.clear();
    g_batch_open = false;
}

const DrawList &end_draw_list()
{
    return g_draw_list;
}

static u32 capture_state()
{
    if (g_draw_list.texture_memory.empty() || (gDP.changed & CHANGED_TMEM))
    {
        g_draw_list.texture_memory.emplace_back();
        std::ranges::copy(TMEM, g_draw_list.texture_memory.back().begin());
    }

    std::span<const u8> background_image;
    if (gDP.textureMode == TEXTUREMODE_BGIMAGE)
    {
        const auto &image = gSP.bgImage;
        if (image.address < RDRAMSize && image.size <= G_IM_SIZ_32b && image.format <= G_IM_FMT_I && image.palette < 16)
        {
            const auto row_bytes = (static_cast<u64>(image.width) << image.size) / 2;
            const auto size = row_bytes * image.height;
            if (size <= RDRAMSize - image.address)
                background_image = {&RDRAM[image.address], static_cast<size_t>(size)};
        }
    }

    const u32 state = static_cast<u32>(g_draw_list.states.size());
    g_draw_list.states.push_back({
        .sp = gSP,
        .dp = gDP,
        .sp_changed = gSP.changed & ~CHANGED_MATRIX,
        .texture_tiles = {static_cast<u32>(gSP.textureTile[0] - gDP.tiles),
            static_cast<u32>(gSP.textureTile[1] - gDP.tiles)},
        .load_tile = static_cast<u32>(gDP.loadTile - gDP.tiles),
        .texture_memory = static_cast<u32>(g_draw_list.texture_memory.size() - 1),
        .background_image = background_image,
        .depth_test = (gSP.geometryMode & G_ZBUFFER) != 0,
        .fog = (gSP.geometryMode & G_FOG) != 0,
        .cull_front = (gSP.geometryMode & G_CULL_FRONT) != 0,
        .cull_back = (gSP.geometryMode & G_CULL_BACK) != 0,
    });
    gSP.changed &= CHANGED_MATRIX;
    gDP.changed = 0;
    return state;
}

static DrawVertex draw_vertex(const SPVertex &vertex)
{
    return {vertex.x, vertex.y, vertex.z, vertex.w, vertex.r, vertex.g, vertex.b, vertex.a, vertex.s, vertex.t};
}

void add_triangle(
    const SPVertex &a, const SPVertex &b, const SPVertex &c, bool screen_space, bool disable_polygon_offset)
{
    auto *batch = g_batch_open ? &std::get<DrawTriangles>(g_draw_list.commands.back().draw) : nullptr;
    if (!batch || batch->screen_space != screen_space || batch->disable_polygon_offset != disable_polygon_offset ||
        (gSP.changed & ~CHANGED_MATRIX) || gDP.changed)
    {
        g_draw_list.commands.push_back(
            {capture_state(), DrawTriangles{g_draw_list.vertices.size(), 0, screen_space, disable_polygon_offset}});
        batch = &std::get<DrawTriangles>(g_draw_list.commands.back().draw);
        g_batch_open = true;
    }

    g_draw_list.vertices.push_back(draw_vertex(a));
    g_draw_list.vertices.push_back(draw_vertex(b));
    g_draw_list.vertices.push_back(draw_vertex(c));
    batch->count += 3;
}

void flush_triangles()
{
    g_batch_open = false;
}

void add_line(const SPVertex &a, const SPVertex &b, float width)
{
    flush_triangles();
    g_draw_list.commands.push_back({capture_state(), DrawLine{{draw_vertex(a), draw_vertex(b)}, width}});
}

void add_rectangle(DrawRectangle rectangle)
{
    flush_triangles();
    g_draw_list.commands.push_back({capture_state(), rectangle});
}

void add_textured_rectangle(DrawTexturedRectangle rectangle)
{
    flush_triangles();
    g_draw_list.commands.push_back({capture_state(), rectangle});
}

void clear_depth()
{
    flush_triangles();
    g_draw_list.commands.push_back({capture_state(), ClearDepth{}});
}
} // namespace BuiltinTAS::Graphics
