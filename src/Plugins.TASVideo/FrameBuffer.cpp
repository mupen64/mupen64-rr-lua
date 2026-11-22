/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "Renderer.hpp"
#include "OpenGL.hpp"
#include "FrameBuffer.hpp"
#include "Textures.hpp"
#include "Combiner.hpp"
#include "Types.hpp"

FrameBufferInfo frameBuffer;

void FrameBuffer_Init()
{
    frameBuffer.current = NULL;
    frameBuffer.top = NULL;
    frameBuffer.bottom = NULL;
    frameBuffer.numBuffers = 0;
}

void FrameBuffer_RemoveBottom()
{
    FrameBuffer *newBottom = frameBuffer.bottom->higher;

    TextureCache_Remove(frameBuffer.bottom->texture);

    if (frameBuffer.bottom == frameBuffer.top) frameBuffer.top = NULL;

    free(frameBuffer.bottom);

    frameBuffer.bottom = newBottom;

    if (frameBuffer.bottom != NULL) frameBuffer.bottom->lower = NULL;

    frameBuffer.numBuffers--;
}

void FrameBuffer_Remove(FrameBuffer *buffer)
{
    if ((buffer == frameBuffer.bottom) && (buffer == frameBuffer.top))
    {
        frameBuffer.top = NULL;
        frameBuffer.bottom = NULL;
    }
    else if (buffer == frameBuffer.bottom)
    {
        frameBuffer.bottom = buffer->higher;

        if (frameBuffer.bottom) frameBuffer.bottom->lower = NULL;
    }
    else if (buffer == frameBuffer.top)
    {
        frameBuffer.top = buffer->lower;

        if (frameBuffer.top) frameBuffer.top->higher = NULL;
    }
    else
    {
        buffer->higher->lower = buffer->lower;
        buffer->lower->higher = buffer->higher;
    }

    if (buffer->texture) TextureCache_Remove(buffer->texture);

    free(buffer);

    frameBuffer.numBuffers--;
}

void FrameBuffer_RemoveBuffer(u32 address)
{
    FrameBuffer *current = frameBuffer.bottom;

    while (current != NULL)
    {
        if (current->startAddress == address)
        {
            current->texture = NULL;
            FrameBuffer_Remove(current);
            return;
        }
        current = current->higher;
    }
}

FrameBuffer *FrameBuffer_AddTop()
{
    auto newtop = (FrameBuffer *)malloc(sizeof(FrameBuffer));

    newtop->texture = TextureCache_AddTop();

    newtop->lower = frameBuffer.top;
    newtop->higher = NULL;

    if (frameBuffer.top) frameBuffer.top->higher = newtop;

    if (!frameBuffer.bottom) frameBuffer.bottom = newtop;

    frameBuffer.top = newtop;

    frameBuffer.numBuffers++;

    return newtop;
}

void FrameBuffer_MoveToTop(FrameBuffer *newtop)
{
    if (newtop == frameBuffer.top) return;

    if (newtop == frameBuffer.bottom)
    {
        frameBuffer.bottom = newtop->higher;
        frameBuffer.bottom->lower = NULL;
    }
    else
    {
        newtop->higher->lower = newtop->lower;
        newtop->lower->higher = newtop->higher;
    }

    newtop->higher = NULL;
    newtop->lower = frameBuffer.top;
    frameBuffer.top->higher = newtop;
    frameBuffer.top = newtop;

    TextureCache_MoveToTop(newtop->texture);
}

void FrameBuffer_Destroy()
{
    while (frameBuffer.bottom) FrameBuffer_RemoveBottom();
}

void FrameBuffer_RenderBuffer(u32 address)
{
    FrameBuffer *current = frameBuffer.top;

    while (current != NULL)
    {
        if ((current->startAddress <= address) && (current->endAddress >= address))
        {
            current->changed = FALSE;

            FrameBuffer_MoveToTop(current);

            renderSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
            renderDP.changed |= CHANGED_COMBINE;
            return;
        }
        current = current->lower;
    }
}

void FrameBuffer_RestoreBuffer(u32 address, u16 size, u16 width)
{
    FrameBuffer *current = frameBuffer.top;

    while (current != NULL)
    {
        if ((current->startAddress == address) && (current->width == width) && (current->size == size))
        {
            FrameBuffer_MoveToTop(current);

            renderSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
            renderDP.changed |= CHANGED_COMBINE;
            return;
        }
        current = current->lower;
    }
}

FrameBuffer *FrameBuffer_FindBuffer(u32 address)
{
    FrameBuffer *current = frameBuffer.top;

    while (current)
    {
        if ((current->startAddress <= address) && (current->endAddress >= address)) return current;
        current = current->lower;
    }

    return NULL;
}

void FrameBuffer_ActivateBufferTexture(s16 t, FrameBuffer *buffer)
{
    buffer->texture->scaleS = OGL.scaleX / (float)buffer->texture->realWidth;
    buffer->texture->scaleT = OGL.scaleY / (float)buffer->texture->realHeight;

    if (renderSP.textureTile[t]->shifts > 10)
        buffer->texture->shiftScaleS = (float)(1 << (16 - renderSP.textureTile[t]->shifts));
    else if (renderSP.textureTile[t]->shifts > 0)
        buffer->texture->shiftScaleS = 1.0f / (float)(1 << renderSP.textureTile[t]->shifts);
    else
        buffer->texture->shiftScaleS = 1.0f;

    if (renderSP.textureTile[t]->shiftt > 10)
        buffer->texture->shiftScaleT = (float)(1 << (16 - renderSP.textureTile[t]->shiftt));
    else if (renderSP.textureTile[t]->shiftt > 0)
        buffer->texture->shiftScaleT = 1.0f / (float)(1 << renderSP.textureTile[t]->shiftt);
    else
        buffer->texture->shiftScaleT = 1.0f;

    if (renderDP.loadType == LOADTYPE_TILE)
    {
        buffer->texture->offsetS = renderDP.loadTile->uls;
        buffer->texture->offsetT =
            (float)buffer->height - (renderDP.loadTile->ult + (renderDP.textureImage.address - buffer->startAddress) /
                                                                  (buffer->width << buffer->size >> 1));
    }
    else
    {
        buffer->texture->offsetS = 0.0f;
        buffer->texture->offsetT = (float)buffer->height - (renderDP.textureImage.address - buffer->startAddress) /
                                                               (buffer->width << buffer->size >> 1);
    }

    FrameBuffer_MoveToTop(buffer);
    TextureCache_ActivateTexture(t, buffer->texture);
}
