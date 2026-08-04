/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <components/MGECompositor.hpp>
#include <plugin/Plugin.hpp>
#include <Messenger.hpp>
#include <lua/LuaCallbacks.hpp>

struct t_mge_context
{
    int32_t width{};
    int32_t height{};
    void *rgba_buffer{};

    SDL_Renderer *renderer{};
    SDL_Window *window{};
    HWND hwnd{};
    SDL_Texture *texture{};
};

static t_mge_context s_ctx{};

static void create_texture()
{
    if (s_ctx.texture) SDL_DestroyTexture(s_ctx.texture);

    g_view_logger->info(L"[MGECompositor] Creating texture: {}x{}...", s_ctx.width, s_ctx.height);

    s_ctx.texture = SDL_CreateTexture(s_ctx.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                      s_ctx.width, s_ctx.height);
    RT_ASSERT(s_ctx.texture, L"Error in SDL_CreateTexture. Check that your video driver is up-to-date.");
}

static void render()
{
    void *pixels;
    int pitch;
    auto result = SDL_LockTexture(s_ctx.texture, nullptr, &pixels, &pitch);
    RT_ASSERT(result, L"SDL_LockTexture failed.");

    memcpy(pixels, s_ctx.rgba_buffer, s_ctx.width * s_ctx.height * 4);

    SDL_UnlockTexture(s_ctx.texture);

    result = SDL_RenderTextureRotated(s_ctx.renderer, s_ctx.texture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL);
    RT_ASSERT(result, L"SDL_RenderTextureRotated failed.");

    result = SDL_RenderPresent(s_ctx.renderer);
    RT_ASSERT(result, L"SDL_RenderPresent failed.");
}

static void set_overlay_visibility(bool visible)
{
    if (visible)
        SDL_ShowWindow(s_ctx.window);
    else
        SDL_HideWindow(s_ctx.window);
}

void MGECompositor::create(HWND hwnd)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 0);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1);
    s_ctx.window = SDL_CreateWindowWithProperties(props);
    RT_ASSERT(s_ctx.window, L"Failed to create window.");
    s_ctx.renderer = SDL_CreateRenderer(s_ctx.window, NULL);
    RT_ASSERT(s_ctx.renderer, L"Failed to create renderer. Check that your video driver is up-to-date.");

    // Make it clickthrough... SDL doesn't support this natively yet
    props = SDL_GetWindowProperties(s_ctx.window);
    s_ctx.hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    RT_ASSERT(IsWindow(s_ctx.hwnd), L"Failed to get window handle from SDL properties.");
    SetWindowLongPtr(s_ctx.hwnd, GWL_EXSTYLE,
                     GetWindowLongPtr(s_ctx.hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT | WS_EX_LAYERED);

    SetParent(s_ctx.hwnd, g_main_ctx.hwnd);
}

void MGECompositor::init()
{
    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, [](const std::any &data) {
        const auto value = std::any_cast<bool>(data);
        const auto visible = value && PluginUtil::mge_available();
        set_overlay_visibility(visible);
    });
}

void MGECompositor::update_screen()
{
    int32_t width{};
    int32_t height{};
    PluginUtil::get_video_size(&width, &height);

    if (s_ctx.width != width || s_ctx.height != height)
    {
        if (s_ctx.rgba_buffer) free(s_ctx.rgba_buffer);

        s_ctx.width = width;
        s_ctx.height = height;
        s_ctx.rgba_buffer = calloc(s_ctx.width * s_ctx.height, 4);

        SDL_SetWindowSize(s_ctx.window, s_ctx.width, s_ctx.height);
        create_texture();
    }

    PluginUtil::read_video(s_ctx.rgba_buffer);
    render();
}

void MGECompositor::get_video_size(int32_t *width, int32_t *height)
{
    if (width)
    {
        *width = s_ctx.width;
    }
    if (height)
    {
        *height = s_ctx.height;
    }
}

void MGECompositor::copy_video(void *buffer)
{
    memcpy(buffer, s_ctx.rgba_buffer, s_ctx.width * s_ctx.height * 4);
}

void MGECompositor::load_screen(void *data)
{
    memcpy(s_ctx.rgba_buffer, data, s_ctx.width * s_ctx.height * 4);
    render();
}
