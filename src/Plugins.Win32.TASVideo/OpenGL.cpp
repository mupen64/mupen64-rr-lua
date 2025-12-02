#include "stdafx.h"
#include "glN64.h"
#include "OpenGL.h"
#include "Types.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "VI.h"

#include <format>

GLInfo OGL{};

void* gCapturedPixels; // pointer to buffer to fill

void OGL_ReadPixels()
{
    GLint oldMode;
    glGetIntegerv(GL_READ_BUFFER, &oldMode);
    // glReadBuffer(GL_FRONT);

    glReadBuffer(GL_BACK);
    glReadPixels(0, OGL.heightOffset, OGL.width, OGL.height,
                 GL_BGR, GL_UNSIGNED_BYTE, gCapturedPixels);
    glReadBuffer(oldMode); // restore old read buffer
}

void OGL_InitExtensions()
{
    GLenum glew = glewInit();
    if (glew != GLEW_OK)
    {
        g_ef->log_error(L"Error initialising glew");
        return;
    }

    OGL.NV_register_combiners = GLEW_NV_register_combiners;
    glGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &OGL.maxGeneralCombiners);

    OGL.ARB_multitexture = GLEW_ARB_multitexture;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &OGL.maxTextureUnits);
    OGL.maxTextureUnits = min(8, OGL.maxTextureUnits); // The plugin only supports 8, and 4 is really enough

    OGL.EXT_fog_coord = GLEW_EXT_fog_coord;
    OGL.EXT_secondary_color = GLEW_EXT_secondary_color;
    OGL.ARB_texture_env_combine = GLEW_ARB_texture_env_combine;
    OGL.ARB_texture_env_crossbar = GLEW_ARB_texture_env_crossbar;
    OGL.EXT_texture_env_combine = GLEW_EXT_texture_env_combine;
    OGL.ATI_texture_env_combine3 = GLEW_ATI_texture_env_combine3;
    OGL.ATIX_texture_env_route = GLEW_ATIX_texture_env_route;
    OGL.NV_texture_env_combine4 = GLEW_NV_texture_env_combine4;
}

void OGL_InitStates()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glVertexPointer(4, GL_FLOAT, sizeof(GLVertex), &OGL.vertices[0].x);
    glEnableClientState(GL_VERTEX_ARRAY);

    glColorPointer(4, GL_FLOAT, sizeof(GLVertex), &OGL.vertices[0].color.r);
    glEnableClientState(GL_COLOR_ARRAY);

    if (OGL.EXT_secondary_color)
    {
        glSecondaryColorPointerEXT(3, GL_FLOAT, sizeof(GLVertex), &OGL.vertices[0].secondaryColor.r);
        glEnableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
    }

    if (OGL.ARB_multitexture)
    {
        glClientActiveTextureARB(GL_TEXTURE0_ARB);
        glTexCoordPointer(2, GL_FLOAT, sizeof(GLVertex), &OGL.vertices[0].s0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glClientActiveTextureARB(GL_TEXTURE1_ARB);
        glTexCoordPointer(2, GL_FLOAT, sizeof(GLVertex), &OGL.vertices[0].s1);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    else
    {
        glTexCoordPointer(2, GL_FLOAT, sizeof(GLVertex), &OGL.vertices[0].s0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (OGL.EXT_fog_coord)
    {
        glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);

        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 0.0f);
        glFogf(GL_FOG_END, 255.0f);

        glFogCoordPointerEXT(GL_FLOAT, sizeof(GLVertex), &OGL.vertices[0].fog);
        glEnableClientState(GL_FOG_COORDINATE_ARRAY_EXT);
    }

    glPolygonOffset(-3.0f, -3.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    srand(timeGetTime());

    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 8; j++)
            for (int k = 0; k < 128; k++)
                OGL.stipplePattern[i][j][k] = ((i > (rand() >> 10)) << 7) |
                ((i > (rand() >> 10)) << 6) |
                ((i > (rand() >> 10)) << 5) |
                ((i > (rand() >> 10)) << 4) |
                ((i > (rand() >> 10)) << 3) |
                ((i > (rand() >> 10)) << 2) |
                ((i > (rand() >> 10)) << 1) |
                ((i > (rand() >> 10)) << 0);
    }

    SwapBuffers(wglGetCurrentDC());
}

void OGL_UpdateScale()
{
    OGL.scaleX = OGL.width / (float)VI.width;
    OGL.scaleY = OGL.height / (float)VI.height;
}

