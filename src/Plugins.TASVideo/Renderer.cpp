/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "Renderer.hpp"
#include "OpenGL.hpp"
#include "Combiner.hpp"
#include "FrameBuffer.hpp"
#include <algorithm>
#include <cstring>
#include <utility>

Renderer renderer;
SPDrawState renderSP;
gDPInfo renderDP;
VIInfo renderVI{320, 240};
std::array<u64, 512> renderTMEM;

Renderer::~Renderer()
{
    if (thread)
    {
        Render_SendMessage(RENDER_CLOSE);
        thread->join();
    }
}

static void flush()
{
    if (OGL.numTriangles) OGL_DrawTriangles();
}

template <typename T> static bool differs(const T &a, const T &b)
{
    return std::memcmp(&a, &b, sizeof(T)) != 0;
}

static const DrawPipelineState *currentState;

static void diff(const DrawPipelineState &previous, const DrawPipelineState &state, u32 &dpChanged, u32 &spChanged)
{
    const auto &a = previous.dp;
    const auto &b = state.dp;
    if (a.otherMode._u64 != b.otherMode._u64)
    {
        dpChanged |= CHANGED_RENDERMODE | CHANGED_CYCLETYPE | CHANGED_ALPHACOMPARE;
        if (a.otherMode.textureLUT != b.otherMode.textureLUT || a.otherMode.textureFilter != b.otherMode.textureFilter)
            dpChanged |= CHANGED_TILE;
    }
    if (a.combine.mux != b.combine.mux) dpChanged |= CHANGED_COMBINE;
    if (differs(a.tiles, b.tiles) || differs(a.paletteCRC16, b.paletteCRC16) || a.paletteCRC256 != b.paletteCRC256)
        dpChanged |= CHANGED_TILE;
    if (differs(a.fogColor, b.fogColor)) dpChanged |= CHANGED_FOGCOLOR;
    if (differs(a.blendColor, b.blendColor)) dpChanged |= CHANGED_ALPHACOMPARE;
    if (differs(a.envColor, b.envColor) || differs(a.primColor, b.primColor) || differs(a.key, b.key) ||
        differs(a.convert, b.convert))
        dpChanged |= CHANGED_COMBINE_COLORS;
    if (differs(a.scissor, b.scissor)) dpChanged |= CHANGED_SCISSOR;
    if (a.textureMode != b.textureMode || differs(a.texRect, b.texRect)) spChanged |= CHANGED_TEXTURE;

    const auto &c = previous.sp;
    const auto &d = state.sp;
    if (differs(c.texture, d.texture) || differs(c.bgImage, d.bgImage) || previous.tile0 != state.tile0 ||
        previous.tile1 != state.tile1)
        spChanged |= CHANGED_TEXTURE;
    if (differs(c.viewport, d.viewport)) spChanged |= CHANGED_VIEWPORT;
    if (c.geometryMode != d.geometryMode || previous.cullBoth != state.cullBoth || previous.cullBack != state.cullBack)
        spChanged |= CHANGED_GEOMETRYMODE;
}

