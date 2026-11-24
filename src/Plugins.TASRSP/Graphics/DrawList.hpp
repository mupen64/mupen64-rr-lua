/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Graphics.hpp"
#include "gSP.hpp"
#include <array>
#include <variant>
#include <vector>

enum class DrawControl : u8
{
    ClearDepth,
    DisableOffset,
    EnableOffset,
    SpriteProjection,
    IdentityProjection,
};

// The subset of SPVertex the renderer consumes.
struct DrawVertex
{
    f32 x, y, z, w;
    f32 r, g, b, a;
    f32 s, t;
    bool background;
    // Part of a triangle covering the whole native screen.
    bool fullscreen;
};

// Switches to states[index].
struct DrawUseState
{
    u32 index;
};
// Overwrites `count` TMEM words starting at word `offset` with tmem[data, data + count).
struct DrawTmemWrite
{
    u16 offset, count;
    u32 data;
};
// Makes bytes[offset, offset + size) the current background image.
struct DrawBackground
{
    u32 offset, size;
};
// Draws vertices[first, first + count) as a triangle list.
struct DrawTriangles
{
    u32 first, count;
};
// Draws a line between vertices[first] and vertices[first + 1].
struct DrawLine
{
    u32 first;
    f32 width;
};
struct DrawRect
{
    s32 x0, y0, x1, y1;
    std::array<f32, 4> color;
};
struct DrawTexturedRect
{
    f32 x0, y0, x1, y1, s, t, lrs, lrt;
    bool flip;
};

using DrawCommand = std::variant<DrawUseState, DrawTmemWrite, DrawBackground, DrawControl, DrawTriangles, DrawLine,
    DrawRect, DrawTexturedRect>;

struct DrawPipelineState
{
    DPPipelineState dp;
    SPPipelineState sp;
    u32 cullBoth, cullBack;
    int loadTile, tile0, tile1;
};

struct DrawList
{
    u32 width, height, number;
    std::vector<DrawCommand> commands;
    std::vector<DrawPipelineState> states;
    std::vector<DrawVertex> vertices;
    std::vector<u64> tmem;
    std::vector<u8> bytes;

    void clear()
    {
        commands.clear();
        states.clear();
        vertices.clear();
        tmem.clear();
        bytes.clear();
    }
};

extern void (*draw_sink)(DrawList &);
void DrawList_Reset();
void DrawList_StateChanged();
void DrawList_TmemWritten(u32 begin, u32 end);
void DrawList_Begin(bool record);
bool DrawList_IsRecording();
void DrawList_End(u32 width, u32 height);
void Draw_Control(DrawControl control);
void Draw_Triangle(const SPVertex *vertices, int a, int b, int c, bool fullscreen = false);
void Draw_Line(const SPVertex *vertices, int a, int b, float width);
void Draw_Rect(int x0, int y0, int x1, int y1, const float *color);
void Draw_TexturedRect(float x0, float y0, float x1, float y1, float s, float t, float lrs, float lrt, bool flip);
