#pragma once

struct Combiner;
struct GLSLProgram;

void GLSLCombiner_Init();
void GLSLCombiner_Uninit();
GLSLProgram *GLSLCombiner_Compile(Combiner *color, Combiner *alpha);
void GLSLCombiner_Set(GLSLProgram *program);
void GLSLCombiner_UpdateColors(GLSLProgram *program);
void GLSLCombiner_SetAlphaTest(int mode, float ref);
void GLSLCombiner_SetFogEnabled(bool enabled);
void GLSLCombiner_SetProjection(const float *matrix);
void GLSLCombiner_BeginTextureUpdate();
void GLSLCombiner_EndTextureUpdate();