void OGL_ResizeWindow()
{
    RECT windowRect, statusRect, toolRect;
    
    OGL.width = OGL.windowedWidth;
    OGL.height = OGL.windowedHeight;

    GetClientRect(hWnd, &windowRect);

    if (hStatusBar)
        GetWindowRect(hStatusBar, &statusRect);
    else
        statusRect.bottom = statusRect.top = 0;

    if (hToolBar)
        GetWindowRect(hToolBar, &toolRect);
    else
        toolRect.bottom = toolRect.top = 0;

    OGL.heightOffset = (statusRect.bottom - statusRect.top);
    windowRect.right = windowRect.left + OGL.windowedWidth - 1;
    windowRect.bottom = windowRect.top + OGL.windowedHeight - 1 + OGL.heightOffset;

    AdjustWindowRect(&windowRect, GetWindowLong(hWnd, GWL_STYLE), GetMenu(hWnd) != NULL);

    SetWindowPos(hWnd, NULL, 0, 0, windowRect.right - windowRect.left + 1,
                 windowRect.bottom - windowRect.top + 1 + toolRect.bottom - toolRect.top + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
}

bool OGL_InitContext()
{
    int pixelFormat;

    PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
    1, // version number
    PFD_DRAW_TO_WINDOW | // support window
    PFD_SUPPORT_OPENGL | // support OpenGL
    PFD_DOUBLEBUFFER, // double buffered
    PFD_TYPE_RGBA, // RGBA type
    32, // color depth
    0, 0, 0, 0, 0, 0, // color bits ignored
    0, // no alpha buffer
    0, // shift bit ignored
    0, // no accumulation buffer
    0, 0, 0, 0, // accum bits ignored
    32, // z-buffer
    0, // no stencil buffer
    0, // no auxiliary buffer
    PFD_MAIN_PLANE, // main layer
    0, // reserved
    0, 0, 0 // layer masks ignored
    };

    if ((OGL.hDC = GetDC(hWnd)) == NULL)
    {
        MessageBox(hWnd, L"Error while getting a device context!", PLUGIN_NAME, MB_ICONERROR | MB_OK);
        return FALSE;
    }

    if ((pixelFormat = ChoosePixelFormat(OGL.hDC, &pfd)) == 0)
    {
        MessageBox(hWnd, L"Unable to find a suitable pixel format!", PLUGIN_NAME, MB_ICONERROR | MB_OK);
        OGL_Stop();
        return FALSE;
    }

    if ((SetPixelFormat(OGL.hDC, pixelFormat, &pfd)) == FALSE)
    {
        MessageBox(hWnd, L"Error while setting pixel format!", PLUGIN_NAME, MB_ICONERROR | MB_OK);
        OGL_Stop();
        return FALSE;
    }
    if ((OGL.hRC = wglCreateContext(OGL.hDC)) == NULL)
    {
        MessageBox(hWnd, L"Error while creating OpenGL context!", PLUGIN_NAME, MB_ICONERROR | MB_OK);
        OGL_Stop();
        return FALSE;
    }

    if ((wglMakeCurrent(OGL.hDC, OGL.hRC)) == FALSE)
    {
        MessageBox(hWnd, L"Error while making OpenGL context current!", PLUGIN_NAME, MB_ICONERROR | MB_OK);
        OGL_Stop();
        return FALSE;
    }

    OGL_InitExtensions();
    OGL_InitStates();
}

bool OGL_DestroyContext()
{
    wglMakeCurrent(NULL, NULL);

    if (OGL.hRC)
    {
        wglDeleteContext(OGL.hRC);
        OGL.hRC = NULL;
    }

    if (OGL.hDC)
    {
        ReleaseDC(hWnd, OGL.hDC);
        OGL.hDC = NULL;
    }

    return TRUE;
}

