/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Config.hpp"
#include "gSP.hpp"

struct GLVertex
{
    float x, y, z, w;

    struct
    {
        float r, g, b, a;
    } color, secondaryColor;

    float s0, t0, s1, t1;
    float fog;
};

struct GLInfo
{
    int32_t context_initialized;

    uint32_t width, height, windowedWidth, windowedHeight;

    int32_t fog;

    float scaleX, scaleY;
    AspectMode aspectMode;
    int32_t adjustScreen;
    float adjustScale;
    float adjustOffset;
    float widescreenScale = 1.0f;

    int maxTextureUnits; // TNT = 2, GeForce = 2-4, Rage 128 = 2, Radeon = 3-6

    uint8_t smoothing;
    TextureFilter textureFilter = TextureFilter::None;
    int32_t msaa{};
    float originAdjust;
    // 2xSAI: 2
    // xBRZ: 2, 3, 4, 5, 6
    // Hqx: 2, 3, 4
    int filterScale = 4;

    GLVertex vertices[256];
    uint8_t triangles[80][3];
    uint8_t numTriangles;
    uint8_t numVertices;

    int32_t usePolygonStipple;
    GLubyte stipplePattern[32][8][128];
    uint8_t lastStipple;

    int32_t ignoreScissor;

    // Clears the game with black color every frame regardless of what N64 asks
    int32_t clear_override = TRUE;

    bool headless{};

    bool isGLES{};
};

extern GLInfo OGL;

struct GLcolor
{
    float r{}, g{}, b{}, a{};
};

void OGL_ReadPixels();
bool OGL_Start();
void OGL_Stop();
void OGL_AddTriangle(SPVertex *vertices, int v0, int v1, int v2);
void OGL_DrawTriangles();
void OGL_DrawLine(SPVertex *vertices, int v0, int v1, float width);
void OGL_DrawRect(int ulx, int uly, int lrx, int lry, float *color);
void OGL_DrawTexturedRect(
    float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip);
void OGL_UpdateScale();
void OGL_SetIdentityProjection();
void OGL_SetOrthoProjection(float left, float right, float bottom, float top, float znear, float zfar);
void OGL_ClearDepthBuffer();
void OGL_ClearColorBuffer(float *color);
void OGL_ResizeWindow();
bool OGL_DestroyContext();
