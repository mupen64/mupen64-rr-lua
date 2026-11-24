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

static void process(const DrawPipelineState &state)
{
    flush();
    static_cast<DPPipelineState &>(renderDP) = state.dp;
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
    renderSP.changed = renderDP.changed = ~0u;
}
static void process(const std::array<u64, 512> &texture)
{
    flush();
    renderTMEM = texture;
    renderDP.changed |= CHANGED_TMEM;
}
static void process(const std::vector<u8> &pixels)
{
    flush();
    renderer.background = &pixels;
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
static void process(DrawTriangle triangle)
{
    for (auto &vertex : triangle.vertices)
        if (!spriteProjection && !(OGL.stretchBackgrounds && vertex.background) &&
            !(triangle.fullscreen && OGL.stretchFullscreenRects))
            vertex.x *= OGL.widescreenScale;
    OGL_AddTriangle(triangle.vertices.data(), 0, 1, 2);
}
static void process(DrawLine line)
{
    flush();
    for (auto &vertex : line.vertices)
        if (!(OGL.stretchBackgrounds && vertex.background)) vertex.x *= OGL.widescreenScale;
    OGL_DrawLine(line.vertices.data(), 0, 1, line.width);
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
static void process(DrawTexturedRect rect)
{
    flush();
    OGL_DrawTexturedRect(rect.x0, rect.y0, rect.x1, rect.y1, rect.s, rect.t, rect.lrs, rect.lrt, rect.flip);
}
static void Render_List(const DrawList &list)
{
    if (!OGL.context_initialized) return;
    renderVI = {list.width, list.height};
    renderer.listNumber = list.number;
    OGL_UpdateScale();
    if (OGL.clear_override) OGL_ClearFramebuffer(GL_COLOR_BUFFER_BIT);
    for (const auto &command : list.commands)
    {
        std::visit(
            [&list](const auto &value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, DrawStateIndex>)
                    process(list.states[value.value]);
                else if constexpr (std::is_same_v<T, DrawTextureIndex>)
                    process(list.textures[value.value]);
                else
                    process(value);
            },
            command);
    }
    flush();
    renderer.background = nullptr;
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

void Render_Submit(DrawList &&list)
{
    renderer.latest.store(std::make_shared<const DrawList>(std::move(list)));
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
