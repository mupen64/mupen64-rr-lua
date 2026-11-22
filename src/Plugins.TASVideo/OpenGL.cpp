/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "Renderer.hpp"
#include "RDPConstants.hpp"

#include <cmath>
#include <Common/Assert.hpp>

#include "TASVideo.hpp"
#include "OpenGL.hpp"
#include "Types.hpp"
#include "Textures.hpp"
#include "Combiner.hpp"

GLInfo OGL{};

// static SDL_Window *g_sdl_window;
// static SDL_GLContext g_sdl_context;

static M64RRSpec::GLContext *g_wsi_context;

static GLuint g_fbo = 0;
static GLuint g_color_rbo = 0;
static GLuint g_aux_rbo = 0;
static GLuint g_vao = 0;
static GLuint g_vbo = 0;

static void OGL_UploadVertices(GLsizei count)
{
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(GLVertex), OGL.vertices);
}

void OGL_ReadPixels(void *buffer)
{
    if (!buffer) return;

    glFlush();
    glFinish();

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glReadPixels(0, 0, OGL.width, OGL.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    auto pixels = (uint8_t *)buffer;
    const size_t count = (size_t)OGL.width * OGL.height;
    for (size_t i = 0; i < count; i++)
    {
        const uint8_t r = pixels[i * 4 + 0];
        pixels[i * 4 + 0] = pixels[i * 4 + 2];
        pixels[i * 4 + 2] = r;
    }
}

void OGL_InitExtensions()
{
    if (OGL.isGLES)
    {
        if (!gladLoadGLES2Loader((GLADloadproc)g_wsi->gl_get_proc_address))
        {
            g_plugin->log_error("Error initializing GLAD (OpenGL ES 2.0+)");
            return;
        }
    }
    else
    {
        if (!gladLoadGLLoader((GLADloadproc)g_wsi->gl_get_proc_address))
        {
            g_plugin->log_error("Error initializing GLAD (OpenGL)");
            return;
        }
    }

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &OGL.maxTextureUnits);
    OGL.maxTextureUnits = std::min(8, OGL.maxTextureUnits); // The plugin only supports 8, and 4 is really enough
}

void OGL_SetIdentityProjection()
{
    static const float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    Combiner_SetProjection(identity);
}

void OGL_SetOrthoProjection(float left, float right, float bottom, float top, float znear, float zfar)
{
    float m[16] = {};
    m[0] = 2.0f / (right - left);
    m[5] = 2.0f / (top - bottom);
    m[10] = -2.0f / (zfar - znear);
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(zfar + znear) / (zfar - znear);
    m[15] = 1.0f;
    Combiner_SetProjection(m);
}

void OGL_DepthRange(float znear, float zfar)
{
    if (OGL.isGLES)
        glDepthRangef(znear, zfar);
    else
        glDepthRange(znear, zfar);
}

void OGL_InitStates()
{
    // setup rendering FBO/RBOs
    glGenFramebuffers(1, &g_fbo);
    glGenRenderbuffers(1, &g_color_rbo);
    glGenRenderbuffers(1, &g_aux_rbo);

    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);
    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(OGL.vertices), nullptr, GL_STREAM_DRAW);

    // configure vertex attributes
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, x));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, color.r));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, secondaryColor.r));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, s0));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, s1));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, fog));
    glEnableVertexAttribArray(5);

    glPolygonOffset(-3.0f, -3.0f);
}

void OGL_UpdateScale()
{
    OGL.adjustScreen = FALSE;
    OGL.adjustScale = 1.0f;
    OGL.adjustOffset = 0.0f;
    OGL.widescreenScale = 1.0f;

    if ((render_size.width != 0) && (render_size.height != 0) && (OGL.width != 0) && (OGL.height != 0))
    {
        const float sourceAspect = (float)render_size.width / (float)render_size.height;
        const float displayAspect = (float)OGL.width / (float)OGL.height;

        if (displayAspect > sourceAspect)
        {
            switch (OGL.aspectMode)
            {
            case AspectMode::Pillarbox:
                OGL.adjustScale = ((float)OGL.height * sourceAspect) / (float)OGL.width;
                OGL.adjustOffset = (float)OGL.width * (1.0f - OGL.adjustScale) / 2.0f;
                OGL.adjustScreen = TRUE;
                break;
            case AspectMode::Widescreen:
                OGL.widescreenScale = sourceAspect / displayAspect;
                break;
            case AspectMode::Stretch:
                break;
            }
        }
    }

    OGL.scaleX = (OGL.width * OGL.adjustScale) / (float)render_size.width;
    OGL.scaleY = OGL.height / (float)render_size.height;
}

