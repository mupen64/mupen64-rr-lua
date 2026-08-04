/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "glN64.hpp"
#include "OpenGL.hpp"
#include "Types.hpp"
#include "N64.hpp"
#include "gSP.hpp"
#include "gDP.hpp"
#include "Textures.h"
#include "Combiner.hpp"
#include "VI.hpp"

GLInfo OGL{};

void *gCapturedPixels; // pointer to buffer to fill

static SDL_Window *s_sdl_window;
static SDL_GLContext s_sdl_context;

void OGL_ReadPixels()
{
    if (!gCapturedPixels) return;

    glFinish();

    glReadPixels(0, 0, OGL.width, OGL.height, GL_RGBA, GL_UNSIGNED_BYTE, gCapturedPixels);

    auto pixels = (uint8_t *)gCapturedPixels;
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
    glewExperimental = GL_TRUE;
    GLenum glew = glewInit();
    if (glew != GLEW_OK)
    {
        g_plugin->log_error(L"Error initialising glew");
        return;
    }

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &OGL.maxTextureUnits);
    OGL.maxTextureUnits = min(8, OGL.maxTextureUnits); // The plugin only supports 8, and 4 is really enough

    OGL.EXT_fog_coord = GLEW_EXT_fog_coord;
    OGL.EXT_secondary_color = GLEW_EXT_secondary_color;
    OGL.EXT_texture_env_combine = GLEW_EXT_texture_env_combine;
}

void OGL_SetIdentityProjection()
{
    static const float identity[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
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
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.vertices[0].x);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.vertices[0].color.r);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.vertices[0].secondaryColor.r);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.vertices[0].s0);
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.vertices[0].s1);
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.vertices[0].fog);
    glEnableVertexAttribArray(5);

    glPolygonOffset(-3.0f, -3.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OGL_UpdateScale()
{
    OGL.adjustScreen = FALSE;
    OGL.adjustScale = 1.0f;
    OGL.adjustOffset = 0.0f;

    if ((VI.width != 0) && (VI.height != 0) && (OGL.width != 0) && (OGL.height != 0))
    {
        const float sourceAspect = (float)VI.width / (float)VI.height;
        const float displayAspect = (float)OGL.width / (float)OGL.height;
        if (displayAspect > sourceAspect)
        {
            const float targetWidth = (float)OGL.height * sourceAspect;
            OGL.adjustScale = targetWidth / (float)OGL.width;
            OGL.adjustOffset = (float)OGL.width * (1.0f - OGL.adjustScale) / 2.0f;
            OGL.adjustScreen = TRUE;
        }
    }

    OGL.scaleX = (OGL.width * OGL.adjustScale) / (float)VI.width;
    OGL.scaleY = OGL.height / (float)VI.height;
}

void OGL_ResizeWindow()
{
    OGL.width = OGL.windowedWidth;
    OGL.height = OGL.windowedHeight;
    g_plugin->request_size(OGL.width, OGL.height);
}

bool OGL_InitContext()
{
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (OGL.msaa > 0)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, OGL.msaa);
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    }

    s_sdl_window =
        SDL_CreateWindow("TASVideo", OGL.windowedWidth, OGL.windowedHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_assert_release(s_sdl_window);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    s_sdl_context = SDL_GL_CreateContext(s_sdl_window);
    OGL.isGLES = (s_sdl_context != nullptr);

    if (!s_sdl_context)
    {
        g_plugin->log_info(L"OpenGL ES 2.0 context unavailable; falling back to desktop OpenGL.");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        s_sdl_context = SDL_GL_CreateContext(s_sdl_window);
    }
    SDL_assert_release(s_sdl_context);

    SDL_assert_release(SDL_GL_MakeCurrent(s_sdl_window, s_sdl_context));

    int multisample_buffers = 0;
    int multisample_samples = 0;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &multisample_buffers);
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &multisample_samples);

    if (OGL.msaa > 0)
    {
        RT_ASSERT(multisample_buffers == 1 && multisample_samples == OGL.msaa,
                  std::format(L"MSAA {}x is required, got {} buffers {} samples. Try updating your graphics driver.",
                              OGL.msaa, multisample_buffers, multisample_samples));
    }

    OGL_InitExtensions();
    OGL_InitStates();

    g_plugin->log_info(OGL.isGLES ? L"TASVideo: running on an OpenGL ES 2.0 context."
                                  : L"TASVideo: running on a desktop OpenGL compatibility context.");

    OGL.context_initialized = TRUE;

    return TRUE;
}

