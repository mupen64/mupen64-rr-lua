/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Types.hpp"
#include <span>

#define CHANGED_VIEWPORT 0x01
#define CHANGED_GEOMETRYMODE 0x08
#define CHANGED_TEXTURE 0x10
#define CHANGED_FOGPOSITION 0x10

#define CHANGED_RENDERMODE 0x001
#define CHANGED_CYCLETYPE 0x002
#define CHANGED_SCISSOR 0x004
#define CHANGED_TMEM 0x008
#define CHANGED_TILE 0x010
#define CHANGED_COMBINE_COLORS 0x020
#define CHANGED_COMBINE 0x040
#define CHANGED_ALPHACOMPARE 0x080
#define CHANGED_FOGCOLOR 0x100

#define TEXTUREMODE_NORMAL 0
#define TEXTUREMODE_TEXRECT 1
#define TEXTUREMODE_BGIMAGE 2

#define LOADTYPE_BLOCK 0
#define LOADTYPE_TILE 1

struct gDPCombine
{
    union {
        struct
        {
            // muxs1
            unsigned aA1 : 3;
            unsigned sbA1 : 3;
            unsigned aRGB1 : 3;
            unsigned aA0 : 3;
            unsigned sbA0 : 3;
            unsigned aRGB0 : 3;
            unsigned mA1 : 3;
            unsigned saA1 : 3;
            unsigned sbRGB1 : 4;
            unsigned sbRGB0 : 4;

            // muxs0
            unsigned mRGB1 : 5;
            unsigned saRGB1 : 4;
            unsigned mA0 : 3;
            unsigned saA0 : 3;
            unsigned mRGB0 : 5;
            unsigned saRGB0 : 4;
        };

        struct
        {
            u32 muxs1, muxs0;
        };

        u64 mux;
    };
};

struct gDPTile
{
    u32 format, size, line, tmem, palette;

    union {
        struct
        {
            unsigned mirrort : 1;
            unsigned clampt : 1;
            unsigned pad0 : 30;

            unsigned mirrors : 1;
            unsigned clamps : 1;
            unsigned pad1 : 30;
        };

        struct
        {
            u32 cmt, cms;
        };
    };

    u32 maskt, masks;
    u32 shiftt, shifts;
    f32 fuls, fult, flrs, flrt;
    u32 uls, ult, lrs, lrt;
};

struct DPDrawState
{
    struct
    {
        union {
            struct
            {
                unsigned int alphaCompare : 2;
                unsigned int depthSource : 1;

                //				struct
                //				{
                unsigned int AAEnable : 1;
                unsigned int depthCompare : 1;
                unsigned int depthUpdate : 1;
                unsigned int imageRead : 1;
                unsigned int clearOnCvg : 1;

                unsigned int cvgDest : 2;
                unsigned int depthMode : 2;

                unsigned int cvgXAlpha : 1;
                unsigned int alphaCvgSel : 1;
                unsigned int forceBlender : 1;
                unsigned int textureEdge : 1;
                //				} renderMode;

                //				struct
                //				{
                unsigned int c2_m2b : 2;
                unsigned int c1_m2b : 2;
                unsigned int c2_m2a : 2;
                unsigned int c1_m2a : 2;
                unsigned int c2_m1b : 2;
                unsigned int c1_m1b : 2;
                unsigned int c2_m1a : 2;
                unsigned int c1_m1a : 2;
                //				} blender;

                unsigned int blendMask : 4;
                unsigned int alphaDither : 2;
                unsigned int colorDither : 2;

                unsigned int combineKey : 1;
                unsigned int textureConvert : 3;
                unsigned int textureFilter : 2;
                unsigned int textureLUT : 2;

                unsigned int textureLOD : 1;
                unsigned int textureDetail : 2;
                unsigned int texturePersp : 1;
                unsigned int cycleType : 2;
                unsigned int unusedColorDither : 1; // unsupported
                unsigned int pipelineMode : 1;

                unsigned int pad : 8;
            };

            u64 _u64;

            struct
            {
                u32 l, h;
            };
        };
    } otherMode;

    gDPCombine combine;

    gDPTile tiles[8];

    struct
    {
        f32 r, g, b, a;
    } fogColor, blendColor, envColor;

    struct
    {
        f32 r, g, b, a;
        f32 z, dz;
    } fillColor;

    struct
    {
        u32 m;
        f32 l, r, g, b, a;
    } primColor;

    struct
    {
        f32 z, deltaZ;
    } primDepth;

    struct
    {
        u32 format, size, width, bpl;
        u32 address;
    } textureImage;

    struct
    {
        u32 mode;
        f32 ulx, uly, lrx, lry;
    } scissor;

    struct
    {
        s32 k0, k1, k2, k3, k4, k5;
    } convert;

    struct
    {
        struct
        {
            f32 r, g, b, a;
        } center, scale, width;
    } key;

    struct
    {
        u32 width, height;
    } texRect;

    u32 changed;

    // u16 palette[256];
    u32 paletteCRC16[16];
    u32 paletteCRC256;
    u32 textureMode;
    u32 loadType;
};

struct SPDrawState
{
    struct
    {
        f32 scales, scalet;
        s32 level, on, tile;
    } texture;

    struct
    {
        f32 vscale[4];
        f32 vtrans[4];
        f32 x, y, width, height;
        f32 nearz, farz;
    } viewport;

    struct
    {
        s16 multiplier, offset;
    } fog;

    struct
    {
        u32 address, width, height, format, size, palette;
    } bgImage;
};

struct RenderState
{
    SPDrawState sp;
    DPDrawState dp;
    u32 sp_changed;
    u32 texture_tiles[2];
    u32 load_tile;
    u32 texture_memory;
    std::span<const u8> background_image;
    bool depth_test;
    bool fog;
    bool cull_front;
    bool cull_back;
};
