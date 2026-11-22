/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "Renderer.hpp"
#include "OpenGL.hpp"

#include <BuiltinTAS/GraphicsTask.hpp>
#include <Common/MiscHelpers.hpp>
#include <condition_variable>
#include <deque>
#include <future>
#include <optional>
#include <thread>

using namespace BuiltinTAS::Graphics;

RenderSize render_size{320, 240};
RenderState render_state{};
TextureMemory render_tmem{};
u32 render_display_list{};

namespace BuiltinTAS::TASVideo
{
namespace
{
enum class MessageType
{
    Start,
    Stop,
    Restart,
    Draw,
    ReadPixels,
    BlackOut,
};

struct Message
{
    MessageType type;
    const DrawList *list;
    void *pixels;
    std::promise<void> completed;
};

void draw(const DrawTriangles &batch, const DrawList &list)
{
    if (batch.screen_space) OGL_SetOrthoProjection(0.0f, render_size.width, render_size.height, 0.0f, 0.0f, 32767.0f);

    for (size_t i = batch.first_vertex; i < batch.first_vertex + batch.count; i += 3)
    {
        const auto *triangle = &list.vertices[i];
        if (!batch.screen_space)
        {
            const auto outside = [](const DrawVertex &v, float side) { return v.x * OGL.widescreenScale * side > v.w; };
            if ((outside(triangle[0], 1) && outside(triangle[1], 1) && outside(triangle[2], 1)) ||
                (outside(triangle[0], -1) && outside(triangle[1], -1) && outside(triangle[2], -1)))
                continue;
        }
        OGL_AddTriangle(triangle, batch.screen_space ? 1.0f : OGL.widescreenScale, batch.disable_polygon_offset);
    }

    if (OGL.numTriangles > 0) OGL_DrawTriangles();
    if (batch.disable_polygon_offset) OGL_RestorePolygonOffset();
    if (batch.screen_space) OGL_SetIdentityProjection();
}

void process_draw_list(const DrawList &list)
{
    if (render_size.width != list.width || render_size.height != list.height)
    {
        render_state.sp_changed |= CHANGED_VIEWPORT;
        render_state.dp.changed |= CHANGED_SCISSOR;
    }
    render_size = {list.width, list.height};
    OGL_UpdateScale();
    render_display_list = list.display_list;

    if (OGL.clear_override)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
        glDisable(GL_SCISSOR_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        if (scissor) glEnable(GL_SCISSOR_TEST);
    }

    std::optional<u32> texture_memory;
    for (const auto &command : list.commands)
    {
        const auto &state = list.states[command.state];
        const auto sp_changed = render_state.sp_changed;
        const auto dp_changed = render_state.dp.changed;
        render_state = state;
        render_state.sp_changed |= sp_changed;
        render_state.dp.changed |= dp_changed;

        if (texture_memory != state.texture_memory)
        {
            render_tmem = list.texture_memory[state.texture_memory];
            texture_memory = state.texture_memory;
        }

        auto &scissor = render_state.dp.scissor;
        if (OGL.ignoreScissor && scissor.ulx == 0 && scissor.uly == 8 && scissor.lrx == 320 && scissor.lry == 232)
        {
            scissor.uly = 0;
            scissor.lry = 240;
        }

        std::visit(MiscHelpers::Overload{
                       [&](const DrawTriangles &batch) { draw(batch, list); },
                       [](const DrawLine &line) { OGL_DrawLine(line.vertices, line.width); },
                       [](const DrawRectangle &rect) {
                           const bool border =
                               rect.left == 0 && rect.right == 320 &&
                               ((rect.top == 0 && rect.bottom == 8) || (rect.top == 232 && rect.bottom == 240));
                           if (!OGL.ignoreScissor || !border)
                               OGL_DrawRect(rect.left, rect.top, rect.right, rect.bottom, rect.color.data());
                       },
                       [](const DrawTexturedRectangle &rect) {
                           OGL_DrawTexturedRect(rect.left, rect.top, rect.right, rect.bottom, rect.s, rect.t,
                               rect.end_s, rect.end_t, rect.flip);
                       },
                       [](ClearDepth) { OGL_ClearDepthBuffer(); },
                   },
            command.draw);
    }
}

class Renderer
{
  public:
    ~Renderer() { stop(); }

    bool running() const { return m_thread.joinable(); }

    void start()
    {
        if (!m_thread.joinable()) m_thread = std::thread([this] { run(); });
        send(MessageType::Start);
    }

    void stop()
    {
        if (!m_thread.joinable()) return;
        send(MessageType::Stop);
        m_thread.join();
    }

    void send(MessageType type, const DrawList *list = nullptr, void *pixels = nullptr)
    {
        if (!m_thread.joinable()) return;

        std::future<void> completed;
        {
            std::lock_guard lock(m_mutex);
            auto &message = m_messages.emplace_back(type, list, pixels);
            completed = message.completed.get_future();
        }
        m_msg_available.notify_one();
        completed.wait();
    }

  private:
    void run()
    {
        hqxInit();

        while (true)
        {
            Message message;
            {
                std::unique_lock lock(m_mutex);
                m_msg_available.wait(lock, [&] { return !m_messages.empty(); });
                message = std::move(m_messages.front());
                m_messages.pop_front();
            }

            switch (message.type)
            {
            case MessageType::Start:
                render_size = {320, 240};
                if (OGL_Start()) OGL_ResizeWindow();
                break;
            case MessageType::Stop:
            case MessageType::Restart:
                if (OGL.context_initialized)
                {
                    OGL_Stop();
                    OGL_DestroyContext();
                }
                if (message.type == MessageType::Restart && OGL_Start()) OGL_ResizeWindow();
                break;
            case MessageType::Draw:
                if (OGL.context_initialized) process_draw_list(*message.list);
                break;
            case MessageType::ReadPixels:
                if (OGL.context_initialized) OGL_ReadPixels(message.pixels);
                break;
            case MessageType::BlackOut:
                if (OGL.context_initialized)
                {
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                }
                break;
            }
            message.completed.set_value();
            if (message.type == MessageType::Stop) return;
        }
    }

    std::mutex m_mutex;
    std::condition_variable m_msg_available;
    std::deque<Message> m_messages;
    std::thread m_thread;
};

Renderer g_renderer;
} // namespace

bool renderer_running()
{
    return g_renderer.running();
}

void start_renderer()
{
    g_renderer.start();
}

void stop_renderer()
{
    g_renderer.stop();
}

void restart_renderer()
{
    g_renderer.send(MessageType::Restart);
}

void black_out_renderer()
{
    g_renderer.send(MessageType::BlackOut);
}

void read_pixels(void *buffer)
{
    g_renderer.send(MessageType::ReadPixels, nullptr, buffer);
}

void submit_draw_list(const DrawList &list)
{
    g_renderer.send(MessageType::Draw, &list);
}
} // namespace BuiltinTAS::TASVideo
