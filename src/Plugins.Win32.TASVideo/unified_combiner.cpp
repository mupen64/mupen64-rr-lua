#include "stdafx.h"
#include "OpenGL.h"
#include "Combiner.h"
#include "unified_combiner.h"
#include "Textures.h"
#include "gDP.h"

struct UCTexArg
{
    GLenum source, operand;
};

struct UCTexStage
{
    WORD constant;
    BOOL used;
    GLenum combine;
    UCTexArg arg0, arg1, arg2;
    WORD outputTexture;
};

struct UnifiedCompiledCombiner
{
    BOOL usesT0, usesT1, usesNoise;
    WORD usedUnits;
    struct
    {
        WORD color, secondaryColor, alpha;
    } vertex;
    UCTexStage color[8];
    UCTexStage alpha[8];
};

static UCTexArg TexEnvArgs[] = {
    // CMB
    {GL_PREVIOUS_ARB, GL_SRC_COLOR},
    // T0
    {GL_TEXTURE, GL_SRC_COLOR},
    // T1
    {GL_TEXTURE, GL_SRC_COLOR},
    // PRIM
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // SHADE
    {GL_PRIMARY_COLOR_ARB, GL_SRC_COLOR},
    // ENV
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // CENTER
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // SCALE
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // CMBALPHA
    {GL_PREVIOUS_ARB, GL_SRC_ALPHA},
    // T0ALPHA
    {GL_TEXTURE, GL_SRC_ALPHA},
    // T1ALPHA
    {GL_TEXTURE, GL_SRC_ALPHA},
    // PRIMALPHA
    {GL_CONSTANT_ARB, GL_SRC_ALPHA},
    // SHADEALPHA
    {GL_PRIMARY_COLOR_ARB, GL_SRC_ALPHA},
    // ENVALPHA
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // LODFRAC
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // PRIMLODFRAC
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // NOISE
    {GL_TEXTURE, GL_SRC_COLOR},
    // K4
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // K5
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // ONE
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
    // ZERO
    {GL_CONSTANT_ARB, GL_SRC_COLOR},
};

#define SetColorCombinerArg(n, a, i)                                                                                   \
    if (TexEnvArgs[i].source == GL_CONSTANT_ARB)                                                                       \
    {                                                                                                                  \
        if ((i > 5) && ((envCombiner->alpha[n].constant == COMBINED) || (envCombiner->alpha[n].constant == i)))        \
        {                                                                                                              \
            envCombiner->alpha[n].constant = i;                                                                        \
            envCombiner->color[n].a.source = GL_CONSTANT_ARB;                                                          \
            envCombiner->color[n].a.operand = GL_SRC_ALPHA;                                                            \
        }                                                                                                              \
        else if ((i > 5) && ((envCombiner->vertex.alpha == COMBINED) || (envCombiner->vertex.alpha == i)))             \
        {                                                                                                              \
            envCombiner->vertex.alpha = i;                                                                             \
            envCombiner->color[n].a.source = GL_PRIMARY_COLOR_ARB;                                                     \
            envCombiner->color[n].a.operand = GL_SRC_ALPHA;                                                            \
        }                                                                                                              \
        else if ((envCombiner->color[n].constant == COMBINED) || (envCombiner->color[n].constant == i))                \
        {                                                                                                              \
            envCombiner->color[n].constant = i;                                                                        \
            envCombiner->color[n].a.source = GL_CONSTANT_ARB;                                                          \
            envCombiner->color[n].a.operand = GL_SRC_COLOR;                                                            \
        }                                                                                                              \
        else if ((envCombiner->vertex.color == COMBINED) || (envCombiner->vertex.color == i))                          \
        {                                                                                                              \
            envCombiner->vertex.color = i;                                                                             \
            envCombiner->color[n].a.source = GL_PRIMARY_COLOR_ARB;                                                     \
            envCombiner->color[n].a.operand = GL_SRC_COLOR;                                                            \
        }                                                                                                              \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        envCombiner->color[n].a.source = TexEnvArgs[i].source;                                                         \
        envCombiner->color[n].a.operand = TexEnvArgs[i].operand;                                                       \
    }

