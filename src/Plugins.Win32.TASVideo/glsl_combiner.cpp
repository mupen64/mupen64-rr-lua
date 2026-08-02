#include "Common.hpp"
#include "glsl_combiner.hpp"
#include "Combiner.hpp"
#include "OpenGL.hpp"
#include "gDP.hpp"
#include "gSP.hpp"
#include "GBI.hpp"
#include "glN64.hpp"
#include "Textures.h"

struct GLSLProgram
{
    GLuint program;

    BOOL usesT0, usesT1, usesNoise;

    GLint locTex0, locTex1;
    GLint locPrimColor, locEnvColor, locCenterColor, locScaleColor, locFogColor;
    GLint locLodFrac, locPrimLodFrac, locK4, locK5, locNoise;
    GLint locFogEnabled;
    GLint locAlphaTest, locAlphaRef;
    GLint locDitherAlpha, locDitherSeed;
    GLint locProjection;
};

struct UniformUsage
{
    bool prim, env, center, scale, lodFrac, primLodFrac, k4, k5, noise;
};

static std::vector<GLSLProgram *> s_programs;
static GLuint s_vertexShader = 0;
static GLSLProgram *s_currentProgram = nullptr;

static int s_alphaTestMode = 0;
static float s_alphaRef = 0.0f;

static float s_ditherAlpha = 0.0f;
static float s_ditherSeed = 0.0f;

static int s_fogEnabled = 0;

static float s_projection[16];

static const char *s_vertexShaderBody = "attribute vec4 aPosition;\n"
                                        "attribute vec4 aColor;\n"
                                        "attribute vec4 aSecondaryColor;\n"
                                        "attribute vec2 aTexCoord0;\n"
                                        "attribute vec2 aTexCoord1;\n"
                                        "attribute float aFog;\n"
                                        "uniform mat4 uProjection;\n"
                                        "varying lowp vec4 vColor;\n"
                                        "varying lowp vec4 vSecondaryColor;\n"
                                        "varying highp vec2 vTexCoord0;\n"
                                        "varying highp vec2 vTexCoord1;\n"
                                        "varying mediump float vFog;\n"
                                        "void main() {\n"
                                        "    gl_Position = uProjection * aPosition;\n"
                                        "    vColor = clamp(aColor, 0.0, 1.0);\n"
                                        "    vSecondaryColor = clamp(aSecondaryColor, 0.0, 1.0);\n"
                                        "    vTexCoord0 = aTexCoord0;\n"
                                        "    vTexCoord1 = aTexCoord1;\n"
                                        "    vFog = aFog;\n"
                                        "}\n";

static void LogInfoLog(const wchar_t *what, GLuint object, bool isProgram)
{
    GLint length = 0;
    if (isProgram)
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &length);
    else
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &length);

    std::string infoLog;
    if (length > 1)
    {
        infoLog.resize(length);
        if (isProgram)
            glGetProgramInfoLog(object, length, nullptr, infoLog.data());
        else
            glGetShaderInfoLog(object, length, nullptr, infoLog.data());
    }

    std::wstring message(what);
    message += L": ";
    for (const char c : infoLog)
    {
        if (c != '\0') message += (wchar_t)(unsigned char)c;
    }
    g_plugin->log_error(message.c_str());
}

