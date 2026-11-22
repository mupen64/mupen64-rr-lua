/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Types.hpp"

// Useful macros for decoding GBI command's parameters
#define _SHIFTL(v, s, w) (((u32)v & ((0x01 << w) - 1)) << s)
#define _SHIFTR(v, s, w) (((u32)v >> s) & ((0x01 << w) - 1))

// Image formats
#define G_IM_FMT_RGBA 0
#define G_IM_FMT_YUV 1
#define G_IM_FMT_CI 2
#define G_IM_FMT_IA 3
#define G_IM_FMT_I 4

// Image sizes
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3
#define G_IM_SIZ_DD 5

#define G_TX_MIRROR 0x1
#define G_TX_CLAMP 0x2

/* G_SETOTHERMODE_H gPipelineMode */
#define G_PM_1PRIMITIVE 1
#define G_PM_NPRIMITIVE 0

/* G_SETOTHERMODE_H gSetCycleType */
#define G_CYC_1CYCLE 0
#define G_CYC_2CYCLE 1
#define G_CYC_COPY 2
#define G_CYC_FILL 3

/* G_SETOTHERMODE_H gSetTexturePersp */
#define G_TP_NONE 0
#define G_TP_PERSP 1

/* G_SETOTHERMODE_H gSetTextureDetail */
#define G_TD_CLAMP 0
#define G_TD_SHARPEN 1
#define G_TD_DETAIL 2

/* G_SETOTHERMODE_H gSetTextureLOD */
#define G_TL_TILE 0
#define G_TL_LOD 1

/* G_SETOTHERMODE_H gSetTextureLUT */
#define G_TT_NONE 0
#define G_TT_RGBA16 2
#define G_TT_IA16 3

/* G_SETOTHERMODE_H gSetTextureFilter */
#define G_TF_POINT 0
#define G_TF_AVERAGE 3
#define G_TF_BILERP 2

/* G_SETOTHERMODE_H gSetTextureConvert */
#define G_TC_CONV 0
#define G_TC_FILTCONV 5
#define G_TC_FILT 6

/* G_SETOTHERMODE_H gSetCombineKey */
#define G_CK_NONE 0
#define G_CK_KEY 1

/* G_SETOTHERMODE_H gSetColorDither */
#define G_CD_MAGICSQ 0
#define G_CD_BAYER 1
#define G_CD_NOISE 2

#define G_CD_DISABLE 3
#define G_CD_ENABLE G_CD_NOISE /* HW 1.0 compatibility mode */

/* G_SETOTHERMODE_H gSetAlphaDither */
#define G_AD_PATTERN 0
#define G_AD_NOTPATTERN 1
#define G_AD_NOISE 2
#define G_AD_DISABLE 3

/* G_SETOTHERMODE_L gSetAlphaCompare */
#define G_AC_NONE 0
#define G_AC_THRESHOLD 1
#define G_AC_DITHER 3

/* G_SETOTHERMODE_L gSetDepthSource */
#define G_ZS_PIXEL 0
#define G_ZS_PRIM 1

/* G_SETOTHERMODE_L gSetRenderMode */
#define AA_EN 1
#define Z_CMP 1
#define Z_UPD 1
#define IM_RD 1
#define CLR_ON_CVG 1
#define CVG_DST_CLAMP 0
#define CVG_DST_WRAP 1
#define CVG_DST_FULL 2
#define CVG_DST_SAVE 3
#define ZMODE_OPA 0
#define ZMODE_INTER 1
#define ZMODE_XLU 2
#define ZMODE_DEC 3
#define CVG_X_ALPHA 1
#define ALPHA_CVG_SEL 1
#define FORCE_BL 1
#define TEX_EDGE 0 // not used

#define G_SC_NON_INTERLACE 0
#define G_SC_EVEN_INTERLACE 2
#define G_SC_ODD_INTERLACE 3

/* Color combiner constants: */
#define G_CCMUX_COMBINED 0
#define G_CCMUX_TEXEL0 1
#define G_CCMUX_TEXEL1 2
#define G_CCMUX_PRIMITIVE 3
#define G_CCMUX_SHADE 4
#define G_CCMUX_ENVIRONMENT 5
#define G_CCMUX_CENTER 6
#define G_CCMUX_SCALE 6
#define G_CCMUX_COMBINED_ALPHA 7
#define G_CCMUX_TEXEL0_ALPHA 8
#define G_CCMUX_TEXEL1_ALPHA 9
#define G_CCMUX_PRIMITIVE_ALPHA 10
#define G_CCMUX_SHADE_ALPHA 11
#define G_CCMUX_ENV_ALPHA 12
#define G_CCMUX_LOD_FRACTION 13
#define G_CCMUX_PRIM_LOD_FRAC 14
#define G_CCMUX_NOISE 7
#define G_CCMUX_K4 7
#define G_CCMUX_K5 15
#define G_CCMUX_1 6
#define G_CCMUX_0 31

/* Alpha combiner constants: */
#define G_ACMUX_COMBINED 0
#define G_ACMUX_TEXEL0 1
#define G_ACMUX_TEXEL1 2
#define G_ACMUX_PRIMITIVE 3
#define G_ACMUX_SHADE 4
#define G_ACMUX_ENVIRONMENT 5
#define G_ACMUX_LOD_FRACTION 0
#define G_ACMUX_PRIM_LOD_FRAC 6
#define G_ACMUX_1 6
#define G_ACMUX_0 7