bool OGL_DestroyContext()
{
    SDL_GL_MakeCurrent(s_sdl_window, nullptr);

    SDL_GL_DestroyContext(s_sdl_context);
    s_sdl_context = nullptr;

    SDL_DestroyWindow(s_sdl_window);
    s_sdl_window = nullptr;

    OGL.context_initialized = FALSE;

    return TRUE;
}

bool OGL_Start()
{
    if (!OGL.context_initialized && !OGL_InitContext()) return FALSE;

    TextureCache_Init();
    FrameBuffer_Init();
    Combiner_Init();

    gSP.changed = gDP.changed = 0xFFFFFFFF;
    OGL_UpdateScale();

    return TRUE;
}

void OGL_Stop()
{
    Combiner_Destroy();
    FrameBuffer_Destroy();
    TextureCache_Destroy();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
}

void OGL_UpdateCullFace()
{
    if (gSP.geometryMode & G_CULL_BOTH)
    {
        glEnable(GL_CULL_FACE);

        if (gSP.geometryMode & G_CULL_BACK)
            glCullFace(GL_BACK);
        else
            glCullFace(GL_FRONT);
    }
    else
        glDisable(GL_CULL_FACE);
}

void OGL_UpdateViewport()
{
    const float viewportX =
        OGL.adjustScreen ? OGL.adjustOffset + (gSP.viewport.x * OGL.scaleX) : (gSP.viewport.x * OGL.scaleX);
    const float viewportY = (VI.height - (gSP.viewport.y + gSP.viewport.height)) * OGL.scaleY;
    const float viewportWidth = gSP.viewport.width * OGL.scaleX;
    const float viewportHeight = gSP.viewport.height * OGL.scaleY;

    glViewport((GLint)viewportX, (GLint)viewportY, (GLint)viewportWidth, (GLint)viewportHeight);
    OGL_DepthRange(0.0f, 1.0f); // gSP.viewport.nearz, gSP.viewport.farz );
}

void OGL_UpdateDepthUpdate()
{
    if (gDP.otherMode.depthUpdate)
        glDepthMask(TRUE);
    else
        glDepthMask(FALSE);
}