void OGL_ResizeWindow()
{
    OGL.width = OGL.windowedWidth;
    OGL.height = OGL.windowedHeight;
    g_plugin->request_size(OGL.width, OGL.height);

    // Bind and setup RBOs
    glBindRenderbuffer(GL_RENDERBUFFER, g_color_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, (int)OGL.width, (int)OGL.height);
    glBindRenderbuffer(GL_RENDERBUFFER, g_aux_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)OGL.width, (int)OGL.height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Bind FBO and link to RBOs
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_color_rbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_aux_rbo);

    // clear main buffer to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool OGL_InitContext()
{
    M64RRSpec::GLSurfaceFormat request_format{
        .major_version = 3,
        .minor_version = 0,
        .context_type = M64RRSpec::GLContextType::OpenGLES,

        .red_bits = 8,
        .green_bits = 8,
        .blue_bits = 8,
        .alpha_bits = 0,

        .depth_bits = 24,
        .stencil_bits = 8,

        // TODO: handle MSAA
        .msaa_samples = 0,
    };
    if (g_wsi->new_gl_context(&request_format, &g_wsi_context) == M64RRSpec::GraphicsResult::Ok)
    {
        OGL.isGLES = true;
    }
    else
    {
        g_plugin->log_info("OpenGL ES 3.0 context unavailable; falling back to desktop OpenGL 4.1 Core.");
        request_format.major_version = 4;
        request_format.minor_version = 1;
        request_format.context_type = M64RRSpec::GLContextType::OpenGLCore;

        need(g_wsi->new_gl_context(&request_format, &g_wsi_context) == M64RRSpec::GraphicsResult::Ok,
            "Creating fallback desktop OpenGL context failed.");
        OGL.isGLES = false;
    }
    if (OGL.msaa > 0) g_plugin->log_warn("MSAA is not supported with FBO rendering yet; ignoring it.");

    OGL_InitExtensions();
    OGL_InitStates();
    glDisable(GL_FRAMEBUFFER_SRGB);

    g_plugin->log_info(OGL.isGLES ? "TASVideo: running on an OpenGL ES 2.0 context."
                                  : "TASVideo: running on a desktop OpenGL 4.1 Core context.");

    OGL.context_initialized = TRUE;

    return TRUE;
}

static void OGL_ClearStates()
{
    if (g_vbo != 0)
    {
        glDeleteBuffers(1, &g_vbo);
        g_vbo = 0;
    }
    if (g_vao != 0)
    {
        glDeleteVertexArrays(1, &g_vao);
        g_vao = 0;
    }
    if (g_fbo != 0)
    {
        glDeleteFramebuffers(1, &g_fbo);
        g_fbo = 0;
    }
    if (g_color_rbo != 0)
    {
        glDeleteRenderbuffers(1, &g_color_rbo);
        g_color_rbo = 0;
    }
    if (g_aux_rbo != 0)
    {
        glDeleteRenderbuffers(1, &g_aux_rbo);
        g_aux_rbo = 0;
    }
}

bool OGL_DestroyContext()
{
    OGL_ClearStates();

    g_wsi->free_gl_context(g_wsi_context);
    g_wsi_context = nullptr;

    OGL.context_initialized = FALSE;

    return TRUE;
}

bool OGL_Start()
{
    if (!OGL.context_initialized && !OGL_InitContext()) return FALSE;

    TextureCache_Init();
    Combiner_Init();

    render_state.sp_changed = render_state.dp.changed = 0xFFFFFFFF;
    OGL_UpdateScale();

    return TRUE;
}

void OGL_Stop()
{
    Combiner_Destroy();
    TextureCache_Destroy();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
}

static void OGL_SetRectViewport()
{
    if (OGL.adjustScreen)
        glViewport((GLint)OGL.adjustOffset, 0, (GLint)((float)OGL.width * OGL.adjustScale), (GLint)OGL.height);
    else
        glViewport(0, 0, (GLint)OGL.width, (GLint)OGL.height);
}

