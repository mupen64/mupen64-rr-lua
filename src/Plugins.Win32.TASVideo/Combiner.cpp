#include "stdafx.h"
#include "OpenGL.h"
#include "Combiner.h"
#include "unified_combiner.h"
#include "gDP.h"

CombinerInfo combiner;

void Combiner_Init()
{
    Init_unified_combiner();
    combiner.root = NULL;
}

void Combiner_UpdateCombineColors()
{
    Update_unified_combiner_Colors(combiner.current->compiled);
    gDP.changed &= ~CHANGED_COMBINE_COLORS;
}

void Combiner_SimplifyCycle(CombineCycle *cc, CombinerStage *stage)
{
    // Load the first operand
    stage->op[0].op = LOAD;
    stage->op[0].param1 = cc->sa;
    stage->numOps = 1;

    // If we're just subtracting zero, skip it
    if (cc->sb != ZERO)
    {
        // Subtracting a number from itself is zero
        if (cc->sb == stage->op[0].param1)
            stage->op[0].param1 = ZERO;
        else
        {
            stage->op[1].op = SUB;
            stage->op[1].param1 = cc->sb;
            stage->numOps++;
        }
    }

    // If we either subtracted, or didn't load a zero
    if ((stage->numOps > 1) || (stage->op[0].param1 != ZERO))
    {
        // Multiplying by zero is zero
        if (cc->m == ZERO)
        {
            stage->numOps = 1;
            stage->op[0].op = LOAD;
            stage->op[0].param1 = ZERO;
        }
        else
        {
            // Multiplying by one, so just do a load
            if ((stage->numOps == 1) && (stage->op[0].param1 == ONE))
                stage->op[0].param1 = cc->m;
            else
            {
                stage->op[stage->numOps].op = MUL;
                stage->op[stage->numOps].param1 = cc->m;
                stage->numOps++;
            }
        }
    }

    // Don't bother adding zero
    if (cc->a != ZERO)
    {
        // If all we have so far is zero, then load this instead
        if ((stage->numOps == 1) && (stage->op[0].param1 == ZERO))
            stage->op[0].param1 = cc->a;
        else
        {
            stage->op[stage->numOps].op = ADD;
            stage->op[stage->numOps].param1 = cc->a;
            stage->numOps++;
        }
    }

    // Handle interpolation
    if ((stage->numOps == 4) && (stage->op[1].param1 == stage->op[3].param1))
    {
        stage->numOps = 1;
        stage->op[0].op = INTER;
        stage->op[0].param2 = stage->op[1].param1;
        stage->op[0].param3 = stage->op[2].param1;
    }
}

void Combiner_MergeStages(Combiner *c)
{
    // If all we have is a load in the first stage we can just replace
    // each occurance of COMBINED in the second stage with it
    if ((c->stage[0].numOps == 1) && (c->stage[0].op[0].op == LOAD))
    {
        int combined = c->stage[0].op[0].param1;

        for (int i = 0; i < c->stage[1].numOps; i++)
        {
            c->stage[0].op[i].op = c->stage[1].op[i].op;
            c->stage[0].op[i].param1 = (c->stage[1].op[i].param1 == COMBINED) ? combined : c->stage[1].op[i].param1;
            c->stage[0].op[i].param2 = (c->stage[1].op[i].param2 == COMBINED) ? combined : c->stage[1].op[i].param2;
            c->stage[0].op[i].param3 = (c->stage[1].op[i].param3 == COMBINED) ? combined : c->stage[1].op[i].param3;
        }

        c->stage[0].numOps = c->stage[1].numOps;
        c->numStages = 1;
    }
    // We can't do any merging on an interpolation
    else if (c->stage[1].op[0].op != INTER)
    {
        int numCombined = 0;

        // See how many times the first stage is used in the second one
        for (int i = 0; i < c->stage[1].numOps; i++)
            if (c->stage[1].op[i].param1 == COMBINED) numCombined++;

        // If it's not used, just replace the first stage with the second
        if (numCombined == 0)
        {
            for (int i = 0; i < c->stage[1].numOps; i++)
            {
                c->stage[0].op[i].op = c->stage[1].op[i].op;
                c->stage[0].op[i].param1 = c->stage[1].op[i].param1;
                c->stage[0].op[i].param2 = c->stage[1].op[i].param2;
                c->stage[0].op[i].param3 = c->stage[1].op[i].param3;
            }
            c->stage[0].numOps = c->stage[1].numOps;

            c->numStages = 1;
        }
        // If it's only used once
        else if (numCombined == 1)
        {
            // It's only used in the load, so tack on the ops from stage 2 on stage 1
            if (c->stage[1].op[0].param1 == COMBINED)
            {
                for (int i = 1; i < c->stage[1].numOps; i++)
                {
                    c->stage[0].op[c->stage[0].numOps].op = c->stage[1].op[i].op;
                    c->stage[0].op[c->stage[0].numOps].param1 = c->stage[1].op[i].param1;
                    c->stage[0].numOps++;
                }

                c->numStages = 1;
            }
            // Otherwise, if it's used in the second op, and that op isn't SUB
            // we can switch the parameters so it works out to tack the ops onto stage 1
            else if ((c->stage[1].op[1].param1 == COMBINED) && (c->stage[1].op[1].op != SUB))
            {
                c->stage[0].op[c->stage[0].numOps].op = c->stage[1].op[1].op;
                c->stage[0].op[c->stage[0].numOps].param1 = c->stage[1].op[0].param1;
                c->stage[0].numOps++;

                // If there's another op, tack it onto stage 1 too
                if (c->stage[1].numOps > 2)
                {
                    c->stage[0].op[c->stage[0].numOps].op = c->stage[1].op[2].op;
                    c->stage[0].op[c->stage[0].numOps].param1 = c->stage[1].op[2].param1;
                    c->stage[0].numOps++;
                }

                c->numStages = 1;
            }
        }
    }
}