#define SetColorCombinerValues(n, a, s, o)                                                                             \
    envCombiner->color[n].a.source = s;                                                                                \
    envCombiner->color[n].a.operand = o

#define SetAlphaCombinerArg(n, a, i)                                                                                   \
    if (TexEnvArgs[i].source == GL_CONSTANT_ARB)                                                                       \
    {                                                                                                                  \
        if ((envCombiner->alpha[n].constant == COMBINED) || (envCombiner->alpha[n].constant == i))                     \
        {                                                                                                              \
            envCombiner->alpha[n].constant = i;                                                                        \
            envCombiner->alpha[n].a.source = GL_CONSTANT_ARB;                                                          \
            envCombiner->alpha[n].a.operand = GL_SRC_ALPHA;                                                            \
        }                                                                                                              \
        else if ((envCombiner->vertex.alpha == COMBINED) || (envCombiner->vertex.alpha == i))                          \
        {                                                                                                              \
            envCombiner->vertex.alpha = i;                                                                             \
            envCombiner->alpha[n].a.source = GL_PRIMARY_COLOR_ARB;                                                     \
            envCombiner->alpha[n].a.operand = GL_SRC_ALPHA;                                                            \
        }                                                                                                              \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        envCombiner->alpha[n].a.source = TexEnvArgs[i].source;                                                         \
        envCombiner->alpha[n].a.operand = GL_SRC_ALPHA;                                                                \
    }

#define SetAlphaCombinerValues(n, a, s, o)                                                                             \
    envCombiner->alpha[n].a.source = s;                                                                                \
    envCombiner->alpha[n].a.operand = o

static void Init()
{
    for (int i = 0; i < OGL.maxTextureUnits; i++) TextureCache_ActivateDummy(i);

    // TEXTURE_ENV_COMBINE is now required
    TexEnvArgs[TEXEL0].source = GL_TEXTURE0_ARB;
    TexEnvArgs[TEXEL0_ALPHA].source = GL_TEXTURE0_ARB;
    TexEnvArgs[TEXEL1].source = GL_TEXTURE1_ARB;
    TexEnvArgs[TEXEL1_ALPHA].source = GL_TEXTURE1_ARB;
}

static void Uninit()
{
    for (int i = 0; i < OGL.maxTextureUnits; i++)
    {
        glActiveTextureARB(GL_TEXTURE0_ARB + i);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    }
}

static void UpdateColors(UnifiedCompiledCombiner *envCombiner)
{
    GLcolor color;

    for (int i = 0; i < OGL.maxTextureUnits; i++)
    {
        SetConstant(color, envCombiner->color[i].constant, envCombiner->alpha[i].constant);
        glActiveTextureARB(GL_TEXTURE0_ARB + i);
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &color.r);
    }
}