static float OGL_AdjustRectX(float x, float rectWidth)
{
    if ((OGL.widescreenScale != 1.0f) && (fabs(rectWidth) < (float)render_size.width * 9.0f / 10.0f))
        return x * OGL.widescreenScale + (float)render_size.width * (1.0f - OGL.widescreenScale) / 2.0f;

    return x;
}

void OGL_UpdateCullFace()
{
    if (render_state.cull_front || render_state.cull_back)
    {
        glEnable(GL_CULL_FACE);

        if (render_state.cull_back)
            glCullFace(GL_BACK);
        else
            glCullFace(GL_FRONT);
    }
    else
        glDisable(GL_CULL_FACE);
}

void OGL_UpdateViewport()
{
    const float viewportX = OGL.adjustScreen ? OGL.adjustOffset + (render_state.sp.viewport.x * OGL.scaleX)
                                             : (render_state.sp.viewport.x * OGL.scaleX);
    const float viewportY =
        (render_size.height - (render_state.sp.viewport.y + render_state.sp.viewport.height)) * OGL.scaleY;
    const float viewportWidth = render_state.sp.viewport.width * OGL.scaleX;
    const float viewportHeight = render_state.sp.viewport.height * OGL.scaleY;

    glViewport((GLint)viewportX, (GLint)viewportY, (GLint)viewportWidth, (GLint)viewportHeight);
    OGL_DepthRange(0.0f, 1.0f); // render_state.sp.viewport.nearz, render_state.sp.viewport.farz );
}

void OGL_UpdateDepthUpdate()
{
    if (render_state.dp.otherMode.depthUpdate)
        glDepthMask(TRUE);
    else
        glDepthMask(FALSE);
}