static GLuint CompileShader(GLenum type, const char *source)
{
    const GLuint shader = glCreateShader(type);
    if (shader == 0)
    {
        g_plugin->log_error(L"GLSL combiner: glCreateShader failed");
        return 0;
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        LogInfoLog(type == GL_VERTEX_SHADER ? L"GLSL combiner: vertex shader compile failed"
                                            : L"GLSL combiner: fragment shader compile failed",
                   shader, false);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static std::string ColorValue(int operand, bool firstCycle, UniformUsage &usage)
{
    switch (operand)
    {
    case COMBINED:
        return firstCycle ? "vec3(0.0)" : "combined";
    case COMBINED_ALPHA:
        return firstCycle ? "vec3(0.0)" : "vec3(combinedA)";
    case TEXEL0:
        return "texture2D(uTex0, vTexCoord0).rgb";
    case TEXEL0_ALPHA:
        return "vec3(texture2D(uTex0, vTexCoord0).a)";
    case TEXEL1:
        return "texture2D(uTex1, vTexCoord1).rgb";
    case TEXEL1_ALPHA:
        return "vec3(texture2D(uTex1, vTexCoord1).a)";
    case PRIMITIVE:
        usage.prim = true;
        return "uPrimColor.rgb";
    case PRIMITIVE_ALPHA:
        usage.prim = true;
        return "vec3(uPrimColor.a)";
    case SHADE:
        return "vColor.rgb";
    case SHADE_ALPHA:
        return "vec3(vColor.a)";
    case ENVIRONMENT:
        usage.env = true;
        return "uEnvColor.rgb";
    case ENV_ALPHA:
        usage.env = true;
        return "vec3(uEnvColor.a)";
    case CENTER:
        usage.center = true;
        return "uCenterColor.rgb";
    case SCALE:
        usage.scale = true;
        return "uScaleColor.rgb";
    case LOD_FRACTION:
        usage.lodFrac = true;
        return "vec3(uLodFrac)";
    case PRIM_LOD_FRAC:
        usage.primLodFrac = true;
        return "vec3(uPrimLodFrac)";
    case NOISE:
        usage.noise = true;
        return "vec3(uNoise)";
    case K4:
        usage.k4 = true;
        return "vec3(uK4)";
    case K5:
        usage.k5 = true;
        return "vec3(uK5)";
    case ONE:
        return "vec3(1.0)";
    case ZERO:
    default:
        return "vec3(0.0)";
    }
}

static std::string AlphaValue(int operand, bool firstCycle, UniformUsage &usage)
{
    switch (operand)
    {
    case COMBINED:
    case COMBINED_ALPHA:
        return firstCycle ? "0.0" : "combinedA";
    case TEXEL0:
    case TEXEL0_ALPHA:
        return "texture2D(uTex0, vTexCoord0).a";
    case TEXEL1:
    case TEXEL1_ALPHA:
        return "texture2D(uTex1, vTexCoord1).a";
    case PRIMITIVE:
    case PRIMITIVE_ALPHA:
        usage.prim = true;
        return "uPrimColor.a";
    case SHADE:
    case SHADE_ALPHA:
        return "vColor.a";
    case ENVIRONMENT:
    case ENV_ALPHA:
        usage.env = true;
        return "uEnvColor.a";
    case CENTER:
        usage.center = true;
        return "uCenterColor.a";
    case SCALE:
        usage.scale = true;
        return "uScaleColor.a";
    case LOD_FRACTION:
        usage.lodFrac = true;
        return "uLodFrac";
    case PRIM_LOD_FRAC:
        usage.primLodFrac = true;
        return "uPrimLodFrac";
    case NOISE:
        usage.noise = true;
        return "uNoise";
    case K4:
        usage.k4 = true;
        return "uK4";
    case K5:
        usage.k5 = true;
        return "uK5";
    case ONE:
        return "1.0";
    case ZERO:
    default:
        return "0.0";
    }
}

static void EmitStage(std::string &body, CombinerStage *stage, bool isColor, bool firstCycle, UniformUsage &usage)
{
    const char *acc = isColor ? "rgb" : "a";

    for (int i = 0; i < stage->numOps; i++)
    {
        const CombinerOp *op = &stage->op[i];
        const std::string p1 =
            isColor ? ColorValue(op->param1, firstCycle, usage) : AlphaValue(op->param1, firstCycle, usage);

        switch (op->op)
        {
        case LOAD:
            body += std::string("    ") + acc + " = " + p1 + ";\n";
            break;

        case SUB:
            body += std::string("    ") + acc + " = " + acc + " - " + p1 + ";\n";
            break;

        case MUL:
            body += std::string("    ") + acc + " = " + acc + " * " + p1 + ";\n";
            break;

        case ADD:
            body += std::string("    ") + acc + " = " + acc + " + " + p1 + ";\n";
            break;

        case INTER: {
            const std::string p2 =
                isColor ? ColorValue(op->param2, firstCycle, usage) : AlphaValue(op->param2, firstCycle, usage);
            const std::string p3 =
                isColor ? ColorValue(op->param3, firstCycle, usage) : AlphaValue(op->param3, firstCycle, usage);
            body += std::string("    ") + acc + " = mix(" + p2 + ", " + p1 + ", " + p3 + ");\n";
            break;
        }
        }
    }
}

static void ScanOperand(GLSLProgram *program, int operand)
{
    if ((operand == TEXEL0) || (operand == TEXEL0_ALPHA)) program->usesT0 = TRUE;
    if ((operand == TEXEL1) || (operand == TEXEL1_ALPHA)) program->usesT1 = TRUE;
    if (operand == NOISE) program->usesNoise = TRUE;
}

static void ScanCombiner(GLSLProgram *program, Combiner *c)
{
    for (int i = 0; i < c->numStages; i++)
    {
        for (int j = 0; j < c->stage[i].numOps; j++)
        {
            ScanOperand(program, c->stage[i].op[j].param1);
            if (c->stage[i].op[j].op == INTER)
            {
                ScanOperand(program, c->stage[i].op[j].param2);
                ScanOperand(program, c->stage[i].op[j].param3);
            }
        }
    }
}

static std::string BuildFragmentSource(Combiner *color, Combiner *alpha)
{
    UniformUsage usage = {};
    const int numCycles = max(color->numStages, alpha->numStages);

    std::string body;
    body += "    vec3 rgb = vec3(0.0);\n";
    body += "    float a = 0.0;\n";
    if (numCycles > 1)
    {
        body += "    vec3 combined = vec3(0.0);\n";
        body += "    float combinedA = 0.0;\n";
    }

    for (int cycle = 0; cycle < numCycles; cycle++)
    {
        const bool firstCycle = (cycle == 0);
        if (!firstCycle)
        {
            body += "    combined = WRAP(rgb, -0.51, 1.51);\n";
            body += "    combinedA = WRAP(a, -0.51, 1.51);\n";
        }
        if (cycle < color->numStages) EmitStage(body, &color->stage[cycle], true, firstCycle, usage);
        if (cycle < alpha->numStages) EmitStage(body, &alpha->stage[cycle], false, firstCycle, usage);
    }

    body += "    vec4 result = clamp(WRAP(vec4(rgb, a), -0.51, 1.51), 0.0, 1.0);\n";
    body += "    if (uFogEnabled != 0) {\n";
    body += "        float f = clamp((255.0 - vFog) / 255.0, 0.0, 1.0);\n";
    body += "        result.rgb = mix(uFogColor.rgb, result.rgb, f);\n";
    body += "    }\n";
    body += "    if (uAlphaTest == 1) { if (result.a < uAlphaRef) discard; }\n";
    body += "    else if (uAlphaTest == 2) { if (result.a <= uAlphaRef) discard; }\n";
    // G_AC_DITHER: screen-door transparency. Replaces the fixed-function polygon stipple, which had
    // one of 32 random 32x32 patterns picked by the env alpha and rotated every draw; here a
    // per-pixel hash is compared against the same alpha, and the seed rotates per draw the same way.
    body += "    else if (uAlphaTest == 3) { if (ditherNoise() >= uDitherAlpha) discard; }\n";
    body += "    gl_FragColor = result;\n";

    std::string source;
    if (OGL.isGLES)
    {
        source += "#version 100\n";
        source += "#ifdef GL_FRAGMENT_PRECISION_HIGH\n";
        source += "precision highp float;\n";
        source += "#else\n";
        source += "precision mediump float;\n";
        source += "#define highp mediump\n";
        source += "#endif\n";
    }
    else
    {
        source += "#version 120\n";
        source += "#define lowp\n";
        source += "#define mediump\n";
        source += "#define highp\n";
    }
    source += "#define WRAP(x, lo, hi) (mod((x) - (lo), (hi) - (lo)) + (lo))\n";
    source += "varying lowp vec4 vColor;\n";
    source += "varying highp vec2 vTexCoord0;\n";
    source += "varying highp vec2 vTexCoord1;\n";
    source += "varying mediump float vFog;\n";
    source += "uniform sampler2D uTex0;\n";
    source += "uniform sampler2D uTex1;\n";
    if (usage.prim) source += "uniform vec4 uPrimColor;\n";
    if (usage.env) source += "uniform vec4 uEnvColor;\n";
    if (usage.center) source += "uniform vec4 uCenterColor;\n";
    if (usage.scale) source += "uniform vec4 uScaleColor;\n";
    source += "uniform vec4 uFogColor;\n";
    if (usage.lodFrac) source += "uniform float uLodFrac;\n";
    if (usage.primLodFrac) source += "uniform float uPrimLodFrac;\n";
    if (usage.k4) source += "uniform float uK4;\n";
    if (usage.k5) source += "uniform float uK5;\n";
    if (usage.noise) source += "uniform float uNoise;\n";
    source += "uniform int uFogEnabled;\n";
    source += "uniform int uAlphaTest;\n";
    source += "uniform float uAlphaRef;\n";
    source += "uniform float uDitherAlpha;\n";
    source += "uniform float uDitherSeed;\n";
    source += "lowp float ditherNoise() {\n";
    source += "    highp vec3 p3 = fract(vec3(floor(gl_FragCoord.xy), uDitherSeed) * 0.1031);\n";
    source += "    p3 += dot(p3, p3.zyx + 31.32);\n";
    source += "    return fract((p3.x + p3.y) * p3.z);\n";
    source += "}\n";
    source += "void main() {\n";
    source += body;
    source += "}\n";

    return source;
}

static void CacheUniformLocations(GLSLProgram *program)
{
    program->locTex0 = glGetUniformLocation(program->program, "uTex0");
    program->locTex1 = glGetUniformLocation(program->program, "uTex1");
    program->locPrimColor = glGetUniformLocation(program->program, "uPrimColor");
    program->locEnvColor = glGetUniformLocation(program->program, "uEnvColor");
    program->locCenterColor = glGetUniformLocation(program->program, "uCenterColor");
    program->locScaleColor = glGetUniformLocation(program->program, "uScaleColor");
    program->locFogColor = glGetUniformLocation(program->program, "uFogColor");
    program->locLodFrac = glGetUniformLocation(program->program, "uLodFrac");
    program->locPrimLodFrac = glGetUniformLocation(program->program, "uPrimLodFrac");
    program->locK4 = glGetUniformLocation(program->program, "uK4");
    program->locK5 = glGetUniformLocation(program->program, "uK5");
    program->locNoise = glGetUniformLocation(program->program, "uNoise");
    program->locFogEnabled = glGetUniformLocation(program->program, "uFogEnabled");
    program->locAlphaTest = glGetUniformLocation(program->program, "uAlphaTest");
    program->locAlphaRef = glGetUniformLocation(program->program, "uAlphaRef");
    program->locDitherAlpha = glGetUniformLocation(program->program, "uDitherAlpha");
    program->locDitherSeed = glGetUniformLocation(program->program, "uDitherSeed");
    program->locProjection = glGetUniformLocation(program->program, "uProjection");
}

static GLuint GetVertexShader()
{
    if (s_vertexShader == 0)
    {
        std::string source;
        if (OGL.isGLES)
        {
            source = "#version 100\n";
        }
        else
        {
            source = "#version 120\n";
            source += "#define lowp\n#define mediump\n#define highp\n";
        }
        source += s_vertexShaderBody;
        s_vertexShader = CompileShader(GL_VERTEX_SHADER, source.c_str());
    }
    return s_vertexShader;
}

void GLSLCombiner_Init()
{
    for (int i = 0; i < 16; i++) s_projection[i] = 0.0f;
    s_projection[0] = s_projection[5] = s_projection[10] = s_projection[15] = 1.0f;

    if (!GLEW_VERSION_2_0)
    {
        g_plugin->log_error(L"GLSL combiner: OpenGL 2.0 not available, shaders will fail to compile");
        return;
    }

    GetVertexShader();
}

void GLSLCombiner_Uninit()
{
    glUseProgram(0);

    for (GLSLProgram *program : s_programs)
    {
        if (program->program != 0) glDeleteProgram(program->program);
        delete program;
    }
    s_programs.clear();

    if (s_vertexShader != 0)
    {
        glDeleteShader(s_vertexShader);
        s_vertexShader = 0;
    }

    s_currentProgram = nullptr;
}

GLSLProgram *GLSLCombiner_Compile(Combiner *color, Combiner *alpha)
{
    auto program = new GLSLProgram();
    program->program = 0;
    program->usesT0 = FALSE;
    program->usesT1 = FALSE;
    program->usesNoise = FALSE;
    s_programs.push_back(program);

    ScanCombiner(program, color);
    ScanCombiner(program, alpha);

    const GLuint vertexShader = GetVertexShader();
    if (vertexShader == 0)
    {
        CacheUniformLocations(program);
        return program;
    }

    const std::string fragmentSource = BuildFragmentSource(color, alpha);
    const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str());
    if (fragmentShader == 0)
    {
        CacheUniformLocations(program);
        return program;
    }

    const GLuint glProgram = glCreateProgram();
    glAttachShader(glProgram, vertexShader);
    glAttachShader(glProgram, fragmentShader);

    glBindAttribLocation(glProgram, 0, "aPosition");
    glBindAttribLocation(glProgram, 1, "aColor");
    glBindAttribLocation(glProgram, 2, "aSecondaryColor");
    glBindAttribLocation(glProgram, 3, "aTexCoord0");
    glBindAttribLocation(glProgram, 4, "aTexCoord1");
    glBindAttribLocation(glProgram, 5, "aFog");

    glLinkProgram(glProgram);
    glDeleteShader(fragmentShader);

    GLint status = GL_FALSE;
    glGetProgramiv(glProgram, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        LogInfoLog(L"GLSL combiner: program link failed", glProgram, true);
        glDeleteProgram(glProgram);
        CacheUniformLocations(program);
        return program;
    }

    program->program = glProgram;
    CacheUniformLocations(program);
    return program;
}

void GLSLCombiner_Set(GLSLProgram *program)
{
    s_currentProgram = program;

    glUseProgram(program->program);
    glUniform1i(program->locTex0, 0);
    glUniform1i(program->locTex1, 1);

    glUniform1i(program->locAlphaTest, s_alphaTestMode);
    glUniform1f(program->locAlphaRef, s_alphaRef);
    glUniform1f(program->locDitherAlpha, s_ditherAlpha);
    glUniform1f(program->locDitherSeed, s_ditherSeed);
    glUniform1i(program->locFogEnabled, s_fogEnabled);
    glUniformMatrix4fv(program->locProjection, 1, GL_FALSE, s_projection);

    combiner.usesT0 = program->usesT0;
    combiner.usesT1 = program->usesT1;
    combiner.usesNoise = program->usesNoise;

    combiner.vertex.color = COMBINED;
    combiner.vertex.secondaryColor = COMBINED;
    combiner.vertex.alpha = COMBINED;
}

void GLSLCombiner_UpdateColors(GLSLProgram *program)
{
    glUseProgram(program->program);

    glUniform4f(program->locPrimColor, gDP.primColor.r, gDP.primColor.g, gDP.primColor.b, gDP.primColor.a);
    glUniform4f(program->locEnvColor, gDP.envColor.r, gDP.envColor.g, gDP.envColor.b, gDP.envColor.a);
    glUniform4f(program->locCenterColor, gDP.key.center.r, gDP.key.center.g, gDP.key.center.b, gDP.key.center.a);
    glUniform4f(program->locScaleColor, gDP.key.scale.r, gDP.key.scale.g, gDP.key.scale.b, gDP.key.scale.a);
    glUniform4f(program->locFogColor, gDP.fogColor.r, gDP.fogColor.g, gDP.fogColor.b, gDP.fogColor.a);

    glUniform1f(program->locLodFrac, gDP.primColor.l);
    glUniform1f(program->locPrimLodFrac, gDP.primColor.l);

    glUniform1f(program->locK4, gDP.convert.k4 / 255.0f);
    glUniform1f(program->locK5, gDP.convert.k5 / 255.0f);

    glUniform1f(program->locNoise, 0.5f);
}

void GLSLCombiner_SetFogEnabled(bool enabled)
{
    s_fogEnabled = enabled ? 1 : 0;

    if (s_currentProgram != nullptr && s_currentProgram->program != 0)
    {
        glUseProgram(s_currentProgram->program);
        glUniform1i(s_currentProgram->locFogEnabled, s_fogEnabled);
    }
}

void GLSLCombiner_SetAlphaTest(int mode, float ref)
{
    s_alphaTestMode = mode;
    s_alphaRef = ref;

    if (s_currentProgram != nullptr && s_currentProgram->program != 0)
    {
        glUseProgram(s_currentProgram->program);
        glUniform1i(s_currentProgram->locAlphaTest, s_alphaTestMode);
        glUniform1f(s_currentProgram->locAlphaRef, s_alphaRef);
    }
}

void GLSLCombiner_UpdateDither(float alpha)
{
    s_ditherAlpha = alpha;
    // Rotate through 8 patterns like the fixed-function path did, so the grain animates.
    s_ditherSeed = (float)(((int)s_ditherSeed + 1) & 0x7);

    if (s_currentProgram != nullptr && s_currentProgram->program != 0)
    {
        glUseProgram(s_currentProgram->program);
        glUniform1f(s_currentProgram->locDitherAlpha, s_ditherAlpha);
        glUniform1f(s_currentProgram->locDitherSeed, s_ditherSeed);
    }
}

void GLSLCombiner_SetProjection(const float *matrix)
{
    for (int i = 0; i < 16; i++) s_projection[i] = matrix[i];

    if (s_currentProgram != nullptr && s_currentProgram->program != 0)
    {
        glUseProgram(s_currentProgram->program);
        glUniformMatrix4fv(s_currentProgram->locProjection, 1, GL_FALSE, s_projection);
    }
}

void GLSLCombiner_BeginTextureUpdate()
{
    glUseProgram(0);
}

void GLSLCombiner_EndTextureUpdate()
{
    if (s_currentProgram != nullptr) glUseProgram(s_currentProgram->program);
}
