/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Graphics.hpp"
#include "gSP.hpp"
#include <array>
#include <memory>
#include <variant>
#include <vector>

enum class DrawControl
{
    ClearDepth,
    DisableOffset,
    EnableOffset,
    SpriteProjection,
    IdentityProjection
};
struct DrawTriangle
{
    std::array<SPVertex, 3> vertices;
    bool fullscreen;
};
struct DrawLine
{
    std::array<SPVertex, 2> vertices;
    float width;
};
struct DrawRect
{
    int x0, y0, x1, y1;
    std::array<float, 4> color;
};
struct DrawTexturedRect
{
    float x0, y0, x1, y1, s, t, lrs, lrt;
    bool flip;
};

struct DrawPipelineState
{
    DPPipelineState dp;
    SPPipelineState sp;
    u32 cullBoth, cullBack;
    int loadTile, tile0, tile1;
};

using DrawCommand = std::variant<std::shared_ptr<const DrawPipelineState>, std::shared_ptr<const std::array<u64, 512>>,
    std::vector<u8>, DrawControl, DrawTriangle, DrawLine, DrawRect, DrawTexturedRect>;
struct DrawList
{
    u32 width, height, number;
    std::vector<DrawCommand> commands;
};

extern void (*draw_sink)(DrawList &&);
void DrawList_Reset();
void DrawList_TextureChanged();
void DrawList_Begin(bool record);
void DrawList_End(u32 width, u32 height);
void Draw_Control(DrawControl control);
void Draw_Triangle(const SPVertex *vertices, int a, int b, int c, bool fullscreen = false);
void Draw_Line(const SPVertex *vertices, int a, int b, float width);
void Draw_Rect(int x0, int y0, int x1, int y1, const float *color);
void Draw_TexturedRect(float x0, float y0, float x1, float y1, float s, float t, float lrs, float lrt, bool flip);
