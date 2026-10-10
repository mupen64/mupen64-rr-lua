#pragma once

#include <Core/Types.hpp>

namespace M64RRSpec {
    enum class GraphicsResult {
        Ok = 0,
        MiscError = 1,
    };

    enum class GLContextType : int8_t {
        // Use the default context type.
        Default = -1,
        // OpenGL core profile.
        OpenGLCore = 0,
        // OpenGL compatibility profile.
        OpenGLCompat = 1,
        // OpenGL ES.
        OpenGLES = 2,
    };

    /**
     * @brief Settings for an OpenGL context.
     */
    struct GLSurfaceFormat {
        int8_t major_version : 4 = -1;
        int8_t minor_version : 4 = -1;
        GLContextType context_type = GLContextType::Default;

        int8_t red_bits = -1;
        int8_t green_bits = -1;
        int8_t blue_bits = -1;
        int8_t alpha_bits = -1;

        int8_t depth_bits = -1;
        int8_t stencil_bits = -1;

        int8_t msaa_samples = -1;
    };

    /**
     * @brief Opaque type corresponding to an OpenGL context.
     */
    struct GLContext;

    struct WindowingAPI {
        // OPENGL
        // ===============

        /**
         * @brief Creates a new OpenGL context using the provided surface format and makes it current.
         * @note The provided context is offscreen, and thus may not have a default framebuffer.
         *
         * @param format The surface format to use.
         * @param context_out The context will be returned through this pointer. It cannot be null.
         * @return
         */
        GraphicsResult (*new_gl_context)(GLSurfaceFormat* format, GLContext** context_out);
        /**
         * @brief Frees the provided OpenGL context.
         * @return Frees OpenGL context.
         */
        GraphicsResult (*free_gl_context)(GLContext* context);
        /**
         * @brief Makes the provided OpenGL context current.
         * @note OpenGL is naturally single-threaded.
         */
        bool (*gl_make_current)(GLContext* context);
        /**
         * @brief Gets a pointer to an OpenGL function.
         * @note This must be called *after* a context is made current.
         */
        void* (*gl_get_proc_address)(const char* symbol);
    };
}
