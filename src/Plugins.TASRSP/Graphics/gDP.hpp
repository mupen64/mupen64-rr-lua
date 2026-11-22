/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "RenderState.hpp"

struct gDPInfo : DPDrawState
{
    gDPTile *loadTile;
    u32 half_1, half_2;

    struct
    {
        u32 format, size, width, height, bpl;
        u32 address, changed;
        u32 depthImage;
    } colorImage;

    u32 depthImageAddress;
};

extern gDPInfo gDP;

void gDPSetOtherMode(u32 mode0, u32 mode1);
void gDPSetPrimDepth(u16 z, u16 dz);
void gDPPipelineMode(u32 mode);
void gDPSetCycleType(u32 type);
void gDPSetTexturePersp(u32 enable);
void gDPSetTextureDetail(u32 type);
void gDPSetTextureLOD(u32 mode);
void gDPSetTextureLUT(u32 mode);
void gDPSetTextureFilter(u32 type);
void gDPSetTextureConvert(u32 type);
void gDPSetCombineKey(u32 type);
void gDPSetColorDither(u32 type);
void gDPSetAlphaDither(u32 type);
void gDPSetAlphaCompare(u32 mode);
void gDPSetDepthSource(u32 source);
void gDPSetRenderMode(u32 mode1, u32 mode2);
void gDPSetCombine(s32 muxs0, s32 muxs1);
void gDPSetColorImage(u32 format, u32 size, u32 width, u32 address);
void gDPSetTextureImage(u32 format, u32 size, u32 width, u32 address);
void gDPSetDepthImage(u32 address);
void gDPSetEnvColor(u32 r, u32 g, u32 b, u32 a);
void gDPSetBlendColor(u32 r, u32 g, u32 b, u32 a);
void gDPSetFogColor(u32 r, u32 g, u32 b, u32 a);
void gDPSetFillColor(u32 c);
void gDPSetPrimColor(u32 m, u32 l, u32 r, u32 g, u32 b, u32 a);
void gDPSetTile(u32 format, u32 size, u32 line, u32 tmem, u32 tile, u32 palette, u32 cmt, u32 cms, u32 maskt, u32 masks,
    u32 shiftt, u32 shifts);
void gDPSetTileSize(u32 tile, u32 uls, u32 ult, u32 lrs, u32 lrt);
void gDPLoadTile(u32 tile, u32 uls, u32 ult, u32 lrs, u32 lrt);
void gDPLoadBlock(u32 tile, u32 uls, u32 ult, u32 lrs, u32 dxt);
void gDPLoadTLUT(u32 tile, u32 uls, u32 ult, u32 lrs, u32 lrt);
void gDPSetScissor(u32 mode, f32 ulx, f32 uly, f32 lrx, f32 lry);
void gDPFillRectangle(s32 ulx, s32 uly, s32 lrx, s32 lry);
void gDPSetConvert(s32 k0, s32 k1, s32 k2, s32 k3, s32 k4, s32 k5);
void gDPSetKeyR(u32 cR, u32 sR, u32 wR);
void gDPSetKeyGB(u32 cG, u32 sG, u32 wG, u32 cB, u32 sB, u32 wB);
void gDPTextureRectangle(
    f32 ulx, f32 uly, f32 lrx, f32 lry, s32 tile, f32 s, f32 t, f32 dsdx, f32 dtdy, bool flip = false);
void gDPTextureRectangleFlip(f32 ulx, f32 uly, f32 lrx, f32 lry, s32 tile, f32 s, f32 t, f32 dsdx, f32 dtdy);
void gDPFullSync();
void gDPTileSync();
void gDPPipeSync();
void gDPLoadSync();
void gDPNoOp();