static void Compile(UnifiedCompiledCombiner *envCombiner, Combiner *color, Combiner *alpha)
{
    int curUnit, combinedUnit;

    for (int i = 0; i < OGL.maxTextureUnits; i++)
    {
        envCombiner->color[i].combine = GL_REPLACE;
        envCombiner->alpha[i].combine = GL_REPLACE;

        SetColorCombinerValues(i, arg0, GL_PREVIOUS_ARB, GL_SRC_COLOR);
        SetColorCombinerValues(i, arg1, GL_PREVIOUS_ARB, GL_SRC_COLOR);
        SetColorCombinerValues(i, arg2, GL_PREVIOUS_ARB, GL_SRC_COLOR);
        envCombiner->color[i].constant = COMBINED;
        envCombiner->color[i].outputTexture = GL_TEXTURE0_ARB + i;

        SetAlphaCombinerValues(i, arg0, GL_PREVIOUS_ARB, GL_SRC_ALPHA);
        SetAlphaCombinerValues(i, arg1, GL_PREVIOUS_ARB, GL_SRC_ALPHA);
        SetAlphaCombinerValues(i, arg2, GL_PREVIOUS_ARB, GL_SRC_ALPHA);
        envCombiner->alpha[i].constant = COMBINED;
        envCombiner->alpha[i].outputTexture = GL_TEXTURE0_ARB + i;
    }

    envCombiner->usesT0 = FALSE;
    envCombiner->usesT1 = FALSE;

    envCombiner->vertex.color = COMBINED;
    envCombiner->vertex.secondaryColor = COMBINED;
    envCombiner->vertex.alpha = COMBINED;

    curUnit = 0;

    for (int i = 0; i < alpha->numStages; i++)
    {
        for (int j = 0; j < alpha->stage[i].numOps; j++)
        {
            float sb = 0.0f;
            if (alpha->stage[i].op[j].param1 == PRIMITIVE_ALPHA)
                sb = gDP.primColor.a;
            else if (alpha->stage[i].op[j].param1 == ENV_ALPHA)
                sb = gDP.envColor.a;
            else if (alpha->stage[i].op[j].param1 == ONE)
                sb = 1.0f;

            if (((alpha->stage[i].numOps - j) >= 3) && (alpha->stage[i].op[j].op == SUB) &&
                (alpha->stage[i].op[j + 1].op == MUL) && (alpha->stage[i].op[j + 2].op == ADD) && (sb > 0.5f))
            {
                envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
                envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

                if (alpha->stage[i].op[j].param1 == ONE)
                {
                    SetAlphaCombinerValues(curUnit, arg0, envCombiner->alpha[curUnit].arg0.source,
                                           GL_ONE_MINUS_SRC_ALPHA);
                }
                else
                {
                    envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
                    SetAlphaCombinerValues(curUnit, arg1, envCombiner->alpha[curUnit].arg0.source, GL_SRC_ALPHA);
                    SetAlphaCombinerArg(curUnit, arg0, alpha->stage[i].op[j].param1);
                    curUnit++;
                }
                j++;

                envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
                envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;
                envCombiner->alpha[curUnit].combine = GL_MODULATE;
                SetAlphaCombinerArg(curUnit, arg1, alpha->stage[i].op[j].param1);
                curUnit++;
                j++;

                envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
                envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;
                envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
                SetAlphaCombinerArg(curUnit, arg0, alpha->stage[i].op[j].param1);
                curUnit++;
            }
            else
            {
                envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
                envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

                switch (alpha->stage[i].op[j].op)
                {
                case LOAD:
                    envCombiner->alpha[curUnit].combine = GL_REPLACE;
                    SetAlphaCombinerArg(curUnit, arg0, alpha->stage[i].op[j].param1);
                    break;

                case SUB:
                    if ((j > 0) && (alpha->stage[i].op[j - 1].op == LOAD) && (alpha->stage[i].op[j - 1].param1 == ONE))
                    {
                        SetAlphaCombinerArg(curUnit, arg0, alpha->stage[i].op[j].param1);
                        envCombiner->alpha[curUnit].arg0.operand = GL_ONE_MINUS_SRC_ALPHA;
                    }
                    else
                    {
                        envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
                        SetAlphaCombinerArg(curUnit, arg1, alpha->stage[i].op[j].param1);
                        curUnit++;
                    }
                    break;

                case MUL:
                    envCombiner->alpha[curUnit].combine = GL_MODULATE;
                    SetAlphaCombinerArg(curUnit, arg1, alpha->stage[i].op[j].param1);
                    curUnit++;
                    break;

                case ADD:
                    envCombiner->alpha[curUnit].combine = GL_ADD;
                    SetAlphaCombinerArg(curUnit, arg1, alpha->stage[i].op[j].param1);
                    curUnit++;
                    break;

                case INTER:
                    envCombiner->usesT0 |= (alpha->stage[i].op[j].param2 == TEXEL0_ALPHA) ||
                                           (alpha->stage[i].op[j].param3 == TEXEL0_ALPHA);
                    envCombiner->usesT1 |= (alpha->stage[i].op[j].param2 == TEXEL1_ALPHA) ||
                                           (alpha->stage[i].op[j].param3 == TEXEL1_ALPHA);
                    envCombiner->alpha[curUnit].combine = GL_INTERPOLATE_ARB;
                    SetAlphaCombinerArg(curUnit, arg0, alpha->stage[i].op[j].param1);
                    SetAlphaCombinerArg(curUnit, arg1, alpha->stage[i].op[j].param2);
                    SetAlphaCombinerArg(curUnit, arg2, alpha->stage[i].op[j].param3);
                    curUnit++;
                    break;
                }
            }
        }
        combinedUnit = max(curUnit - 1, 0);
    }

    envCombiner->usedUnits = max(curUnit, 1);

    curUnit = 0;
    for (int i = 0; i < color->numStages; i++)
    {
        for (int j = 0; j < color->stage[i].numOps; j++)
        {
            float sb = 0.0f;
            if (color->stage[i].op[j].param1 == PRIMITIVE)
                sb = (gDP.primColor.r + gDP.primColor.b + gDP.primColor.g) / 3.0f;
            else if (color->stage[i].op[j].param1 == ENVIRONMENT)
                sb = (gDP.envColor.r + gDP.envColor.b + gDP.envColor.g) / 3.0f;

            if (((color->stage[i].numOps - j) >= 3) && (color->stage[i].op[j].op == SUB) &&
                (color->stage[i].op[j + 1].op == MUL) && (color->stage[i].op[j + 2].op == ADD) && (sb > 0.5f))
            {
                envCombiner->usesT0 |=
                    (color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA);
                envCombiner->usesT1 |=
                    (color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA);

                envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
                SetColorCombinerValues(curUnit, arg1, envCombiner->color[curUnit].arg0.source,
                                       envCombiner->color[curUnit].arg0.operand);
                SetColorCombinerArg(curUnit, arg0, color->stage[i].op[j].param1);
                curUnit++;
                j++;

                envCombiner->usesT0 |=
                    (color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA);
                envCombiner->usesT1 |=
                    (color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA);
                envCombiner->color[curUnit].combine = GL_MODULATE;
                SetColorCombinerArg(curUnit, arg1, color->stage[i].op[j].param1);
                curUnit++;
                j++;

                envCombiner->usesT0 |=
                    (color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA);
                envCombiner->usesT1 |=
                    (color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA);
                envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
                SetColorCombinerArg(curUnit, arg0, color->stage[i].op[j].param1);
                curUnit++;
            }
            else
            {
                envCombiner->usesT0 |=
                    (color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA);
                envCombiner->usesT1 |=
                    (color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA);

                switch (color->stage[i].op[j].op)
                {
                case LOAD:
                    envCombiner->color[curUnit].combine = GL_REPLACE;
                    SetColorCombinerArg(curUnit, arg0, color->stage[i].op[j].param1);
                    break;

                case SUB:
                    if ((j > 0) && (color->stage[i].op[j - 1].op == LOAD) && (color->stage[i].op[j - 1].param1 == ONE))
                    {
                        SetColorCombinerArg(curUnit, arg0, color->stage[i].op[j].param1);
                        envCombiner->color[curUnit].arg0.operand = GL_ONE_MINUS_SRC_COLOR;
                    }
                    else
                    {
                        envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
                        SetColorCombinerArg(curUnit, arg1, color->stage[i].op[j].param1);
                        curUnit++;
                    }
                    break;

                case MUL:
                    envCombiner->color[curUnit].combine = GL_MODULATE;
                    SetColorCombinerArg(curUnit, arg1, color->stage[i].op[j].param1);
                    curUnit++;
                    break;

                case ADD:
                    envCombiner->color[curUnit].combine = GL_ADD;
                    SetColorCombinerArg(curUnit, arg1, color->stage[i].op[j].param1);
                    curUnit++;
                    break;

                case INTER:
                    envCombiner->usesT0 |= (color->stage[i].op[j].param2 == TEXEL0) ||
                                           (color->stage[i].op[j].param3 == TEXEL0) ||
                                           (color->stage[i].op[j].param3 == TEXEL0_ALPHA);
                    envCombiner->usesT1 |= (color->stage[i].op[j].param2 == TEXEL1) ||
                                           (color->stage[i].op[j].param3 == TEXEL1) ||
                                           (color->stage[i].op[j].param3 == TEXEL1_ALPHA);

                    envCombiner->color[curUnit].combine = GL_INTERPOLATE_ARB;
                    SetColorCombinerArg(curUnit, arg0, color->stage[i].op[j].param1);
                    SetColorCombinerArg(curUnit, arg1, color->stage[i].op[j].param2);
                    SetColorCombinerArg(curUnit, arg2, color->stage[i].op[j].param3);
                    curUnit++;
                    break;
                }
            }
        }
        combinedUnit = max(curUnit - 1, 0);
    }

    envCombiner->usedUnits = max(curUnit, (int)envCombiner->usedUnits);
}