static void process(const DrawPipelineState &state)
{
    const auto *previous = std::exchange(currentState, &state);
    u32 dpChanged = 0;
    u32 spChanged = 0;
    if (previous)
        diff(*previous, state, dpChanged, spChanged);
    else
        dpChanged = spChanged = ~0u;

    if (dpChanged || spChanged) flush();

    const bool tilesChanged = dpChanged & CHANGED_TILE;
    std::array<gDPTile, 8> tiles;
    if (!tilesChanged) std::ranges::copy(renderDP.tiles, tiles.begin());
    static_cast<DPPipelineState &>(renderDP) = state.dp;
    if (!tilesChanged) std::ranges::copy(tiles, renderDP.tiles);

    renderDP.loadTile = &renderDP.tiles[state.loadTile];
    renderSP.textureTile[0] = &renderDP.tiles[state.tile0];
    renderSP.textureTile[1] = &renderDP.tiles[state.tile1];
    static_cast<SPPipelineState &>(renderSP) = state.sp;
    renderer.cullBoth = state.cullBoth;
    renderer.cullBack = state.cullBack;
    auto &scissor = renderDP.scissor;
    if (OGL.ignoreScissor && scissor.ulx == 0 && scissor.uly == 8 && scissor.lrx == 320 && scissor.lry == 232)
    {
        scissor.uly = 0;
        scissor.lry = 240;
    }
    renderDP.changed |= dpChanged;
    renderSP.changed |= spChanged;
}
static void process(const DrawList &list, const DrawTmemWrite &write)
{
    flush();
    std::copy_n(list.tmem.begin() + write.data, write.count, renderTMEM.begin() + write.offset);
    renderDP.changed |= CHANGED_TMEM;
}
static void process(const DrawList &list, const DrawBackground &background)
{
    flush();
    renderer.background = std::span(list.bytes).subspan(background.offset, background.size);
    renderSP.changed |= CHANGED_TEXTURE;
}
static bool spriteProjection;
static void process(DrawControl control)
{
    flush();
    switch (control)
    {
    case DrawControl::ClearDepth:
        OGL_ClearDepthBuffer();
        break;
    case DrawControl::DisableOffset:
        glDisable(GL_POLYGON_OFFSET_FILL);
        break;
    case DrawControl::EnableOffset:
        glEnable(GL_POLYGON_OFFSET_FILL);
        break;
    case DrawControl::SpriteProjection:
        spriteProjection = true;
        OGL_SetOrthoProjection(0, renderVI.width, renderVI.height, 0, 0, 32767);
        break;
    case DrawControl::IdentityProjection:
        spriteProjection = false;
        OGL_SetIdentityProjection();
        break;
    }
}
static void process(const DrawList &list, const DrawTriangles &triangles)
{
    const bool scale2D = !spriteProjection;
    for (u32 i = 0; i < triangles.count; i += 3)
    {
        std::array<DrawVertex, 3> triangle;
        std::copy_n(list.vertices.begin() + triangles.first + i, 3, triangle.begin());
        for (auto &vertex : triangle)
            if (scale2D && !(OGL.stretchBackgrounds && vertex.background) &&
                !(vertex.fullscreen && OGL.stretchFullscreenRects))
                vertex.x *= OGL.widescreenScale;
        OGL_AddTriangle(triangle.data());
    }
}
static void process(const DrawList &list, const DrawLine &line)
{
    flush();
    std::array<DrawVertex, 2> vertices;
    std::copy_n(list.vertices.begin() + line.first, 2, vertices.begin());
    for (auto &vertex : vertices)
        if (!(OGL.stretchBackgrounds && vertex.background)) vertex.x *= OGL.widescreenScale;
    OGL_DrawLine(vertices.data(), line.width);
}
static void process(DrawRect rect)
{
    flush();
    if (OGL.ignoreScissor && rect.x0 == 0 && rect.x1 == 320)
    {
        if ((rect.y0 == 0 && rect.y1 == 8) || (rect.y0 == 232 && rect.y1 == 240)) return;
        if (rect.y0 == 8 && rect.y1 == 232 && renderDP.otherMode.cycleType == G_CYC_FILL)
        {
            rect.y0 = 0;
            rect.y1 = 240;
        }
    }
    OGL_DrawRect(rect.x0, rect.y0, rect.x1, rect.y1, rect.color.data());
}
static void process(const DrawTexturedRect &rect)
{
    flush();
    OGL_DrawTexturedRect(rect.x0, rect.y0, rect.x1, rect.y1, rect.s, rect.t, rect.lrs, rect.lrt, rect.flip);
}
static void Render_List(const DrawList &list)
{
    if (!OGL.context_initialized) return;
    renderVI = {list.width, list.height};
    renderer.listNumber = list.number;
    currentState = nullptr;
    OGL_UpdateScale();
    if (OGL.clear_override) OGL_ClearFramebuffer(GL_COLOR_BUFFER_BIT);
    for (const auto &command : list.commands)
    {
        std::visit(
            [&list](const auto &value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, DrawUseState>)
                    process(list.states[value.index]);
                else if constexpr (requires { process(list, value); })
                    process(list, value);
                else
                    process(value);
            },
            command);
    }
    flush();
    renderer.background = {};
}

void Render_SendMessage(u32 command, void *pixels)
{
    std::future<void> completed;
    {
        std::lock_guard lock(renderer.mutex);
        auto &message = renderer.messages.emplace_back(command, pixels);
        completed = message.completed.get_future();
    }
    renderer.msg_available.notify_one();
    completed.wait();
}

static void recycle(DrawList *list)
{
    std::lock_guard lock(renderer.poolMutex);
    if (renderer.pool.size() < 2)
        renderer.pool.emplace_back(list);
    else
        delete list;
}

void Render_Submit(DrawList &list)
{
    std::unique_ptr<DrawList> next;
    {
        std::lock_guard lock(renderer.poolMutex);
        if (!renderer.pool.empty())
        {
            next = std::move(renderer.pool.back());
            renderer.pool.pop_back();
        }
    }
    if (!next) next = std::make_unique<DrawList>();

    std::swap(*next, list);
    renderer.latest.store(std::shared_ptr<const DrawList>(next.release(), recycle));
}

void Render_ThreadProc()
{
    hqxInit();
    std::weak_ptr<const DrawList> rendered;

    while (true)
    {
        RenderMessage message;
        {
            std::unique_lock lock(renderer.mutex);
            renderer.msg_available.wait(lock, [] { return !renderer.messages.empty(); });
            message = std::move(renderer.messages.front());
            renderer.messages.pop_front();
        }

        const auto command = message.command;
        if (command == RENDER_CLOSE)
        {
            renderer.latest.store(nullptr);
            OGL_Stop();
            OGL_DestroyContext();
            message.completed.set_value();
            return;
        }

        switch (command)
        {
        case RENDER_START:
            renderer.latest.store(nullptr);
            OGL_Start();
            OGL_ResizeWindow();
            break;
        case RENDER_RESTART:
            rendered.reset();
            OGL_Stop();
            OGL_DestroyContext();
            OGL_Start();
            OGL_ResizeWindow();
            break;
        case RENDER_DESTROYTEXTURES:
            Combiner_Destroy();
            FrameBuffer_Destroy();
            TextureCache_Destroy();
            break;
        case RENDER_INITTEXTURES:
            FrameBuffer_Init();
            TextureCache_Init();
            Combiner_Init();
            renderSP.changed = renderDP.changed = 0xFFFFFFFF;
            break;
        case RENDER_READPIXELS: {
            const auto list = renderer.latest.load();
            if (list && list != rendered.lock())
            {
                Render_List(*list);
                rendered = list;
            }
            OGL_ReadPixels(message.pixels);
            break;
        }
        case RENDER_BLACKOUT:
            renderer.latest.store(nullptr);
            OGL_ClearFramebuffer(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            break;
        default:
            break;
        }
        message.completed.set_value();
    }
}
