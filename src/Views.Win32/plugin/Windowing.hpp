/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/Graphics.hpp>

namespace SDLWindowAPI
{
const M64RRSpec::WindowingAPI &functions();

/**
 * @brief Creates a new OpenGL context using the provided surface format and makes it current.
 * @note The provided context is offscreen, and thus may not have a default framebuffer.
 *
 * @param format The surface format to use.
 * @param context_out The context will be returned through this pointer. It cannot be null.
 * @return A result. If the result is GraphicsResult::Ok, the pointer written to `context_out` will be valid.
 */
M64RRSpec::GraphicsResult new_gl_context(M64RRSpec::GLSurfaceFormat *format, M64RRSpec::GLContext **context_out);
/**
 * @brief Frees the provided OpenGL context.
 * @return Frees OpenGL context.
 */
M64RRSpec::GraphicsResult free_gl_context(M64RRSpec::GLContext *context);
/**
 * @brief Makes the provided OpenGL context current.
 * @note OpenGL is single threaded. Calls to this function should occur from at most one thread.
 */
M64RRSpec::GraphicsResult gl_make_current(M64RRSpec::GLContext *context);
/**
 * @brief Gets a pointer to an OpenGL function.
 * @note This must be called *after* a context is made current.
 */
void *gl_get_proc_address(const char *symbol);
} // namespace SDLWindowAPI