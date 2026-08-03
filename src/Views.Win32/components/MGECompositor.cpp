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

// bgra8 unorm

constexpr auto CONTROL_CLASS_NAME = L"MGECompositor";

struct t_mge_context
{
    int32_t width{};
    int32_t height{};
    void *rgba_buffer{};

    SDL_Renderer *renderer{};
    SDL_Window *window{};
    SDL_Texture *texture{};
};

static t_mge_context s_ctx{};

static void create_texture()
{
    if (s_ctx.texture) SDL_DestroyTexture(s_ctx.texture);

    g_view_logger->info(L"[MGECompositor] Creating texture: {}x{}...", s_ctx.width, s_ctx.height);

    s_ctx.texture = SDL_CreateTexture(s_ctx.renderer, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING,
                                      s_ctx.width, s_ctx.height);
    RT_ASSERT(s_ctx.texture, L"Error in SDL_CreateTexture. Check that your video driver is up-to-date.");
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    Main::handle_mouse_events(hwnd, msg, wparam, lparam);

    switch (msg)
    {
    case WM_SIZE:
        return 0;
    case WM_NCDESTROY:
        SDL_DestroyTexture(s_ctx.texture);
        SDL_DestroyRenderer(s_ctx.renderer);
        SDL_DestroyWindow(s_ctx.window);
        break;
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void set_overlay_visibility(bool visible)
{
    //     if (visible)
    //         SDL_ShowWindow(s_ctx.window);
    //     else
    //         SDL_HideWindow(s_ctx.window);
}

static LRESULT CALLBACK main_window_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                                  DWORD_PTR data)
{
    switch (msg)
    {
    case WM_ACTIVATE:
        switch (LOWORD(wparam))
        {
        case WA_ACTIVE:
        case WA_CLICKACTIVE:
            set_overlay_visibility(true);
            break;
        case WA_INACTIVE:
            set_overlay_visibility(false);
            break;
        default:
            break;
        }
        break;
    case WM_MOVE:
    case WM_SIZE: {
        RECT rc = Main::get_overlay_rect();
        SDL_SetWindowSize(s_ctx.window, rc.right - rc.left, rc.bottom - rc.top);
        break;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, main_window_subclass_proc, id);
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

void MGECompositor::create(HWND hwnd)
{
    RECT rc = Main::get_overlay_rect();

    auto result = SDL_CreateWindowAndRenderer("TASVideo", rc.right - rc.left, rc.bottom - rc.top, SDL_WINDOW_OPENGL,
                                              &s_ctx.window, &s_ctx.renderer);
    RT_ASSERT(result, L"Error in SDL_CreateWindowAndRenderer. Check that your video driver is up-to-date.");

    SetWindowSubclass(g_main_ctx.hwnd, main_window_subclass_proc, 0, 0);
}

void MGECompositor::init()
{
    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.lpfnWndProc = (WNDPROC)wndproc;
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = CONTROL_CLASS_NAME;
    RegisterClass(&wndclass);

    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, [](const std::any &data) {
        const auto value = std::any_cast<bool>(data);
        const auto visible = value && PluginUtil::mge_available();

        // if (visible)
        //     SDL_ShowWindow(s_ctx.window);
        // else
        //     SDL_HideWindow(s_ctx.window);
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
        create_texture();
    }

    PluginUtil::read_video(s_ctx.rgba_buffer);

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

    // ensure_texture_exists_with_size(mge_context.width, mge_context.height);
    // upload_rgb32_buffer();
    // render_and_present();
}