void OGL_UpdateStates()
{
    if (render_state.sp_changed & CHANGED_GEOMETRYMODE)
    {
        OGL_UpdateCullFace();

        Combiner_SetFogEnabled(render_state.fog && OGL.fog);

        render_state.sp_changed &= ~CHANGED_GEOMETRYMODE;
    }

    if (render_state.depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (render_state.dp.changed & CHANGED_RENDERMODE)
    {
        if (render_state.dp.otherMode.depthCompare)
            glDepthFunc(GL_LEQUAL);
        else
            glDepthFunc(GL_ALWAYS);

        OGL_UpdateDepthUpdate();

        if (render_state.dp.otherMode.depthMode == ZMODE_DEC)
            glEnable(GL_POLYGON_OFFSET_FILL);
        else
        {
            //			glPolygonOffset( -3.0f, -3.0f );
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    if ((render_state.dp.changed & CHANGED_ALPHACOMPARE) || (render_state.dp.changed & CHANGED_RENDERMODE))
    {
        if ((render_state.dp.otherMode.alphaCompare == G_AC_THRESHOLD) && !(render_state.dp.otherMode.alphaCvgSel))
            Combiner_SetAlphaTest((render_state.dp.blendColor.a > 0.0f) ? 1 : 2, render_state.dp.blendColor.a);
        else if (OGL.usePolygonStipple && (render_state.dp.otherMode.alphaCompare == G_AC_DITHER) &&
                 !(render_state.dp.otherMode.alphaCvgSel))
            Combiner_SetAlphaTest(3, 0.0f);
        // Used in TEX_EDGE and similar render modes
        else if (render_state.dp.otherMode.cvgXAlpha)
            Combiner_SetAlphaTest(1, 0.5f); // arbitrary threshold that gives nice results
        else
            Combiner_SetAlphaTest(0, 0.0f);
    }

    if (render_state.dp.changed & CHANGED_SCISSOR)
    {
        const float scissorX = OGL.adjustScreen ? OGL.adjustOffset + (render_state.dp.scissor.ulx * OGL.scaleX)
                                                : (render_state.dp.scissor.ulx * OGL.scaleX);
        const float scissorY = (render_size.height - render_state.dp.scissor.lry) * OGL.scaleY;
        const float scissorWidth = (render_state.dp.scissor.lrx - render_state.dp.scissor.ulx) * OGL.scaleX;
        const float scissorHeight = (render_state.dp.scissor.lry - render_state.dp.scissor.uly) * OGL.scaleY;

        glScissor((GLint)scissorX, (GLint)scissorY, (GLint)scissorWidth, (GLint)scissorHeight);
    }

    if (render_state.sp_changed & CHANGED_VIEWPORT)
    {
        OGL_UpdateViewport();
    }

    if ((render_state.dp.changed & CHANGED_COMBINE) || (render_state.dp.changed & CHANGED_CYCLETYPE))
    {
        if (render_state.dp.otherMode.cycleType == G_CYC_COPY)
            Combiner_SetCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
        else if (render_state.dp.otherMode.cycleType == G_CYC_FILL)
            Combiner_SetCombine(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, 1, 0, 0, 0, SHADE, 0, 0, 0, 1));
        else
            Combiner_SetCombine(render_state.dp.combine.mux);
    }

    if ((render_state.dp.changed & CHANGED_COMBINE_COLORS) || (render_state.dp.changed & CHANGED_FOGCOLOR))
    {
        Combiner_UpdateCombineColors();
        render_state.dp.changed &= ~CHANGED_FOGCOLOR;
    }

    if ((render_state.sp_changed & CHANGED_TEXTURE) || (render_state.dp.changed & CHANGED_TILE) ||
        (render_state.dp.changed & CHANGED_TMEM))
    {
        Combiner_BeginTextureUpdate();

        if (combiner.usesT0)
        {
            TextureCache_Update(0);

            render_state.sp_changed &= ~CHANGED_TEXTURE;
            render_state.dp.changed &= ~CHANGED_TILE;
            render_state.dp.changed &= ~CHANGED_TMEM;
        }
        else
        {
            TextureCache_ActivateDummy(0);
        }

        if (combiner.usesT1)
        {
            TextureCache_Update(1);

            render_state.sp_changed &= ~CHANGED_TEXTURE;
            render_state.dp.changed &= ~CHANGED_TILE;
            render_state.dp.changed &= ~CHANGED_TMEM;
        }
        else
        {
            TextureCache_ActivateDummy(1);
        }

        Combiner_EndTextureUpdate();
    }

    if ((render_state.dp.changed & CHANGED_RENDERMODE) || (render_state.dp.changed & CHANGED_CYCLETYPE))
    {
        if ((render_state.dp.otherMode.forceBlender) && (render_state.dp.otherMode.cycleType != G_CYC_COPY) &&
            (render_state.dp.otherMode.cycleType != G_CYC_FILL) && !(render_state.dp.otherMode.alphaCvgSel))
        {
            glEnable(GL_BLEND);

            switch (render_state.dp.otherMode.l >> 16)
            {
            case 0x0448: // Add
            case 0x055A:
                glBlendFunc(GL_ONE, GL_ONE);
                break;
            case 0x0C08: // 1080 Sky
            case 0x0F0A: // Used LOTS of places
                glBlendFunc(GL_ONE, GL_ZERO);
                break;
            case 0xC810: // Blends fog
            case 0xC811: // Blends fog
            case 0x0C18: // Standard interpolated blend
            case 0x0C19: // Used for antialiasing
            case 0x0050: // Standard interpolated blend
            case 0x0055: // Used for antialiasing
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
            case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
                glBlendFunc(GL_ZERO, GL_ONE);
                break;
            default:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            }
        }
        else
            glDisable(GL_BLEND);

        if (render_state.dp.otherMode.cycleType == G_CYC_FILL)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
        }
    }

    render_state.dp.changed &= CHANGED_TILE | CHANGED_TMEM;
    render_state.sp_changed &= CHANGED_TEXTURE;
}

void OGL_AddTriangle(const DrawVertex *vertices, float x_scale, bool disable_polygon_offset)
{

    if (render_state.sp_changed || render_state.dp.changed) OGL_UpdateStates();

    if (disable_polygon_offset) glDisable(GL_POLYGON_OFFSET_FILL);

    for (int i = 0; i < 3; i++)
    {
        OGL.vertices[OGL.numVertices].x = vertices[i].x * x_scale;
        OGL.vertices[OGL.numVertices].y = vertices[i].y;
        OGL.vertices[OGL.numVertices].z = render_state.dp.otherMode.depthSource == G_ZS_PRIM
                                              ? render_state.dp.primDepth.z * vertices[i].w
                                              : vertices[i].z;
        OGL.vertices[OGL.numVertices].w = vertices[i].w;

        // The vertex color carries SHADE and nothing else: the shader reads PRIM/ENV straight from
        // their uniforms, so the old fixed-function trick of smuggling a constant color in through
        // the vertex color must not overwrite it here.
        OGL.vertices[OGL.numVertices].color.r = vertices[i].r;
        OGL.vertices[OGL.numVertices].color.g = vertices[i].g;
        OGL.vertices[OGL.numVertices].color.b = vertices[i].b;
        OGL.vertices[OGL.numVertices].color.a = vertices[i].a;

        OGL.vertices[OGL.numVertices].secondaryColor.r = 0.0f;
        OGL.vertices[OGL.numVertices].secondaryColor.g = 0.0f;
        OGL.vertices[OGL.numVertices].secondaryColor.b = 0.0f;
        OGL.vertices[OGL.numVertices].secondaryColor.a = 1.0f;

        if (render_state.fog && OGL.fog)
        {
            if (vertices[i].z < -vertices[i].w)
                OGL.vertices[OGL.numVertices].fog =
                    std::max(0.0f, -(float)render_state.sp.fog.multiplier + (float)render_state.sp.fog.offset);
            else
                OGL.vertices[OGL.numVertices].fog =
                    std::max(0.0f, vertices[i].z / vertices[i].w * (float)render_state.sp.fog.multiplier +
                                       (float)render_state.sp.fog.offset);
        }

        if (combiner.usesT0)
        {
            OGL.vertices[OGL.numVertices].s0 =
                (vertices[i].s * cache.current[0]->shiftScaleS * render_state.sp.texture.scales -
                    render_texture_tile(0).fuls + cache.current[0]->offsetS) *
                cache.current[0]->scaleS;
            OGL.vertices[OGL.numVertices].t0 =
                (vertices[i].t * cache.current[0]->shiftScaleT * render_state.sp.texture.scalet -
                    render_texture_tile(0).fult + cache.current[0]->offsetT) *
                cache.current[0]->scaleT;
        }

        if (combiner.usesT1)
        {
            OGL.vertices[OGL.numVertices].s1 =
                (vertices[i].s * cache.current[1]->shiftScaleS * render_state.sp.texture.scales -
                    render_texture_tile(1).fuls + cache.current[1]->offsetS) *
                cache.current[1]->scaleS;
            OGL.vertices[OGL.numVertices].t1 =
                (vertices[i].t * cache.current[1]->shiftScaleT * render_state.sp.texture.scalet -
                    render_texture_tile(1).fult + cache.current[1]->offsetT) *
                cache.current[1]->scaleT;
        }
        OGL.numVertices++;
    }
    OGL.numTriangles++;

    if (OGL.numVertices >= 255) OGL_DrawTriangles();
}

void OGL_RestorePolygonOffset()
{
    if (render_state.dp.otherMode.depthMode == ZMODE_DEC) glEnable(GL_POLYGON_OFFSET_FILL);
}

void OGL_DrawTriangles()
{
    if (OGL.usePolygonStipple && (render_state.dp.otherMode.alphaCompare == G_AC_DITHER) &&
        !(render_state.dp.otherMode.alphaCvgSel))
        Combiner_UpdateDither(render_state.dp.envColor.a);

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    OGL_UploadVertices(OGL.numVertices);
    glDrawArrays(GL_TRIANGLES, 0, OGL.numVertices);
    OGL.numTriangles = OGL.numVertices = 0;
}

void OGL_DrawLine(const DrawVertex *vertices, float width)
{

    if (render_state.sp_changed || render_state.dp.changed) OGL_UpdateStates();

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glLineWidth(OGL.isGLES ? width * OGL.scaleX : 1.0f);

    for (int i = 0; i < 2; i++)
    {
        OGL.vertices[i].color.r = vertices[i].r;
        OGL.vertices[i].color.g = vertices[i].g;
        OGL.vertices[i].color.b = vertices[i].b;
        OGL.vertices[i].color.a = vertices[i].a;

        OGL.vertices[i].secondaryColor.r = 0.0f;
        OGL.vertices[i].secondaryColor.g = 0.0f;
        OGL.vertices[i].secondaryColor.b = 0.0f;
        OGL.vertices[i].secondaryColor.a = 1.0f;

        OGL.vertices[i].x = vertices[i].x * OGL.widescreenScale;
        OGL.vertices[i].y = vertices[i].y;
        OGL.vertices[i].z = vertices[i].z;
        OGL.vertices[i].w = vertices[i].w;
        OGL.vertices[i].fog = 0.0f;
    }
    OGL_UploadVertices(2);
    glDrawArrays(GL_LINES, 0, 2);
}

void OGL_DrawRect(int ulx, int uly, int lrx, int lry, const float *color)
{
    OGL_UpdateStates();

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    OGL_SetOrthoProjection(0.0f, render_size.width, render_size.height, 0.0f, 1.0f, -1.0f);

    OGL_SetRectViewport();
    OGL_DepthRange(0.0f, 1.0f);

    const float rectZ = (render_state.dp.otherMode.depthSource == G_ZS_PRIM) ? render_state.dp.primDepth.z : 0.0f;

    const float ulxa = OGL_AdjustRectX((float)ulx, (float)(lrx - ulx));
    const float lrxa = OGL_AdjustRectX((float)lrx, (float)(lrx - ulx));

    OGL.vertices[0].x = ulxa;
    OGL.vertices[0].y = uly;
    OGL.vertices[1].x = lrxa;
    OGL.vertices[1].y = uly;
    OGL.vertices[2].x = ulxa;
    OGL.vertices[2].y = lry;

    OGL.vertices[3].x = lrxa;
    OGL.vertices[3].y = uly;
    OGL.vertices[4].x = lrxa;
    OGL.vertices[4].y = lry;
    OGL.vertices[5].x = ulxa;
    OGL.vertices[5].y = lry;

    for (int i = 0; i < 6; i++)
    {
        OGL.vertices[i].z = rectZ;
        OGL.vertices[i].w = 1.0f;
        OGL.vertices[i].color.r = color[0];
        OGL.vertices[i].color.g = color[1];
        OGL.vertices[i].color.b = color[2];
        OGL.vertices[i].color.a = color[3];
        OGL.vertices[i].fog = 0.0f;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    OGL_UploadVertices(6);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    OGL_SetIdentityProjection();
    OGL_UpdateCullFace();
    OGL_UpdateViewport();
    glEnable(GL_SCISSOR_TEST);
}

void OGL_DrawTexturedRect(
    float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip)
{
    const float depth = render_state.dp.otherMode.depthSource == G_ZS_PRIM ? render_state.dp.primDepth.z
                                                                           : render_state.sp.viewport.nearz;
    GLVertex rect[2] = {
        {ulx, uly, depth, 1.0f, {1.0f, 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, uls, ult, uls, ult, 0.0f},
        {lrx, lry, depth, 1.0f, {1.0f, 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, lrs, lrt, lrs, lrt, 0.0f},
    };

    OGL_UpdateStates();

    glDisable(GL_CULL_FACE);
    OGL_SetOrthoProjection(0.0f, render_size.width, render_size.height, 0.0f, 1.0f, -1.0f);

    OGL_SetRectViewport();

    const float rectWidth = rect[1].x - rect[0].x;
    rect[0].x = OGL_AdjustRectX(rect[0].x, rectWidth);
    rect[1].x = OGL_AdjustRectX(rect[1].x, rectWidth);

    if (combiner.usesT0)
    {
        rect[0].s0 = rect[0].s0 * cache.current[0]->shiftScaleS - render_texture_tile(0).fuls;
        rect[0].t0 = rect[0].t0 * cache.current[0]->shiftScaleT - render_texture_tile(0).fult;
        rect[1].s0 = (rect[1].s0 + 1.0f) * cache.current[0]->shiftScaleS - render_texture_tile(0).fuls;
        rect[1].t0 = (rect[1].t0 + 1.0f) * cache.current[0]->shiftScaleT - render_texture_tile(0).fult;

        if ((cache.current[0]->maskS) && (fmod(rect[0].s0, cache.current[0]->width) == 0.0f) &&
            !(cache.current[0]->mirrorS))
        {
            rect[1].s0 -= rect[0].s0;
            rect[0].s0 = 0.0f;
        }

        if ((cache.current[0]->maskT) && (fmod(rect[0].t0, cache.current[0]->height) == 0.0f) &&
            !(cache.current[0]->mirrorT))
        {
            rect[1].t0 -= rect[0].t0;
            rect[0].t0 = 0.0f;
        }

        glActiveTexture(GL_TEXTURE0);

        if ((rect[0].s0 >= 0.0f) && (rect[1].s0 <= cache.current[0]->width))
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

        if ((rect[0].t0 >= 0.0f) && (rect[1].t0 <= cache.current[0]->height))
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //		GLint height;

        //		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );

        rect[0].s0 *= cache.current[0]->scaleS;
        rect[0].t0 *= cache.current[0]->scaleT;
        rect[1].s0 *= cache.current[0]->scaleS;
        rect[1].t0 *= cache.current[0]->scaleT;
    }

    if (combiner.usesT1)
    {
        rect[0].s1 = rect[0].s1 * cache.current[1]->shiftScaleS - render_texture_tile(1).fuls;
        rect[0].t1 = rect[0].t1 * cache.current[1]->shiftScaleT - render_texture_tile(1).fult;
        rect[1].s1 = (rect[1].s1 + 1.0f) * cache.current[1]->shiftScaleS - render_texture_tile(1).fuls;
        rect[1].t1 = (rect[1].t1 + 1.0f) * cache.current[1]->shiftScaleT - render_texture_tile(1).fult;

        if ((cache.current[1]->maskS) && (fmod(rect[0].s1, cache.current[1]->width) == 0.0f) &&
            !(cache.current[1]->mirrorS))
        {
            rect[1].s1 -= rect[0].s1;
            rect[0].s1 = 0.0f;
        }

        if ((cache.current[1]->maskT) && (fmod(rect[0].t1, cache.current[1]->height) == 0.0f) &&
            !(cache.current[1]->mirrorT))
        {
            rect[1].t1 -= rect[0].t1;
            rect[0].t1 = 0.0f;
        }

        glActiveTexture(GL_TEXTURE1);

        if ((rect[0].s1 == 0.0f) && (rect[1].s1 <= cache.current[1]->width))
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

        if ((rect[0].t1 == 0.0f) && (rect[1].t1 <= cache.current[1]->height))
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        rect[0].s1 *= cache.current[1]->scaleS;
        rect[0].t1 *= cache.current[1]->scaleT;
        rect[1].s1 *= cache.current[1]->scaleS;
        rect[1].t1 *= cache.current[1]->scaleT;
    }

    if (render_state.dp.otherMode.cycleType == G_CYC_COPY)
    {
        glActiveTexture(GL_TEXTURE0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    OGL.vertices[0].s0 = rect[0].s0;
    OGL.vertices[0].t0 = rect[0].t0;
    OGL.vertices[0].s1 = rect[0].s1;
    OGL.vertices[0].t1 = rect[0].t1;
    OGL.vertices[0].x = rect[0].x;
    OGL.vertices[0].y = rect[0].y;

    OGL.vertices[1].s0 = rect[1].s0;
    OGL.vertices[1].t0 = rect[0].t0;
    OGL.vertices[1].s1 = rect[1].s1;
    OGL.vertices[1].t1 = rect[0].t1;
    OGL.vertices[1].x = rect[1].x;
    OGL.vertices[1].y = rect[0].y;

    OGL.vertices[2].s0 = rect[0].s0;
    OGL.vertices[2].t0 = rect[1].t0;
    OGL.vertices[2].s1 = rect[0].s1;
    OGL.vertices[2].t1 = rect[1].t1;
    OGL.vertices[2].x = rect[0].x;
    OGL.vertices[2].y = rect[1].y;

    OGL.vertices[3].s0 = rect[1].s0;
    OGL.vertices[3].t0 = rect[1].t0;
    OGL.vertices[3].s1 = rect[1].s1;
    OGL.vertices[3].t1 = rect[1].t1;
    OGL.vertices[3].x = rect[1].x;
    OGL.vertices[3].y = rect[1].y;

    for (int i = 0; i < 4; i++)
    {
        OGL.vertices[i].z = rect[0].z;
        OGL.vertices[i].w = 1.0f;
        OGL.vertices[i].fog = 0.0f;
        OGL.vertices[i].color = rect[0].color;
        OGL.vertices[i].secondaryColor = rect[0].secondaryColor;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    OGL_UploadVertices(4);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    OGL_SetIdentityProjection();
    OGL_UpdateCullFace();
    OGL_UpdateViewport();
}

void OGL_ClearDepthBuffer()
{
    OGL_UpdateStates();
    glDepthMask(TRUE);
    //	glEnable( GL_DEPTH_TEST );
    glClear(GL_DEPTH_BUFFER_BIT);

    OGL_UpdateDepthUpdate();
}

void OGL_ClearColorBuffer(float *color)
{
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}