CachedCombiner *Combiner_Compile(u64 mux)
{
    gDPCombine combine;

    combine.mux = mux;

    int numCycles;

    Combiner color, alpha;

    if (gDP.otherMode.cycleType == G_CYC_2CYCLE)
    {
        numCycles = 2;
        color.numStages = 2;
        alpha.numStages = 2;
    }
    else
    {
        numCycles = 1;
        color.numStages = 1;
        alpha.numStages = 1;
    }

    CombineCycle cc[2];
    CombineCycle ac[2];

    // Decode and expand the combine mode into a more general form
    cc[0].sa = saRGBExpanded[combine.saRGB0];
    cc[0].sb = sbRGBExpanded[combine.sbRGB0];
    cc[0].m = mRGBExpanded[combine.mRGB0];
    cc[0].a = aRGBExpanded[combine.aRGB0];
    ac[0].sa = saAExpanded[combine.saA0];
    ac[0].sb = sbAExpanded[combine.sbA0];
    ac[0].m = mAExpanded[combine.mA0];
    ac[0].a = aAExpanded[combine.aA0];

    cc[1].sa = saRGBExpanded[combine.saRGB1];
    cc[1].sb = sbRGBExpanded[combine.sbRGB1];
    cc[1].m = mRGBExpanded[combine.mRGB1];
    cc[1].a = aRGBExpanded[combine.aRGB1];
    ac[1].sa = saAExpanded[combine.saA1];
    ac[1].sb = sbAExpanded[combine.sbA1];
    ac[1].m = mAExpanded[combine.mA1];
    ac[1].a = aAExpanded[combine.aA1];

    for (int i = 0; i < numCycles; i++)
    {
        // Simplify each RDP combiner cycle into a combiner stage
        Combiner_SimplifyCycle(&cc[i], &color.stage[i]);
        Combiner_SimplifyCycle(&ac[i], &alpha.stage[i]);
    }

    if (numCycles == 2)
    {
        // Attempt to merge the two stages into one
        Combiner_MergeStages(&color);
        Combiner_MergeStages(&alpha);
    }

    auto cached = (CachedCombiner *)malloc(sizeof(CachedCombiner));

    cached->combine.mux = combine.mux;
    cached->left = NULL;
    cached->right = NULL;

    // Send the simplified combiner to the unified hardware compiler
    cached->compiled = Compile_unified_combiner(&color, &alpha);

    return cached;
}

void Combiner_DeleteCombiner(CachedCombiner *combiner)
{
    if (combiner->left) Combiner_DeleteCombiner(combiner->left);
    if (combiner->right) Combiner_DeleteCombiner(combiner->right);

    free(combiner->compiled);
    free(combiner);
}

void Combiner_Destroy()
{
    if (combiner.root)
    {
        Combiner_DeleteCombiner(combiner.root);
        combiner.root = NULL;
    }

    Uninit_unified_combiner();

    for (int i = 0; i < OGL.maxTextureUnits; i++)
    {
        glActiveTextureARB(GL_TEXTURE0_ARB + i);
        glDisable(GL_TEXTURE_2D);
    }
}

void Combiner_BeginTextureUpdate()
{
    BeginTextureUpdate_unified_combiner();
}

void Combiner_EndTextureUpdate()
{
    EndTextureUpdate_unified_combiner();
}

void Combiner_SelectCombine(u64 mux)
{
    // Hack for the Banjo-Tooie shadow (framebuffer textures must be enabled too)
    if ((gDP.otherMode.cycleType == G_CYC_1CYCLE) && (mux == 0x00ffe7ffffcf9fcf) &&
        (cache.current[0]->frameBufferTexture))
    {
        mux = EncodeCombineMode(0, 0, 0, 0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, 0, TEXEL0, 0, PRIMITIVE, 0);
    }

    CachedCombiner *current = combiner.root;
    CachedCombiner *parent = current;

    while (current)
    {
        parent = current;

        if (mux == current->combine.mux)
            break;
        else if (mux < current->combine.mux)
            current = current->left;
        else
            current = current->right;
    }

    if (current == NULL)
    {
        current = Combiner_Compile(mux);

        if (parent == NULL)
            combiner.root = current;
        else if (parent->combine.mux > current->combine.mux)
            parent->left = current;
        else
            parent->right = current;
    }

    combiner.current = current;

    gDP.changed |= CHANGED_COMBINE_COLORS;
}

void Combiner_SetCombineStates()
{
    Set_unified_combiner(combiner.current->compiled);
}

void Combiner_SetCombine(u64 mux)
{
    Combiner_SelectCombine(mux);
    Combiner_SetCombineStates();
}