static void Set(UnifiedCompiledCombiner *envCombiner)
{
    combiner.usesT0 = envCombiner->usesT0;
    combiner.usesT1 = envCombiner->usesT1;
    combiner.usesNoise = FALSE;

    combiner.vertex.color = envCombiner->vertex.color;
    combiner.vertex.secondaryColor = envCombiner->vertex.secondaryColor;
    combiner.vertex.alpha = envCombiner->vertex.alpha;

    for (int i = 0; i < OGL.maxTextureUnits; i++)
    {
        glActiveTextureARB(GL_TEXTURE0_ARB + i);

        if ((i < envCombiner->usedUnits) || ((i < 2) && envCombiner->usesT1))
        {
            glEnable(GL_TEXTURE_2D);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, envCombiner->color[i].combine);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, envCombiner->color[i].arg0.source);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, envCombiner->color[i].arg0.operand);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, envCombiner->color[i].arg1.source);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, envCombiner->color[i].arg1.operand);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, envCombiner->color[i].arg2.source);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, envCombiner->color[i].arg2.operand);

            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, envCombiner->alpha[i].combine);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, envCombiner->alpha[i].arg0.source);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, envCombiner->alpha[i].arg0.operand);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, envCombiner->alpha[i].arg1.source);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, envCombiner->alpha[i].arg1.operand);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, envCombiner->alpha[i].arg2.source);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, envCombiner->alpha[i].arg2.operand);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }
    }
}

void Init_unified_combiner()
{
    Init();
}

void Uninit_unified_combiner()
{
    Uninit();
}

UnifiedCompiledCombiner *Compile_unified_combiner(Combiner *color, Combiner *alpha)
{
    auto compiled = (UnifiedCompiledCombiner *)malloc(sizeof(UnifiedCompiledCombiner));
    Compile(compiled, color, alpha);
    return compiled;
}

void Update_unified_combiner_Colors(UnifiedCompiledCombiner *compiled)
{
    UpdateColors(compiled);
}

void Set_unified_combiner(UnifiedCompiledCombiner *compiled)
{
    Set(compiled);
}

void BeginTextureUpdate_unified_combiner()
{
    for (int i = 0; i < OGL.maxTextureUnits; i++)
    {
        glActiveTextureARB(GL_TEXTURE0_ARB + i);
        glDisable(GL_TEXTURE_2D);
    }
}

void EndTextureUpdate_unified_combiner()
{
    if (combiner.current && combiner.current->compiled) Set_unified_combiner(combiner.current->compiled);
}
