/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Windowing.hpp"
#include <SDL3/SDL_video.h>

struct M64RRSpec::GLContext
{
    SDL_Window *window;
    SDL_GLContext context;

    ~GLContext()
    {
        if (context != nullptr) SDL_GL_DestroyContext(context);
        if (window != nullptr) SDL_DestroyWindow(window);
    }
};

namespace SDLWindowAPI
{

const M64RRSpec::WindowingAPI &functions()
{
    static const M64RRSpec::WindowingAPI s_instance{
        .new_gl_context = new_gl_context,
        .free_gl_context = free_gl_context,
        .gl_make_current = gl_make_current,
        .gl_get_proc_address = gl_get_proc_address,
    };
    return s_instance;
}

/**
 * @brief Creates a new OpenGL context using the provided surface format and makes it current.
 * @note The provided context is offscreen, and thus may not have a default framebuffer.
 *
 * @param format The surface format to use.
 * @param context_out The context will be returned through this pointer. It cannot be null.
 * @return A result. If the result is GraphicsResult::Ok, the pointer written to `context_out` will be valid.
 */
M64RRSpec::GraphicsResult new_gl_context(M64RRSpec::GLSurfaceFormat *format, M64RRSpec::GLContext **context_out)
{
    if (format == nullptr || context_out == nullptr) return M64RRSpec::GraphicsResult::InvalidArgument;

    if (format->major_version >= 0) SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, format->major_version);
    if (format->minor_version >= 0) SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, format->minor_version);
    switch (format->context_type)
    {
    case M64RRSpec::GLContextType::OpenGLCore:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        break;
    case M64RRSpec::GLContextType::OpenGLCompat:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        break;
    case M64RRSpec::GLContextType::OpenGLES:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        break;
    default:
        break;
    }
    if (format->red_bits >= 0) SDL_GL_SetAttribute(SDL_GL_RED_SIZE, format->red_bits);
    if (format->green_bits >= 0) SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, format->green_bits);
    if (format->blue_bits >= 0) SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, format->blue_bits);
    if (format->alpha_bits >= 0) SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, format->alpha_bits);

    if (format->depth_bits >= 0) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, format->depth_bits);
    if (format->stencil_bits >= 0) SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, format->stencil_bits);

    if (format->msaa_samples > 0)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, format->msaa_samples);
    }
    else if (format->msaa_samples == 0)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    }

    // SDL requires a hidden window for OpenGL, but raw WGL requires us to do this anyways
    // on Windows, it should be okay to call this off the main thread
    SDL_Window *window = SDL_CreateWindow("", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window == nullptr)
    {
        return M64RRSpec::GraphicsResult::MiscError;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr)
    {
        SDL_DestroyWindow(window);
        return M64RRSpec::GraphicsResult::GL_ContextCreationFailed;
    }

    *context_out = new M64RRSpec::GLContext{window, context};
    return M64RRSpec::GraphicsResult::Ok;
}
/**
 * @brief Frees the provided OpenGL context.
 * @return Frees OpenGL context.
 */
M64RRSpec::GraphicsResult free_gl_context(M64RRSpec::GLContext *context)
{
    // on Windows, it should be okay to call this off the main thread
    delete context;
    return M64RRSpec::GraphicsResult::Ok;
}
/**
 * @brief Makes the provided OpenGL context current.
 * @note OpenGL is single threaded. Calls to this function should occur from at most one thread.
 */
M64RRSpec::GraphicsResult gl_make_current(M64RRSpec::GLContext *context)
{
    bool result = SDL_GL_MakeCurrent(context->window, context->context);
    return result ? M64RRSpec::GraphicsResult::Ok : M64RRSpec::GraphicsResult::MiscError;
}
/**
 * @brief Gets a pointer to an OpenGL function.
 * @note This must be called *after* a context is made current.
 */
void *gl_get_proc_address(const char *symbol)
{
    return (void *)SDL_GL_GetProcAddress(symbol);
}
} // namespace SDLWindowAPI