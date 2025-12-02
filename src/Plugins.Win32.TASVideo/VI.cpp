#include "stdafx.h"
#include "glN64.h"
#include "VI.h"
#include "OpenGL.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"
#include "FrameBuffer.h"
#include "Debug.h"

VIInfo VI;

void VI_UpdateSize()
{
    f32 xScale = _FIXED2FLOAT(_SHIFTR(*REG.VI_X_SCALE, 0, 12), 10);
    f32 xOffset = _FIXED2FLOAT(_SHIFTR(*REG.VI_X_SCALE, 16, 12), 10);

    f32 yScale = _FIXED2FLOAT(_SHIFTR(*REG.VI_Y_SCALE, 0, 12), 10);
    f32 yOffset = _FIXED2FLOAT(_SHIFTR(*REG.VI_Y_SCALE, 16, 12), 10);

    u32 hEnd = _SHIFTR(*REG.VI_H_START, 0, 10);
    u32 hStart = _SHIFTR(*REG.VI_H_START, 16, 10);

    // These are in half-lines, so shift an extra bit
    u32 vEnd = _SHIFTR(*REG.VI_V_START, 1, 9);
    u32 vStart = _SHIFTR(*REG.VI_V_START, 17, 9);

    VI.width = (hEnd - hStart) * xScale;
    VI.height = (vEnd - vStart) * yScale * 1.0126582f;

    if (VI.width == 0.0f)
        VI.width = 320.0f;
    if (VI.height == 0.0f)
        VI.height = 240.0f;
}

void VI_UpdateScreen()
{
    glFinish();

    if (gSP.changed & CHANGED_COLORBUFFER)
    {
        SwapBuffers(OGL.hDC);
        gSP.changed &= ~CHANGED_COLORBUFFER;
    }
    
    glFinish();
}
