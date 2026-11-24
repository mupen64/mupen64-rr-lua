/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DrawList.hpp"
#include "N64.hpp"
#include <cstring>
#include <optional>

void (*draw_sink)(DrawList &&);
static DrawList list;
static bool recording;
static std::optional<DrawPipelineState> lastState;
static bool stateChanged;
static bool needTexture;
struct BackgroundSource
{
    u32 address;
    uint64_t size;

    bool operator==(const BackgroundSource &) const = default;
};
static std::optional<BackgroundSource> lastBackground;

void DrawList_StateChanged()
{
    stateChanged = true;
}

void DrawList_TextureChanged()
{
    needTexture = true;
}

void DrawList_Reset()
{
    list = {};
}
void DrawList_Begin(bool record)
{
    recording = record;
    list.commands.clear();
    list.states.clear();
    list.textures.clear();
    lastState.reset();
    lastBackground.reset();
    stateChanged = true;
    needTexture = true;
}
bool DrawList_IsRecording()
{
    return recording;
}
void DrawList_End(u32 width, u32 height)
{
    list.width = width;
    list.height = height;
    if (recording) draw_sink(std::move(list));
    ++list.number;
}

static void captureState()
{
    if (stateChanged)
    {
        DrawPipelineState state{gDP, gSP, G_CULL_BOTH, G_CULL_BACK, int(gDP.loadTile - gDP.tiles),
            int(gSP.textureTile[0] - gDP.tiles), int(gSP.textureTile[1] - gDP.tiles)};
        if (!lastState || std::memcmp(&*lastState, &state, sizeof(state)) != 0)
        {
            list.commands.emplace_back(DrawStateIndex{list.states.size()});
            list.states.push_back(state);
            lastState = state;
        }
        stateChanged = false;
    }
    if (needTexture)
    {
        list.commands.emplace_back(DrawTextureIndex{list.textures.size()});
        list.textures.push_back(std::to_array(TMEM));
        needTexture = false;
    }
    if (gDP.textureMode == TEXTUREMODE_BGIMAGE)
    {
        const auto &image = gSP.bgImage;
        const uint64_t size =
            image.size <= G_IM_SIZ_32b ? (uint64_t(image.width) * image.height << image.size) >> 1 : 0;
        const BackgroundSource source{image.address, size};
        // Graphics tasks read RDRAM synchronously, so the same range can be reused within this list.
        if (lastBackground == source) return;

        std::vector<u8> pixels;
        if (source.address < RDRAMSize && size <= RDRAMSize - source.address)
        {
            pixels.resize(size);
            for (size_t i = 0; i < pixels.size(); ++i) pixels[i] = RDRAM[(source.address + i) ^ 3];
        }
        list.commands.emplace_back(std::move(pixels));
        lastBackground = source;
    }
}
void Draw_Control(DrawControl control)
{
    if (!recording) return;
    if (control == DrawControl::ClearDepth) captureState();
    list.commands.emplace_back(control);
}
void Draw_Triangle(const SPVertex *v, int a, int b, int c, bool fullscreen)
{
    if (!recording) return;
    captureState();
    list.commands.emplace_back(DrawTriangle{{v[a], v[b], v[c]}, fullscreen});
}
void Draw_Line(const SPVertex *v, int a, int b, float width)
{
    if (!recording) return;
    captureState();
    list.commands.emplace_back(DrawLine{{v[a], v[b]}, width});
}
void Draw_Rect(int x0, int y0, int x1, int y1, const float *color)
{
    if (!recording) return;
    captureState();
    list.commands.emplace_back(DrawRect{x0, y0, x1, y1, {color[0], color[1], color[2], color[3]}});
}
void Draw_TexturedRect(float x0, float y0, float x1, float y1, float s, float t, float lrs, float lrt, bool flip)
{
    if (!recording) return;
    captureState();
    list.commands.emplace_back(DrawTexturedRect{x0, y0, x1, y1, s, t, lrs, lrt, flip});
}