void OGL_UpdateStates()
{
    if (gSP.changed & CHANGED_GEOMETRYMODE)
    {
        OGL_UpdateCullFace();

        Combiner_SetFogEnabled((gSP.geometryMode & G_FOG) && OGL.fog);

        gSP.changed &= ~CHANGED_GEOMETRYMODE;
    }

    if (gSP.geometryMode & G_ZBUFFER)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (gDP.changed & CHANGED_RENDERMODE)
    {
        if (gDP.otherMode.depthCompare)
            glDepthFunc(GL_LEQUAL);
        else
            glDepthFunc(GL_ALWAYS);

        OGL_UpdateDepthUpdate();

        if (gDP.otherMode.depthMode == ZMODE_DEC)
            glEnable(GL_POLYGON_OFFSET_FILL);
        else
        {
            //			glPolygonOffset( -3.0f, -3.0f );
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    if ((gDP.changed & CHANGED_ALPHACOMPARE) || (gDP.changed & CHANGED_RENDERMODE))
    {
        if ((gDP.otherMode.alphaCompare == G_AC_THRESHOLD) && !(gDP.otherMode.alphaCvgSel))
            Combiner_SetAlphaTest((gDP.blendColor.a > 0.0f) ? 1 : 2, gDP.blendColor.a);
        else if (OGL.usePolygonStipple && (gDP.otherMode.alphaCompare == G_AC_DITHER) && !(gDP.otherMode.alphaCvgSel))
            Combiner_SetAlphaTest(3, 0.0f);
        // Used in TEX_EDGE and similar render modes
        else if (gDP.otherMode.cvgXAlpha)
            Combiner_SetAlphaTest(1, 0.5f); // arbitrary threshold that gives nice results
        else
            Combiner_SetAlphaTest(0, 0.0f);
    }

    if (gDP.changed & CHANGED_SCISSOR)
    {
        const float scissorX =
            OGL.adjustScreen ? OGL.adjustOffset + (gDP.scissor.ulx * OGL.scaleX) : (gDP.scissor.ulx * OGL.scaleX);
        const float scissorY = (VI.height - gDP.scissor.lry) * OGL.scaleY;
        const float scissorWidth = (gDP.scissor.lrx - gDP.scissor.ulx) * OGL.scaleX;
        const float scissorHeight = (gDP.scissor.lry - gDP.scissor.uly) * OGL.scaleY;

        glScissor((GLint)scissorX, (GLint)scissorY, (GLint)scissorWidth, (GLint)scissorHeight);
    }

    if (gSP.changed & CHANGED_VIEWPORT)
    {
        OGL_UpdateViewport();
    }

    if ((gDP.changed & CHANGED_COMBINE) || (gDP.changed & CHANGED_CYCLETYPE))
    {
        if (gDP.otherMode.cycleType == G_CYC_COPY)
            Combiner_SetCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
        else if (gDP.otherMode.cycleType == G_CYC_FILL)
            Combiner_SetCombine(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, 1, 0, 0, 0, SHADE, 0, 0, 0, 1));
        else
            Combiner_SetCombine(gDP.combine.mux);
    }

    if ((gDP.changed & CHANGED_COMBINE_COLORS) || (gDP.changed & CHANGED_FOGCOLOR))
    {
        Combiner_UpdateCombineColors();
        gDP.changed &= ~CHANGED_FOGCOLOR;
    }

    if ((gSP.changed & CHANGED_TEXTURE) || (gDP.changed & CHANGED_TILE) || (gDP.changed & CHANGED_TMEM))
    {
        Combiner_BeginTextureUpdate();

        if (combiner.usesT0)
        {
            TextureCache_Update(0);

            gSP.changed &= ~CHANGED_TEXTURE;
            gDP.changed &= ~CHANGED_TILE;
            gDP.changed &= ~CHANGED_TMEM;
        }
        else
        {
            TextureCache_ActivateDummy(0);
        }

        if (combiner.usesT1)
        {
            TextureCache_Update(1);

            gSP.changed &= ~CHANGED_TEXTURE;
            gDP.changed &= ~CHANGED_TILE;
            gDP.changed &= ~CHANGED_TMEM;
        }
        else
        {
            TextureCache_ActivateDummy(1);
        }

        Combiner_EndTextureUpdate();
    }

    if ((gDP.changed & CHANGED_RENDERMODE) || (gDP.changed & CHANGED_CYCLETYPE))
    {
        if ((gDP.otherMode.forceBlender) && (gDP.otherMode.cycleType != G_CYC_COPY) &&
            (gDP.otherMode.cycleType != G_CYC_FILL) && !(gDP.otherMode.alphaCvgSel))
        {
            glEnable(GL_BLEND);

            switch (gDP.otherMode.l >> 16)
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

        if (gDP.otherMode.cycleType == G_CYC_FILL)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
        }
    }

    gDP.changed &= CHANGED_TILE | CHANGED_TMEM;
    gSP.changed &= CHANGED_TEXTURE | CHANGED_MATRIX;
}

void OGL_AddTriangle(SPVertex *vertices, int v0, int v1, int v2)
{
    int v[] = {v0, v1, v2};

    if (gSP.changed || gDP.changed) OGL_UpdateStates();

    //	Playing around with lod fraction junk...
    //	float ds = max( max( fabs( vertices[v0].s - vertices[v1].s ), fabs( vertices[v0].s - vertices[v2].s ) ), fabs(
    // vertices[v1].s - vertices[v2].s ) ) * cache.current[0]->shiftScaleS * gSP.texture.scales; 	float dx = max( max(
    // fabs( vertices[v0].x / vertices[v0].w - vertices[v1].x / vertices[v1].w ), fabs( vertices[v0].x / vertices[v0].w
    // - vertices[v2].x / vertices[v2].w ) ), fabs( vertices[v1].x / vertices[v1].w - vertices[v2].x / vertices[v2].w )
    // ) * gSP.viewport.vscale[0]; 	float lod = ds / dx; 	float lod_fraction = min( 1.0f, max( 0.0f, lod - 1.0f ) /
    // max( 1.0f, gSP.texture.level ) );

    for (int i = 0; i < 3; i++)
    {
        OGL.vertices[OGL.numVertices].x = vertices[v[i]].x;
        OGL.vertices[OGL.numVertices].y = vertices[v[i]].y;
        OGL.vertices[OGL.numVertices].z =
            gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z * vertices[v[i]].w : vertices[v[i]].z;
        OGL.vertices[OGL.numVertices].w = vertices[v[i]].w;

        // The vertex color carries SHADE and nothing else: the shader reads PRIM/ENV straight from
        // their uniforms, so the old fixed-function trick of smuggling a constant color in through
        // the vertex color must not overwrite it here.
        OGL.vertices[OGL.numVertices].color.r = vertices[v[i]].r;
        OGL.vertices[OGL.numVertices].color.g = vertices[v[i]].g;
        OGL.vertices[OGL.numVertices].color.b = vertices[v[i]].b;
        OGL.vertices[OGL.numVertices].color.a = vertices[v[i]].a;

        OGL.vertices[OGL.numVertices].secondaryColor.r = 0.0f;
        OGL.vertices[OGL.numVertices].secondaryColor.g = 0.0f;
        OGL.vertices[OGL.numVertices].secondaryColor.b = 0.0f;
        OGL.vertices[OGL.numVertices].secondaryColor.a = 1.0f;

        if ((gSP.geometryMode & G_FOG) && OGL.fog)
        {
            if (vertices[v[i]].z < -vertices[v[i]].w)
                OGL.vertices[OGL.numVertices].fog = max(0.0f, -(float)gSP.fog.multiplier + (float)gSP.fog.offset);
            else
                OGL.vertices[OGL.numVertices].fog =
                    max(0.0f, vertices[v[i]].z / vertices[v[i]].w * (float)gSP.fog.multiplier + (float)gSP.fog.offset);
        }

        if (combiner.usesT0)
        {
            if (cache.current[0]->frameBufferTexture)
            {
                /*				OGL.vertices[OGL.numVertices].s0 = (cache.current[0]->offsetS + (vertices[v[i]].s *
                   cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls)) *
                   cache.current[0]->scaleS; OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT -
                   (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult)) *
                   cache.current[0]->scaleT;*/

                if (gSP.textureTile[0]->masks)
                    OGL.vertices[OGL.numVertices].s0 =
                        (cache.current[0]->offsetS +
                         (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales -
                          fmod(gSP.textureTile[0]->fuls, 1 << gSP.textureTile[0]->masks))) *
                        cache.current[0]->scaleS;
                else
                    OGL.vertices[OGL.numVertices].s0 =
                        (cache.current[0]->offsetS +
                         (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales -
                          gSP.textureTile[0]->fuls)) *
                        cache.current[0]->scaleS;

                if (gSP.textureTile[0]->maskt)
                    OGL.vertices[OGL.numVertices].t0 =
                        (cache.current[0]->offsetT -
                         (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet -
                          fmod(gSP.textureTile[0]->fult, 1 << gSP.textureTile[0]->maskt))) *
                        cache.current[0]->scaleT;
                else
                    OGL.vertices[OGL.numVertices].t0 =
                        (cache.current[0]->offsetT -
                         (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet -
                          gSP.textureTile[0]->fult)) *
                        cache.current[0]->scaleT;
            }
            else
            {
                OGL.vertices[OGL.numVertices].s0 =
                    (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls +
                     cache.current[0]->offsetS) *
                    cache.current[0]->scaleS;
                OGL.vertices[OGL.numVertices].t0 =
                    (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult +
                     cache.current[0]->offsetT) *
                    cache.current[0]->scaleT;
            }
        }

        if (combiner.usesT1)
        {
            if (cache.current[0]->frameBufferTexture)
            {
                OGL.vertices[OGL.numVertices].s1 =
                    (cache.current[1]->offsetS +
                     (vertices[v[i]].s * cache.current[1]->shiftScaleS * gSP.texture.scales -
                      gSP.textureTile[1]->fuls)) *
                    cache.current[1]->scaleS;
                OGL.vertices[OGL.numVertices].t1 =
                    (cache.current[1]->offsetT -
                     (vertices[v[i]].t * cache.current[1]->shiftScaleT * gSP.texture.scalet -
                      gSP.textureTile[1]->fult)) *
                    cache.current[1]->scaleT;
            }
            else
            {
                OGL.vertices[OGL.numVertices].s1 =
                    (vertices[v[i]].s * cache.current[1]->shiftScaleS * gSP.texture.scales - gSP.textureTile[1]->fuls +
                     cache.current[1]->offsetS) *
                    cache.current[1]->scaleS;
                OGL.vertices[OGL.numVertices].t1 =
                    (vertices[v[i]].t * cache.current[1]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[1]->fult +
                     cache.current[1]->offsetT) *
                    cache.current[1]->scaleT;
            }
        }
        OGL.numVertices++;
    }
    OGL.numTriangles++;

    if (OGL.numVertices >= 255) OGL_DrawTriangles();
}

void OGL_DrawTriangles()
{
    if (OGL.usePolygonStipple && (gDP.otherMode.alphaCompare == G_AC_DITHER) && !(gDP.otherMode.alphaCvgSel))
        Combiner_UpdateDither(gDP.envColor.a);

    glDrawArrays(GL_TRIANGLES, 0, OGL.numVertices);
    OGL.numTriangles = OGL.numVertices = 0;
}

void OGL_DrawLine(SPVertex *vertices, int v0, int v1, float width)
{
    int v[] = {v0, v1};

    if (gSP.changed || gDP.changed) OGL_UpdateStates();

    glLineWidth(width * OGL.scaleX);

    for (int i = 0; i < 2; i++)
    {
        OGL.vertices[i].color.r = vertices[v[i]].r;
        OGL.vertices[i].color.g = vertices[v[i]].g;
        OGL.vertices[i].color.b = vertices[v[i]].b;
        OGL.vertices[i].color.a = vertices[v[i]].a;

        OGL.vertices[i].secondaryColor.r = 0.0f;
        OGL.vertices[i].secondaryColor.g = 0.0f;
        OGL.vertices[i].secondaryColor.b = 0.0f;
        OGL.vertices[i].secondaryColor.a = 1.0f;

        OGL.vertices[i].x = vertices[v[i]].x;
        OGL.vertices[i].y = vertices[v[i]].y;
        OGL.vertices[i].z = vertices[v[i]].z;
        OGL.vertices[i].w = vertices[v[i]].w;
        OGL.vertices[i].fog = 0.0f;
    }
    glDrawArrays(GL_LINES, 0, 2);
}

void OGL_DrawRect(int ulx, int uly, int lrx, int lry, float *color)
{
    OGL_UpdateStates();

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    OGL_SetOrthoProjection(0.0f, VI.width, VI.height, 0.0f, 1.0f, -1.0f);

    const float viewportX = OGL.adjustScreen ? OGL.adjustOffset : 0.0f;
    const float viewportWidth = OGL.adjustScreen ? OGL.width * OGL.adjustScale : (float)OGL.width;
    glViewport((GLint)viewportX, 0, (GLint)viewportWidth, (GLint)OGL.height);
    OGL_DepthRange(0.0f, 1.0f);

    const float rectZ = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f;

    OGL.vertices[0].x = ulx;
    OGL.vertices[0].y = uly;
    OGL.vertices[1].x = lrx;
    OGL.vertices[1].y = uly;
    OGL.vertices[2].x = ulx;
    OGL.vertices[2].y = lry;

    OGL.vertices[3].x = lrx;
    OGL.vertices[3].y = uly;
    OGL.vertices[4].x = lrx;
    OGL.vertices[4].y = lry;
    OGL.vertices[5].x = ulx;
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

    glDrawArrays(GL_TRIANGLES, 0, 6);

    OGL_SetIdentityProjection();
    OGL_UpdateCullFace();
    OGL_UpdateViewport();
    glEnable(GL_SCISSOR_TEST);
}

void OGL_DrawTexturedRect(float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt,
                          bool flip)
{
    GLVertex rect[2] = {
        {ulx,
         uly,
         gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z : gSP.viewport.nearz,
         1.0f,
         {/*gDP.blendColor.r, gDP.blendColor.g, gDP.blendColor.b, gDP.blendColor.a */ 1.0f, 1.0f, 1.0f, 0.0f},
         {1.0f, 1.0f, 1.0f, 1.0f},
         uls,
         ult,
         uls,
         ult,
         0.0f},
        {lrx,
         lry,
         gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z : gSP.viewport.nearz,
         1.0f,
         {/*gDP.blendColor.r, gDP.blendColor.g, gDP.blendColor.b, gDP.blendColor.a*/ 1.0f, 1.0f, 1.0f, 0.0f},
         {1.0f, 1.0f, 1.0f, 1.0f},
         lrs,
         lrt,
         lrs,
         lrt,
         0.0f},
    };

    OGL_UpdateStates();

    glDisable(GL_CULL_FACE);
    OGL_SetOrthoProjection(0.0f, VI.width, VI.height, 0.0f, 1.0f, -1.0f);

    const float viewportX = OGL.adjustScreen ? OGL.adjustOffset : 0.0f;
    const float viewportWidth = OGL.adjustScreen ? OGL.width * OGL.adjustScale : (float)OGL.width;
    glViewport((GLint)viewportX, 0, (GLint)viewportWidth, (GLint)OGL.height);

    if (combiner.usesT0)
    {
        rect[0].s0 = rect[0].s0 * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
        rect[0].t0 = rect[0].t0 * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
        rect[1].s0 = (rect[1].s0 + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
        rect[1].t0 = (rect[1].t0 + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;

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

        if (cache.current[0]->frameBufferTexture)
        {
            rect[0].s0 = cache.current[0]->offsetS + rect[0].s0;
            rect[0].t0 = cache.current[0]->offsetT - rect[0].t0;
            rect[1].s0 = cache.current[0]->offsetS + rect[1].s0;
            rect[1].t0 = cache.current[0]->offsetT - rect[1].t0;
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
        rect[0].s1 = rect[0].s1 * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
        rect[0].t1 = rect[0].t1 * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
        rect[1].s1 = (rect[1].s1 + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
        rect[1].t1 = (rect[1].t1 + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

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

        if (cache.current[1]->frameBufferTexture)
        {
            rect[0].s1 = cache.current[1]->offsetS + rect[0].s1;
            rect[0].t1 = cache.current[1]->offsetT - rect[0].t1;
            rect[1].s1 = cache.current[1]->offsetS + rect[1].s1;
            rect[1].t1 = cache.current[1]->offsetT - rect[1].t1;
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

    if (gDP.otherMode.cycleType == G_CYC_COPY)
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