bool OGL_Start()
{

    if (OGL.recycle_context)
    {
        if (!OGL.context_initialized)
        {
            OGL_InitContext();
        }
    }
    else
    {
        OGL_InitContext();
    }

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
    SwapBuffers(OGL.hDC);

    if (!OGL.recycle_context)
    {
        OGL_DestroyContext();
    }
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
    glViewport(gSP.viewport.x * OGL.scaleX, (VI.height - (gSP.viewport.y + gSP.viewport.height)) * OGL.scaleY + OGL.heightOffset,
               gSP.viewport.width * OGL.scaleX, gSP.viewport.height * OGL.scaleY);
    glDepthRange(0.0f, 1.0f); // gSP.viewport.nearz, gSP.viewport.farz );
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

        if ((gSP.geometryMode & G_FOG) && OGL.EXT_fog_coord && OGL.fog)
            glEnable(GL_FOG);
        else
            glDisable(GL_FOG);

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
        // Enable alpha test for threshold mode
        if ((gDP.otherMode.alphaCompare == G_AC_THRESHOLD) && !(gDP.otherMode.alphaCvgSel))
        {
            glEnable(GL_ALPHA_TEST);

            glAlphaFunc((gDP.blendColor.a > 0.0f) ? GL_GEQUAL : GL_GREATER, gDP.blendColor.a);
        }
        // Used in TEX_EDGE and similar render modes
        else if (gDP.otherMode.cvgXAlpha)
        {
            glEnable(GL_ALPHA_TEST);

            // Arbitrary number -- gives nice results though
            glAlphaFunc(GL_GEQUAL, 0.5f);
        }
        else
            glDisable(GL_ALPHA_TEST);

        if (OGL.usePolygonStipple && (gDP.otherMode.alphaCompare == G_AC_DITHER) && !(gDP.otherMode.alphaCvgSel))
            glEnable(GL_POLYGON_STIPPLE);
        else
            glDisable(GL_POLYGON_STIPPLE);
    }

    if (gDP.changed & CHANGED_SCISSOR)
    {
        glScissor(gDP.scissor.ulx * OGL.scaleX, (VI.height - gDP.scissor.lry) * OGL.scaleY + OGL.heightOffset,
                  (gDP.scissor.lrx - gDP.scissor.ulx) * OGL.scaleX, (gDP.scissor.lry - gDP.scissor.uly) * OGL.scaleY);
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

    if (gDP.changed & CHANGED_COMBINE_COLORS)
    {
        Combiner_UpdateCombineColors();
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

    if ((gDP.changed & CHANGED_FOGCOLOR) && OGL.fog)
        glFogfv(GL_FOG_COLOR, &gDP.fogColor.r);

    if ((gDP.changed & CHANGED_RENDERMODE) || (gDP.changed & CHANGED_CYCLETYPE))
    {
        if ((gDP.otherMode.forceBlender) &&
            (gDP.otherMode.cycleType != G_CYC_COPY) &&
            (gDP.otherMode.cycleType != G_CYC_FILL) &&
            !(gDP.otherMode.alphaCvgSel))
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

void OGL_AddTriangle(SPVertex* vertices, int v0, int v1, int v2)
{
    int v[] = {v0, v1, v2};

    if (gSP.changed || gDP.changed)
        OGL_UpdateStates();

    //	Playing around with lod fraction junk...
    //	float ds = max( max( fabs( vertices[v0].s - vertices[v1].s ), fabs( vertices[v0].s - vertices[v2].s ) ), fabs( vertices[v1].s - vertices[v2].s ) ) * cache.current[0]->shiftScaleS * gSP.texture.scales;
    //	float dx = max( max( fabs( vertices[v0].x / vertices[v0].w - vertices[v1].x / vertices[v1].w ), fabs( vertices[v0].x / vertices[v0].w - vertices[v2].x / vertices[v2].w ) ), fabs( vertices[v1].x / vertices[v1].w - vertices[v2].x / vertices[v2].w ) ) * gSP.viewport.vscale[0];
    //	float lod = ds / dx;
    //	float lod_fraction = min( 1.0f, max( 0.0f, lod - 1.0f ) / max( 1.0f, gSP.texture.level ) );


    for (int i = 0; i < 3; i++)
    {
        OGL.vertices[OGL.numVertices].x = vertices[v[i]].x;
        OGL.vertices[OGL.numVertices].y = vertices[v[i]].y;
        OGL.vertices[OGL.numVertices].z = gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z * vertices[v[i]].w : vertices[v[i]].z;
        OGL.vertices[OGL.numVertices].w = vertices[v[i]].w;

        OGL.vertices[OGL.numVertices].color.r = vertices[v[i]].r;
        OGL.vertices[OGL.numVertices].color.g = vertices[v[i]].g;
        OGL.vertices[OGL.numVertices].color.b = vertices[v[i]].b;
        OGL.vertices[OGL.numVertices].color.a = vertices[v[i]].a;
        SetConstant(OGL.vertices[OGL.numVertices].color, combiner.vertex.color, combiner.vertex.alpha);
        // SetConstant( OGL.vertices[OGL.numVertices].secondaryColor, combiner.vertex.secondaryColor, ONE );

        if (OGL.EXT_secondary_color)
        {
            OGL.vertices[OGL.numVertices].secondaryColor.r = 0.0f; // lod_fraction; //vertices[v[i]].r;
            OGL.vertices[OGL.numVertices].secondaryColor.g = 0.0f; // lod_fraction; //vertices[v[i]].g;
            OGL.vertices[OGL.numVertices].secondaryColor.b = 0.0f; // lod_fraction; //vertices[v[i]].b;
            OGL.vertices[OGL.numVertices].secondaryColor.a = 1.0f;
            SetConstant(OGL.vertices[OGL.numVertices].secondaryColor, combiner.vertex.secondaryColor, ONE);
        }

        if ((gSP.geometryMode & G_FOG) && OGL.EXT_fog_coord && OGL.fog)
        {
            if (vertices[v[i]].z < -vertices[v[i]].w)
                OGL.vertices[OGL.numVertices].fog = max(0.0f, -(float)gSP.fog.multiplier + (float)gSP.fog.offset);
            else
                OGL.vertices[OGL.numVertices].fog = max(0.0f, vertices[v[i]].z / vertices[v[i]].w * (float)gSP.fog.multiplier + (float)gSP.fog.offset);
        }

        if (combiner.usesT0)
        {
            if (cache.current[0]->frameBufferTexture)
            {
                /*				OGL.vertices[OGL.numVertices].s0 = (cache.current[0]->offsetS + (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls)) * cache.current[0]->scaleS;
                                OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT - (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult)) * cache.current[0]->scaleT;*/

                if (gSP.textureTile[0]->masks)
                    OGL.vertices[OGL.numVertices].s0 = (cache.current[0]->offsetS + (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - fmod(gSP.textureTile[0]->fuls, 1 << gSP.textureTile[0]->masks))) * cache.current[0]->scaleS;
                else
                    OGL.vertices[OGL.numVertices].s0 = (cache.current[0]->offsetS + (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls)) * cache.current[0]->scaleS;

                if (gSP.textureTile[0]->maskt)
                    OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT - (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - fmod(gSP.textureTile[0]->fult, 1 << gSP.textureTile[0]->maskt))) * cache.current[0]->scaleT;
                else
                    OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT - (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult)) * cache.current[0]->scaleT;
            }
            else
            {
                OGL.vertices[OGL.numVertices].s0 = (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls + cache.current[0]->offsetS) * cache.current[0]->scaleS;
                OGL.vertices[OGL.numVertices].t0 = (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult + cache.current[0]->offsetT) * cache.current[0]->scaleT;
            }
        }

        if (combiner.usesT1)
        {
            if (cache.current[0]->frameBufferTexture)
            {
                OGL.vertices[OGL.numVertices].s1 = (cache.current[1]->offsetS + (vertices[v[i]].s * cache.current[1]->shiftScaleS * gSP.texture.scales - gSP.textureTile[1]->fuls)) * cache.current[1]->scaleS;
                OGL.vertices[OGL.numVertices].t1 = (cache.current[1]->offsetT - (vertices[v[i]].t * cache.current[1]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[1]->fult)) * cache.current[1]->scaleT;
            }
            else
            {
                OGL.vertices[OGL.numVertices].s1 = (vertices[v[i]].s * cache.current[1]->shiftScaleS * gSP.texture.scales - gSP.textureTile[1]->fuls + cache.current[1]->offsetS) * cache.current[1]->scaleS;
                OGL.vertices[OGL.numVertices].t1 = (vertices[v[i]].t * cache.current[1]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[1]->fult + cache.current[1]->offsetT) * cache.current[1]->scaleT;
            }
        }
        OGL.numVertices++;
    }
    OGL.numTriangles++;

    if (OGL.numVertices >= 255)
        OGL_DrawTriangles();
}

void OGL_DrawTriangles()
{
    if (OGL.usePolygonStipple && (gDP.otherMode.alphaCompare == G_AC_DITHER) && !(gDP.otherMode.alphaCvgSel))
    {
        OGL.lastStipple = (OGL.lastStipple + 1) & 0x7;
        glPolygonStipple(OGL.stipplePattern[(BYTE)(gDP.envColor.a * 255.0f) >> 3][OGL.lastStipple]);
    }

    glDrawArrays(GL_TRIANGLES, 0, OGL.numVertices);
    OGL.numTriangles = OGL.numVertices = 0;
}

void OGL_DrawLine(SPVertex* vertices, int v0, int v1, float width)
{
    int v[] = {v0, v1};

    GLcolor color;

    if (gSP.changed || gDP.changed)
        OGL_UpdateStates();

    glLineWidth(width * OGL.scaleX);

    glBegin(GL_LINES);
    for (int i = 0; i < 2; i++)
    {
        color.r = vertices[v[i]].r;
        color.g = vertices[v[i]].g;
        color.b = vertices[v[i]].b;
        color.a = vertices[v[i]].a;
        SetConstant(color, combiner.vertex.color, combiner.vertex.alpha);
        glColor4fv(&color.r);

        if (OGL.EXT_secondary_color)
        {
            color.r = vertices[v[i]].r;
            color.g = vertices[v[i]].g;
            color.b = vertices[v[i]].b;
            color.a = vertices[v[i]].a;
            SetConstant(color, combiner.vertex.secondaryColor, combiner.vertex.alpha);
            glSecondaryColor3fvEXT(&color.r);
        }

        glVertex4f(vertices[v[i]].x, vertices[v[i]].y, vertices[v[i]].z, vertices[v[i]].w);
    }
    glEnd();
}

void OGL_DrawRect(int ulx, int uly, int lrx, int lry, float* color)
{
    OGL_UpdateStates();

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, VI.width, VI.height, 0, 1.0f, -1.0f);
    glViewport(0, OGL.heightOffset, OGL.width, OGL.height);
    glDepthRange(0.0f, 1.0f);

    glColor4f(color[0], color[1], color[2], color[3]);

    glBegin(GL_TRIANGLES);
    glVertex4f(ulx, uly, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f, 1.0f);
    glVertex4f(lrx, uly, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f, 1.0f);
    glVertex4f(ulx, lry, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f, 1.0f);

    glVertex4f(lrx, uly, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f, 1.0f);
    glVertex4f(lrx, lry, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f, 1.0f);
    glVertex4f(ulx, lry, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f, 1.0f);
    glEnd();

    glLoadIdentity();
    OGL_UpdateCullFace();
    OGL_UpdateViewport();
    if (!OGL.ignoreScissor)
        glEnable(GL_SCISSOR_TEST);
}

