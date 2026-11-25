/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DrawList.hpp"
#include "N64.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <optional>

void (*draw_sink)(DrawList &);
static DrawList list;
static u32 listNumber;
static bool recording;
static bool stateChanged;
struct BackgroundSource
{
    u32 address;
    uint64_t size;

    bool operator==(const BackgroundSource &) const = default;
};
static std::optional<BackgroundSource> lastBackground;

static constexpr u32 TMEM_WORDS = sizeof(TMEM) / sizeof(TMEM[0]);

static void writeTmem(u32 begin, u32 end)
{
    list.commands.emplace_back(DrawTmemWrite{u16(begin), u16(end - begin), u32(list.tmem.size())});
    list.tmem.insert(list.tmem.end(), TMEM + begin, TMEM + end);
}

// RDRAM is stored as native 32-bit words, so byte `a` lives at RDRAM[a ^ 3].
static void copyRdram(u8 *dst, u32 address, size_t size)
{
    size_t i = 0;
    for (; i < size && ((address + i) & 3); ++i) dst[i] = RDRAM[(address + i) ^ 3];
    for (; i + 4 <= size; i += 4)
    {
        u32 word;
        std::memcpy(&word, RDRAM + address + i, sizeof(word));
        word = std::byteswap(word);
        std::memcpy(dst + i, &word, sizeof(word));
    }
    for (; i < size; ++i) dst[i] = RDRAM[(address + i) ^ 3];
}

void DrawList_StateChanged()
{
    stateChanged = true;
}

void DrawList_TmemWritten(u32 begin, u32 end)
{
    if (!recording) return;
    end = std::min(end, TMEM_WORDS);
    if (begin < end) writeTmem(begin, end);
}

void DrawList_Reset()
{
    list = {};
    listNumber = 0;
}
void DrawList_Begin(bool record)
{
    recording = record;
    list.clear();
    lastBackground.reset();
    stateChanged = true;
    if (recording) writeTmem(0, TMEM_WORDS);
}
bool DrawList_IsRecording()
{
    return recording;
}
void DrawList_End(u32 width, u32 height)
{
    list.width = width;
    list.height = height;
    list.number = listNumber++;
    if (recording) draw_sink(list);
}

static void captureState()
{
    if (stateChanged)
    {
        const DrawPipelineState state{gDP, gSP, G_CULL_BOTH, G_CULL_BACK, int(gDP.loadTile - gDP.tiles),
            int(gSP.textureTile[0] - gDP.tiles), int(gSP.textureTile[1] - gDP.tiles)};
        if (list.states.empty() || std::memcmp(&list.states.back(), &state, sizeof(state)) != 0)
        {
            list.commands.emplace_back(DrawUseState{u32(list.states.size())});
            list.states.push_back(state);
        }
        stateChanged = false;
    }
    if (gDP.textureMode == TEXTUREMODE_BGIMAGE)
    {
        const auto &image = gSP.bgImage;
        const uint64_t size =
            image.size <= G_IM_SIZ_32b ? (uint64_t(image.width) * image.height << image.size) >> 1 : 0;
        const BackgroundSource source{image.address, size};
        // Graphics tasks read RDRAM synchronously, so the same range can be reused within this list.
        if (lastBackground == source) return;

        const auto offset = u32(list.bytes.size());
        u32 copied = 0;
        if (source.address < RDRAMSize && size <= RDRAMSize - source.address)
        {
            copied = u32(size);
            list.bytes.resize(offset + copied);
            copyRdram(list.bytes.data() + offset, source.address, copied);
        }
        list.commands.emplace_back(DrawBackground{offset, copied});
        lastBackground = source;
    }
}

static DrawVertex toDrawVertex(const SPVertex &v, bool fullscreen)
{
    return {v.x, v.y, v.z, v.w, v.r, v.g, v.b, v.a, v.s, v.t, v.background, fullscreen};
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
    const auto first = u32(list.vertices.size());
    list.vertices.push_back(toDrawVertex(v[a], fullscreen));
    list.vertices.push_back(toDrawVertex(v[b], fullscreen));
    list.vertices.push_back(toDrawVertex(v[c], fullscreen));
    if (auto *batch = list.commands.empty() ? nullptr : std::get_if<DrawTriangles>(&list.commands.back());
        batch && batch->first + batch->count == first)
    {
        batch->count += 3;
        return;
    }
    list.commands.emplace_back(DrawTriangles{first, 3});
}
void Draw_Line(const SPVertex *v, int a, int b, float width)
{
    if (!recording) return;
    captureState();
    const auto first = u32(list.vertices.size());
    list.vertices.push_back(toDrawVertex(v[a], false));
    list.vertices.push_back(toDrawVertex(v[b], false));
    list.commands.emplace_back(DrawLine{first, width});
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
