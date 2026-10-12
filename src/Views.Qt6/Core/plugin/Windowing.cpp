/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Windowing.hpp"

#include <future>
#include <memory>

#include <QObject>
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QThread>

struct M64RRSpec::GLContext
{
    std::unique_ptr<QOffscreenSurface> surface;
    std::unique_ptr<QOpenGLContext> context;
};

static thread_local M64RRSpec::GLContext *g_last_context;

namespace QtWindowAPI
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

    QThread *dst_thread = QThread::currentThread();
    std::promise<std::expected<M64RRSpec::GLContext *, M64RRSpec::GraphicsResult>> promise;
    auto future = promise.get_future();

    std::println("calling into main thread ({}) from {}", (void *)qGuiApp->thread(), (void *)dst_thread);
    QMetaObject::invokeMethod(
        qGuiApp, [format, context_out, dst_thread, promise = std::move(promise)]() mutable -> void {
            QSurfaceFormat qt_format;

            // copy plugin's surface format to Qt
            if (format->major_version >= 0) qt_format.setMajorVersion(format->major_version);
            if (format->minor_version >= 0) qt_format.setMinorVersion(format->minor_version);
            switch (format->context_type)
            {
            case M64RRSpec::GLContextType::OpenGLCore:
                qt_format.setRenderableType(QSurfaceFormat::OpenGL);
                qt_format.setProfile(QSurfaceFormat::CoreProfile);
                break;
            case M64RRSpec::GLContextType::OpenGLCompat:
                qt_format.setRenderableType(QSurfaceFormat::OpenGL);
                qt_format.setProfile(QSurfaceFormat::CompatibilityProfile);
                break;
            case M64RRSpec::GLContextType::OpenGLES:
                qt_format.setRenderableType(QSurfaceFormat::OpenGLES);
                break;
            default:
                break;
            }

            if (format->red_bits >= 0) qt_format.setRedBufferSize(format->red_bits);
            if (format->green_bits >= 0) qt_format.setGreenBufferSize(format->green_bits);
            if (format->blue_bits >= 0) qt_format.setBlueBufferSize(format->blue_bits);
            if (format->alpha_bits >= 0) qt_format.setAlphaBufferSize(format->alpha_bits);

            if (format->depth_bits >= 0) qt_format.setDepthBufferSize(format->depth_bits);
            if (format->stencil_bits >= 0) qt_format.setStencilBufferSize(format->stencil_bits);

            if (format->msaa_samples >= 0) qt_format.setSamples(format->msaa_samples);

            std::println("creating surface");
            // create surface and OpenGL context
            std::unique_ptr<QOffscreenSurface> surface{new QOffscreenSurface};
            surface->setFormat(qt_format);
            surface->create();

            std::println("creating context");
            std::unique_ptr<QOpenGLContext> gl_context{new QOpenGLContext};
            gl_context->setShareContext(QOpenGLContext::globalShareContext());
            gl_context->setFormat(qt_format);
            if (!gl_context->create())
            {
                promise.set_value(std::unexpected(M64RRSpec::GraphicsResult::GL_ContextCreationFailed));
                return;
            }

            // move to destination thread
            std::println("shifting context to output thread");
            gl_context->doneCurrent();
            surface->moveToThread(dst_thread);
            gl_context->moveToThread(dst_thread);

            std::println("returning context");
            promise.set_value(new M64RRSpec::GLContext{std::move(surface), std::move(gl_context)});
        });

    auto result = future.get();
    if (!result.has_value()) return result.error();

    auto *new_context = result.value();
    new_context->context->makeCurrent(new_context->surface.get());

    *context_out = g_last_context = new_context;

    return M64RRSpec::GraphicsResult::Ok;
}
/**
 * @brief Frees the provided OpenGL context.
 * @return Frees OpenGL context.
 */
M64RRSpec::GraphicsResult free_gl_context(M64RRSpec::GLContext *context)
{
    if (context == nullptr) return M64RRSpec::GraphicsResult::InvalidArgument;

    if (context == g_last_context) g_last_context = nullptr;

    context->context->doneCurrent();
    context->context->moveToThread(qGuiApp->thread());
    context->surface->moveToThread(qGuiApp->thread());

    std::promise<void> promise;
    auto future = promise.get_future();
    QMetaObject::invokeMethod(qGuiApp, [context, promise = std::move(promise)] mutable -> void {
        delete context;
        promise.set_value();
    });

    future.get();
    return M64RRSpec::GraphicsResult::Ok;
}
/**
 * @brief Makes the provided OpenGL context current.
 * @note OpenGL is single threaded. Calls to this function should occur from at most one thread.
 */
M64RRSpec::GraphicsResult gl_make_current(M64RRSpec::GLContext *context)
{
    if (context == nullptr) return M64RRSpec::GraphicsResult::InvalidArgument;
    g_last_context = context;
    if (!context->context->makeCurrent(context->surface.get())) return M64RRSpec::GraphicsResult::MiscError;
    return M64RRSpec::GraphicsResult::Ok;
}
/**
 * @brief Gets a pointer to an OpenGL function.
 * @note This must be called *after* a context is made current.
 */
void *gl_get_proc_address(const char *symbol)
{
    if (!g_last_context) return nullptr;
    return (void *)g_last_context->context->getProcAddress(symbol);
}
} // namespace QtWindowAPI