void OGL_DrawTexturedRect(float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip)
{
    GLVertex rect[2] =
    {
    {ulx, uly, gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z : gSP.viewport.nearz, 1.0f, {/*gDP.blendColor.r, gDP.blendColor.g, gDP.blendColor.b, gDP.blendColor.a */ 1.0f, 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, uls, ult, uls, ult, 0.0f},
    {lrx, lry, gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z : gSP.viewport.nearz, 1.0f, {/*gDP.blendColor.r, gDP.blendColor.g, gDP.blendColor.b, gDP.blendColor.a*/ 1.0f, 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, lrs, lrt, lrs, lrt, 0.0f},
    };

    OGL_UpdateStates();

    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, VI.width, VI.height, 0, 1.0f, -1.0f);
    glViewport(0, OGL.heightOffset, OGL.width, OGL.height);

    if (combiner.usesT0)
    {
        rect[0].s0 = rect[0].s0 * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
        rect[0].t0 = rect[0].t0 * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
        rect[1].s0 = (rect[1].s0 + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
        rect[1].t0 = (rect[1].t0 + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;

        if ((cache.current[0]->maskS) && (fmod(rect[0].s0, cache.current[0]->width) == 0.0f) && !(cache.current[0]->mirrorS))
        {
            rect[1].s0 -= rect[0].s0;
            rect[0].s0 = 0.0f;
        }

        if ((cache.current[0]->maskT) && (fmod(rect[0].t0, cache.current[0]->height) == 0.0f) && !(cache.current[0]->mirrorT))
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

        if (OGL.ARB_multitexture)
            glActiveTextureARB(GL_TEXTURE0_ARB);

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

    if (combiner.usesT1 && OGL.ARB_multitexture)
    {
        rect[0].s1 = rect[0].s1 * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
        rect[0].t1 = rect[0].t1 * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
        rect[1].s1 = (rect[1].s1 + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
        rect[1].t1 = (rect[1].t1 + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

        if ((cache.current[1]->maskS) && (fmod(rect[0].s1, cache.current[1]->width) == 0.0f) && !(cache.current[1]->mirrorS))
        {
            rect[1].s1 -= rect[0].s1;
            rect[0].s1 = 0.0f;
        }

        if ((cache.current[1]->maskT) && (fmod(rect[0].t1, cache.current[1]->height) == 0.0f) && !(cache.current[1]->mirrorT))
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

        glActiveTextureARB(GL_TEXTURE1_ARB);

        if ((rect[0].s1 == 0.0f) && (rect[1].s1 <= cache.current[1]->width))
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

        if ((rect[0].t1 == 0.0f) && (rect[1].t1 <= cache.current[1]->height))
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        rect[0].s1 *= cache.current[1]->scaleS;
        rect[0].t1 *= cache.current[1]->scaleT;
        rect[1].s1 *= cache.current[1]->scaleS;
        rect[1].t1 *= cache.current[1]->scaleT;
    }

    if ((gDP.otherMode.cycleType == G_CYC_COPY) && !OGL.forceBilinear)
    {
        if (OGL.ARB_multitexture)
            glActiveTextureARB(GL_TEXTURE0_ARB);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    SetConstant(rect[0].color, combiner.vertex.color, combiner.vertex.alpha);

    if (OGL.EXT_secondary_color)
        SetConstant(rect[0].secondaryColor, combiner.vertex.secondaryColor, combiner.vertex.alpha);

    glBegin(GL_QUADS);
    glColor4f(rect[0].color.r, rect[0].color.g, rect[0].color.b, rect[0].color.a);
    if (OGL.EXT_secondary_color)
        glSecondaryColor3fEXT(rect[0].secondaryColor.r, rect[0].secondaryColor.g, rect[0].secondaryColor.b);

    if (OGL.ARB_multitexture)
    {
        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, rect[0].s0, rect[0].t0);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, rect[0].s1, rect[0].t1);
        glVertex4f(rect[0].x, rect[0].y, rect[0].z, 1.0f);

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, rect[1].s0, rect[0].t0);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, rect[1].s1, rect[0].t1);
        glVertex4f(rect[1].x, rect[0].y, rect[0].z, 1.0f);

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, rect[1].s0, rect[1].t0);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, rect[1].s1, rect[1].t1);
        glVertex4f(rect[1].x, rect[1].y, rect[0].z, 1.0f);

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, rect[0].s0, rect[1].t0);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, rect[0].s1, rect[1].t1);
        glVertex4f(rect[0].x, rect[1].y, rect[0].z, 1.0f);
    }
    else
    {
        glTexCoord2f(rect[0].s0, rect[0].t0);
        glVertex4f(rect[0].x, rect[0].y, rect[0].z, 1.0f);

        if (flip)
            glTexCoord2f(rect[1].s0, rect[0].t0);
        else
            glTexCoord2f(rect[0].s0, rect[1].t0);

        glVertex4f(rect[1].x, rect[0].y, rect[0].z, 1.0f);

        glTexCoord2f(rect[1].s0, rect[1].t0);
        glVertex4f(rect[1].x, rect[1].y, rect[0].z, 1.0f);

        if (flip)
            glTexCoord2f(rect[1].s0, rect[0].t0);
        else
            glTexCoord2f(rect[1].s0, rect[0].t0);
        glVertex4f(rect[0].x, rect[1].y, rect[0].z, 1.0f);
    }
    glEnd();

    glLoadIdentity();
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

void OGL_ClearColorBuffer(float* color)
{
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OGL_SaveScreenshot()
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    HANDLE hBitmapFile;

    gCapturedPixels = malloc(OGL.width * OGL.height * 3);
    OGL_ReadPixels();

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = OGL.width;
    infoHeader.biHeight = OGL.height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = OGL.width * OGL.height * 3;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    fileHeader.bfType = 19778;
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + infoHeader.biSizeImage;
    fileHeader.bfReserved1 = fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    CreateDirectory(screenDirectory.c_str(), NULL);

    const auto filename = std::format(L"{}screen{}.bmp", screenDirectory.wstring(), time(nullptr));
    hBitmapFile = CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

    DWORD written;

    WriteFile(hBitmapFile, &fileHeader, sizeof(BITMAPFILEHEADER), &written, NULL);
    WriteFile(hBitmapFile, &infoHeader, sizeof(BITMAPINFOHEADER), &written, NULL);
    WriteFile(hBitmapFile, gCapturedPixels, infoHeader.biSizeImage, &written, NULL);

    CloseHandle(hBitmapFile);
    free(gCapturedPixels);
}
