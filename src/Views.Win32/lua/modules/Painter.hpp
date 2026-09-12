/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.hpp>
#include <Common/Assert.hpp>
#include <lua/LuaManager.hpp>
#include <lua/LuaRenderer.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <dwrite_1.h>
#include <format>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

inline bool operator==(const D2D1_COLOR_F &a, const D2D1_COLOR_F &b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

namespace LuaCore::Painter
{
namespace Detail
{
constexpr const char *IMAGE_MT = "mupen64.PainterImage";
constexpr const char *PAINTER_MT = "mupen64.Painter";
constexpr float MAX_LAYOUT_SIZE = 10000000.0f;
constexpr size_t TEXT_LAYOUT_CACHE_CAPACITY = 2048;
constexpr std::uint64_t TEXT_LAYOUT_CACHE_MAX_UNUSED_GENERATIONS = 120;
constexpr size_t TEXT_MEASUREMENT_CACHE_CAPACITY = 2048;

struct Image
{
    ComPtr<ID2D1BitmapRenderTarget> target;
    ComPtr<ID2D1Bitmap> bitmap;
    UINT width{};
    UINT height{};
    bool closed{};
    bool painting{};
};

struct TextStyle
{
    std::wstring family{L"Segoe UI"};
    float size{12.0f};
    DWRITE_FONT_WEIGHT weight{DWRITE_FONT_WEIGHT_NORMAL};
    DWRITE_FONT_STYLE slant{DWRITE_FONT_STYLE_NORMAL};
    bool underline{};
    bool strikethrough{};
    float letter_spacing{};
    float line_height{};
    bool has_line_height{};
};

inline bool operator==(const TextStyle &a, const TextStyle &b)
{
    return a.family == b.family && a.size == b.size && a.weight == b.weight && a.slant == b.slant &&
           a.underline == b.underline && a.strikethrough == b.strikethrough && a.letter_spacing == b.letter_spacing &&
           a.line_height == b.line_height && a.has_line_height == b.has_line_height;
}

struct Stroke
{
    float width{1.0f};
    D2D1_STROKE_STYLE_PROPERTIES properties{D2D1::StrokeStyleProperties()};
    std::vector<float> dashes;
    bool specified{};
};

inline bool operator==(const Stroke &a, const Stroke &b)
{
    if (a.specified != b.specified || a.width != b.width || a.dashes != b.dashes) return false;
    if (!a.specified) return true;
    const auto &p = a.properties;
    const auto &q = b.properties;
    return p.startCap == q.startCap && p.endCap == q.endCap && p.dashCap == q.dashCap && p.lineJoin == q.lineJoin &&
           p.miterLimit == q.miterLimit && p.dashStyle == q.dashStyle && p.dashOffset == q.dashOffset;
}

struct BrushResource
{
    D2D1_COLOR_F color{};
    ComPtr<ID2D1SolidColorBrush> native;
};

struct StrokeResource
{
    Stroke value{};
    ComPtr<ID2D1StrokeStyle> native;
};

struct TextFormatResource
{
    TextStyle style{};
    DWRITE_TEXT_ALIGNMENT alignment{DWRITE_TEXT_ALIGNMENT_LEADING};
    DWRITE_PARAGRAPH_ALIGNMENT paragraph_alignment{DWRITE_PARAGRAPH_ALIGNMENT_NEAR};
    DWRITE_WORD_WRAPPING wrapping{DWRITE_WORD_WRAPPING_WRAP};
    ComPtr<IDWriteTextFormat> native;
};

struct ImageResource
{
    ComPtr<ID2D1Bitmap> bitmap;
    UINT width{};
    UINT height{};
};

struct ImageSlice
{
    D2D1_RECT_F source{};
    D2D1_RECT_F destination{};
};

struct PathOp
{
    enum class Kind : std::uint8_t
    {
        Move,
        Line,
        Cubic,
        Quadratic,
        Arc,
        Close,
    };

    struct CurveData
    {
        D2D1_POINT_2F point1;
        D2D1_POINT_2F point2;
    };
    struct ArcData
    {
        float radius;
        float start_angle;
        float end_angle;
        bool ccw;
    };

    Kind kind{};
    D2D1_POINT_2F point0{};
    union {
        CurveData curve;
        ArcData arc;
    };

    PathOp() : kind(Kind::Move), point0{}, curve{} {}
};

struct TextRun
{
    std::wstring text;
    D2D1_RECT_F rect{};
    UINT32 format{};
    bool ellipsis{};
    bool fit{};
    D2D1_DRAW_TEXT_OPTIONS options{D2D1_DRAW_TEXT_OPTIONS_NONE};
};

struct PathPayload
{
    std::vector<PathOp> ops;
    std::vector<TextRun> texts;
};

constexpr std::uint8_t MAX_IMAGE_SLICES = 9;

struct ImagePayload
{
    std::array<ImageSlice, MAX_IMAGE_SLICES> slices{};
    std::uint8_t slice_count{};
    D2D1_COLOR_F tint{D2D1::ColorF(1, 1, 1, 1)};
    float opacity{1.0f};
    D2D1_BITMAP_INTERPOLATION_MODE interpolation{D2D1_BITMAP_INTERPOLATION_MODE_LINEAR};
    bool tinted{};
};

enum class CommandType : std::uint8_t
{
    Clear,
    PushClip,
    PopClip,
    FillPath,
    StrokePath,
    Image,
};

struct Command
{
    struct ClearData
    {
        D2D1_COLOR_F color;
    };
    struct ClipData
    {
        D2D1_RECT_F bounds;
    };
    struct PathData
    {
        D2D1_MATRIX_3X2_F transform;
        D2D1_COLOR_F color;
        UINT32 brush;
        UINT32 stroke;
        UINT32 path;
    };
    struct ImageData
    {
        D2D1_MATRIX_3X2_F transform;
        UINT32 resource;
        UINT32 payload;
    };

    CommandType type{};
    union {
        ClearData clear;
        ClipData clip;
        PathData path;
        ImageData image;
    };

    Command() : type(CommandType::Clear), clear{} {}
};

inline D2D1::Matrix3x2F to_matrix(const D2D1_MATRIX_3X2_F &matrix)
{
    return D2D1::Matrix3x2F(matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32);
}

struct StateSnapshot
{
    D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Identity();
    size_t clip_depth{};
};

struct Painter
{
    ID2D1RenderTarget *target{};
    LuaRenderingContext *context{};
    bool active{};

    D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Identity();
    std::vector<StateSnapshot> states;

    std::vector<D2D1_RECT_F> clips;

    std::vector<PathOp> path_ops;
    std::vector<TextRun> path_texts;

    std::vector<Command> commands;
    std::vector<PathPayload> paths;
    std::vector<ImagePayload> image_payloads;
    std::vector<BrushResource> brushes;
    std::vector<StrokeResource> strokes;
    std::vector<TextFormatResource> text_formats;
    std::vector<ImageResource> images;
};

inline std::string hresult_message(const char *operation, HRESULT hr)
{
    char *system_message = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
        static_cast<DWORD>(hr), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char *>(&system_message), 0,
        nullptr);
    if (system_message)
    {
        std::string_view detail(system_message);
        while (!detail.empty() && (detail.back() == '\r' || detail.back() == '\n')) detail.remove_suffix(1);
        LocalFree(system_message);
        return std::format("{} failed (HRESULT 0x{:08X}): {}", operation, static_cast<unsigned long>(hr), detail);
    }
    return std::format("{} failed (HRESULT 0x{:08X})", operation, static_cast<unsigned long>(hr));
}

inline int fail_hr(lua_State *L, const char *operation, HRESULT hr)
{
    const auto message = hresult_message(operation, hr);
    return luaL_error(L, "%s", message.c_str());
}

inline D2D1_COLOR_F parse_hex_color(lua_State *L, std::string_view hex)
{
    auto parse_hex_digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    if (!hex.empty() && hex.front() == '#') hex.remove_prefix(1);
    if (hex.size() != 6 && hex.size() != 8) luaL_error(L, "color string must be \"#RRGGBB\" or \"#RRGGBBAA\"");
    int values[4] = {0, 0, 0, 255};
    for (size_t i = 0; i < hex.size(); i += 2)
    {
        const int high = parse_hex_digit(hex[i]);
        const int low = i + 1 < hex.size() ? parse_hex_digit(hex[i + 1]) : high;
        if (high < 0 || low < 0) luaL_error(L, "color string contains an invalid hex digit");
        values[i / 2] = high << 4 | low;
    }
    constexpr float scale = 1.0f / 255.0f;
    return D2D1::ColorF(values[0] * scale, values[1] * scale, values[2] * scale, values[3] * scale);
}

inline D2D1_COLOR_F check_color(lua_State *L, int index)
{
    if (lua_type(L, index) == LUA_TSTRING)
    {
        size_t length{};
        const char *hex = lua_tolstring(L, index, &length);
        return parse_hex_color(L, {hex, length});
    }
    luaL_checktype(L, index, LUA_TTABLE);
    const float r = luaL_tablenumber(L, index, "r", 0, true);
    const float g = luaL_tablenumber(L, index, "g", 0, true);
    const float b = luaL_tablenumber(L, index, "b", 0, true);
    const float a = luaL_tablenumber(L, index, "a", 1);
    if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1 || a < 0 || a > 1)
        luaL_error(L, "color components must be in the range [0, 1]");
    return D2D1::ColorF(r, g, b, a);
}

inline float check_coordinate(lua_State *L, int index, const char *name)
{
    const float value = luaL_checkfinitenumber(L, index, name);
    if (std::fabs(value) > MAX_LAYOUT_SIZE) luaL_error(L, "%s is out of range", name);
    return value;
}

inline void validate_transform(lua_State *L, const D2D1::Matrix3x2F &transform)
{
    const float values[] = {transform._11, transform._12, transform._21, transform._22, transform._31, transform._32};
    for (const float value : values)
        if (!std::isfinite(value) || std::fabs(value) > MAX_LAYOUT_SIZE) luaL_error(L, "transform is out of range");
}

inline D2D1_RECT_F check_rect(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    const float x = luaL_tablenumber(L, index, "x", 0, true);
    const float y = luaL_tablenumber(L, index, "y", 0, true);
    const float width = luaL_tablenumber(L, index, "w", 0, true);
    const float height = luaL_tablenumber(L, index, "h", 0, true);
    const float right = x + width;
    const float bottom = y + height;
    if (width < 0 || height < 0) luaL_error(L, "rectangle width and height must be non-negative");
    if (!std::isfinite(right) || !std::isfinite(bottom) || std::fabs(x) > MAX_LAYOUT_SIZE ||
        std::fabs(y) > MAX_LAYOUT_SIZE || std::fabs(right) > MAX_LAYOUT_SIZE || std::fabs(bottom) > MAX_LAYOUT_SIZE)
        luaL_error(L, "rectangle coordinates are out of range");
    return D2D1::RectF(x, y, right, bottom);
}

inline LuaRenderingContext *check_context(lua_State *L)
{
    auto *environment = LuaManager::get_environment_for_state(L);
    if (!environment) luaL_error(L, "painter is unavailable outside a Lua environment");
    LuaRenderer::ensure_d2d_renderer_created(&environment->rctx);
    auto *context = &environment->rctx;
    if (!context->presenter || context->d2d_render_target_stack.empty() || !context->d2d_render_target_stack.top())
        luaL_error(L, "Direct2D renderer is unavailable");
    return context;
}

inline ID2D1RenderTarget *check_current_target(lua_State *L)
{
    return check_context(L)->d2d_render_target_stack.top();
}

inline Image *check_image(lua_State *L, int index)
{
    auto *image = static_cast<Image *>(luaL_checkudata(L, index, IMAGE_MT));
    if (image->closed || !image->target || !image->bitmap) luaL_error(L, "attempt to use a closed PainterImage");
    return image;
}

inline Painter *check_painter(lua_State *L, int index)
{
    auto *painter = static_cast<Painter *>(luaL_checkudata(L, index, PAINTER_MT));
    if (!painter->active || !painter->target || !painter->context)
        luaL_error(L, "Painter is no longer valid (painters are callback-scoped)");
    if (painter->context->d2d_render_target_stack.empty() ||
        painter->context->d2d_render_target_stack.top() != painter->target)
        luaL_error(L, "Painter target is no longer active");
    return painter;
}

inline UINT32 intern_brush(Painter *painter, const D2D1_COLOR_F &color)
{
    for (UINT32 i = 0; i < painter->brushes.size(); ++i)
        if (painter->brushes[i].color == color) return i;
    painter->brushes.push_back({color, nullptr});
    return static_cast<UINT32>(painter->brushes.size() - 1);
}

inline void close_image(Image *image)
{
    if (image->closed) return;
    image->target.Reset();
    image->bitmap.Reset();
    image->closed = true;
}

inline Painter *push_painter(lua_State *L, LuaRenderingContext *context, ID2D1RenderTarget *target)
{
    auto *painter = new (lua_newuserdata(L, sizeof(Painter))) Painter{};
    painter->target = target;
    painter->context = context;
    painter->active = true;
    luaL_getmetatable(L, PAINTER_MT);
    lua_setmetatable(L, -2);
    return painter;
}

inline void discard_commands(Painter *painter)
{
    if (!painter) return;
    painter->commands.clear();
    painter->paths.clear();
    painter->image_payloads.clear();
    painter->brushes.clear();
    painter->strokes.clear();
    painter->text_formats.clear();
    painter->images.clear();
    painter->path_ops.clear();
    painter->path_texts.clear();
    painter->states.clear();
    painter->clips.clear();
}

inline void invalidate_painter(Painter *painter)
{
    if (!painter) return;
    discard_commands(painter);
    painter->active = false;
    painter->target = nullptr;
    painter->context = nullptr;
}

inline D2D1_CAP_STYLE parse_cap(lua_State *L, const std::string &value)
{
    if (value == "butt") return D2D1_CAP_STYLE_FLAT;
    if (value == "round") return D2D1_CAP_STYLE_ROUND;
    if (value == "square") return D2D1_CAP_STYLE_SQUARE;
    luaL_error(L, "invalid stroke cap '%s'", value.c_str());
    return D2D1_CAP_STYLE_FLAT;
}

inline D2D1_LINE_JOIN parse_join(lua_State *L, const std::string &value)
{
    if (value == "miter") return D2D1_LINE_JOIN_MITER;
    if (value == "round") return D2D1_LINE_JOIN_ROUND;
    if (value == "bevel") return D2D1_LINE_JOIN_BEVEL;
    luaL_error(L, "invalid stroke join '%s'", value.c_str());
    return D2D1_LINE_JOIN_MITER;
}

inline Stroke check_stroke(lua_State *L, int index)
{
    Stroke result{};
    if (lua_isnoneornil(L, index)) return result;
    luaL_checktype(L, index, LUA_TTABLE);
    result.specified = true;
    result.width = luaL_tablenumber(L, index, "width", 1);
    if (!(result.width > 0) || result.width > MAX_LAYOUT_SIZE) luaL_error(L, "stroke width is out of range");

    const auto cap = parse_cap(L, luaL_tablestring(L, index, "cap", "butt"));
    const auto join = parse_join(L, luaL_tablestring(L, index, "join", "miter"));
    const float miter = luaL_tablenumber(L, index, "miter_limit", 4);
    const float offset = luaL_tablenumber(L, index, "dash_offset", 0);
    if (!(miter > 0) || miter > MAX_LAYOUT_SIZE || std::fabs(offset) > MAX_LAYOUT_SIZE)
        luaL_error(L, "stroke limits are out of range");

    const int absolute = lua_absindex(L, index);
    lua_getfield(L, absolute, "dashes");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        const size_t count = lua_rawlen(L, -1);
        result.dashes.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));
            const float dash = luaL_checkfinitenumber(L, -1, "dash length");
            if (!(dash > 0)) luaL_error(L, "dash lengths must be greater than zero");
            result.dashes.push_back(dash / result.width);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    result.properties = D2D1::StrokeStyleProperties(cap, cap, cap, join, miter,
        result.dashes.empty() ? D2D1_DASH_STYLE_SOLID : D2D1_DASH_STYLE_CUSTOM, offset / result.width);
    return result;
}

inline UINT32 intern_stroke(Painter *painter, Stroke stroke)
{
    for (UINT32 i = 0; i < painter->strokes.size(); ++i)
        if (painter->strokes[i].value == stroke) return i;
    painter->strokes.push_back({std::move(stroke), nullptr});
    return static_cast<UINT32>(painter->strokes.size() - 1);
}

inline TextStyle check_text_style(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    const int absolute = lua_absindex(L, index);
    TextStyle style{};

    lua_getfield(L, absolute, "family");
    if (lua_isstring(L, -1))
    {
        style.family = luaL_checkstlwstring(L, -1);
    }
    else if (lua_istable(L, -1))
    {
        const size_t count = lua_rawlen(L, -1);
        for (size_t i = 0; i < count; ++i)
        {
            lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));
            if (!lua_isstring(L, -1)) luaL_error(L, "font family list entries must be strings");
            const auto candidate = luaL_checkstlwstring(L, -1);
            if (!candidate.empty() && style.family == L"Segoe UI") style.family = candidate;
            lua_pop(L, 1);
        }
    }
    else if (!lua_isnil(L, -1))
    {
        luaL_error(L, "font family must be a string or an array of strings");
    }
    lua_pop(L, 1);
    if (style.family.empty()) style.family = L"Segoe UI";

    style.size = luaL_tablenumber(L, absolute, "size", 12);
    if (!(style.size > 0) || style.size > MAX_LAYOUT_SIZE)
        luaL_error(L, "font size must be greater than zero and no larger than %.0f", MAX_LAYOUT_SIZE);
    const float weight = luaL_tablenumber(L, absolute, "weight", 400);
    if (weight < 1 || weight > 1000 || std::floor(weight) != weight)
        luaL_error(L, "font weight must be an integer from 1 through 1000");
    style.weight = static_cast<DWRITE_FONT_WEIGHT>(static_cast<int>(weight));
    const auto slant = luaL_tablestring(L, absolute, "slant", "normal");
    if (slant == "normal")
        style.slant = DWRITE_FONT_STYLE_NORMAL;
    else if (slant == "italic")
        style.slant = DWRITE_FONT_STYLE_ITALIC;
    else if (slant == "oblique")
        style.slant = DWRITE_FONT_STYLE_OBLIQUE;
    else
        luaL_error(L, "invalid font slant '%s'", slant.c_str());
    style.underline = luaL_tablebool(L, absolute, "underline", false);
    style.strikethrough = luaL_tablebool(L, absolute, "strikethrough", false);
    style.letter_spacing = luaL_tablenumber(L, absolute, "letter_spacing", 0);
    if (std::fabs(style.letter_spacing) > MAX_LAYOUT_SIZE) luaL_error(L, "letter_spacing is out of range");
    lua_getfield(L, absolute, "line_height");
    if (!lua_isnil(L, -1))
    {
        style.line_height = luaL_checkfinitenumber(L, -1, "line_height");
        if (!(style.line_height > 0) || style.line_height > MAX_LAYOUT_SIZE ||
            !std::isfinite(style.size * style.line_height) || style.size * style.line_height > MAX_LAYOUT_SIZE)
            luaL_error(L, "line_height is out of range");
        style.has_line_height = true;
    }
    lua_pop(L, 1);
    return style;
}

inline UINT32 intern_text_format(Painter *painter, const TextStyle &style, DWRITE_TEXT_ALIGNMENT alignment,
    DWRITE_PARAGRAPH_ALIGNMENT paragraph_alignment, DWRITE_WORD_WRAPPING wrapping)
{
    for (UINT32 i = 0; i < painter->text_formats.size(); ++i)
    {
        const auto &candidate = painter->text_formats[i];
        if (candidate.alignment == alignment && candidate.paragraph_alignment == paragraph_alignment &&
            candidate.wrapping == wrapping && candidate.style == style)
            return i;
    }
    TextFormatResource resource{};
    resource.style = style;
    resource.alignment = alignment;
    resource.paragraph_alignment = paragraph_alignment;
    resource.wrapping = wrapping;
    painter->text_formats.push_back(std::move(resource));
    return static_cast<UINT32>(painter->text_formats.size() - 1);
}

inline UINT32 intern_image(Painter *painter, const Image &image)
{
    for (UINT32 i = 0; i < painter->images.size(); ++i)
        if (painter->images[i].bitmap.Get() == image.bitmap.Get()) return i;
    painter->images.push_back({image.bitmap, image.width, image.height});
    return static_cast<UINT32>(painter->images.size() - 1);
}

inline std::vector<D2D1_POINT_2F> check_points(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    const size_t count = lua_rawlen(L, index);
    if (count < 4 || (count & 1) != 0) luaL_error(L, "points must contain at least two x/y pairs");

    std::vector<D2D1_POINT_2F> points;
    points.reserve(count / 2);
    index = lua_absindex(L, index);
    for (size_t i = 0; i < count; i += 2)
    {
        lua_rawgeti(L, index, static_cast<lua_Integer>(i + 1));
        const float x = luaL_checkfinitenumber(L, -1, "point x");
        lua_pop(L, 1);
        lua_rawgeti(L, index, static_cast<lua_Integer>(i + 2));
        const float y = luaL_checkfinitenumber(L, -1, "point y");
        lua_pop(L, 1);
        if (std::fabs(x) > MAX_LAYOUT_SIZE || std::fabs(y) > MAX_LAYOUT_SIZE)
            luaL_error(L, "point coordinates are out of range");
        points.push_back(D2D1::Point2F(x, y));
    }
    return points;
}

inline DWRITE_WORD_WRAPPING parse_wrap(lua_State *L, const std::string &wrap)
{
    if (wrap == "none") return DWRITE_WORD_WRAPPING_NO_WRAP;
    if (wrap == "word") return DWRITE_WORD_WRAPPING_WRAP;
    if (wrap == "character") return DWRITE_WORD_WRAPPING_CHARACTER;
    luaL_error(L, "invalid text wrap mode '%s'", wrap.c_str());
    return DWRITE_WORD_WRAPPING_WRAP;
}

struct TextLayoutCacheKey
{
    std::wstring text;
    TextStyle style{};
    DWRITE_TEXT_ALIGNMENT alignment{};
    DWRITE_PARAGRAPH_ALIGNMENT paragraph_alignment{};
    DWRITE_WORD_WRAPPING wrapping{};
    float width{};
    float height{};
    bool ellipsis{};
};

class TextLayoutCache
{
  public:
    ~TextLayoutCache() { clear(); }

    void begin_generation()
    {
        if (m_generation == std::numeric_limits<std::uint64_t>::max())
        {
            m_generation = 1;
            for (auto &entry : m_lru) entry.generation = 0;
        }
        else
        {
            ++m_generation;
        }

        while (!m_lru.empty() && m_generation - m_lru.back().generation > TEXT_LAYOUT_CACHE_MAX_UNUSED_GENERATIONS)
            evict(std::prev(m_lru.end()));
    }

    IDWriteTextLayout *get(
        const std::wstring &text, const TextFormatResource &format, float width, float height, bool ellipsis)
    {
        const size_t hash = hash_key(text, format, width, height, ellipsis);
        const auto [first, last] = m_index.equal_range(hash);
        for (auto candidate = first; candidate != last; ++candidate)
        {
            const auto entry = candidate->second;
            if (!equal_key(entry->key, text, format, width, height, ellipsis)) continue;
            entry->generation = m_generation;
            m_lru.splice(m_lru.begin(), m_lru, entry);
            return entry->layout.Get();
        }
        return nullptr;
    }

    void add(const std::wstring &text, const TextFormatResource &format, float width, float height, bool ellipsis,
        ComPtr<IDWriteTextLayout> layout)
    {
        const size_t hash = hash_key(text, format, width, height, ellipsis);
        TextLayoutCacheKey key{
            text, format.style, format.alignment, format.paragraph_alignment, format.wrapping, width, height, ellipsis};
        m_lru.push_front({std::move(key), std::move(layout), m_generation, hash});
        m_index.emplace(hash, m_lru.begin());
        while (m_lru.size() > TEXT_LAYOUT_CACHE_CAPACITY) evict(std::prev(m_lru.end()));
    }

    void clear()
    {
        m_index.clear();
        m_lru.clear();
        m_generation = 0;
    }

  private:
    struct Entry
    {
        TextLayoutCacheKey key{};
        ComPtr<IDWriteTextLayout> layout;
        std::uint64_t generation{};
        size_t hash{};
    };

    template <typename T> static void hash_combine(size_t &seed, const T &value)
    {
        seed ^= std::hash<T>{}(value) + static_cast<size_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
    }

    static size_t hash_key(
        const std::wstring &text, const TextFormatResource &format, float width, float height, bool ellipsis)
    {
        size_t hash = std::hash<std::wstring>{}(text);
        hash_combine(hash, format.style.family);
        hash_combine(hash, format.style.size);
        hash_combine(hash, static_cast<int>(format.style.weight));
        hash_combine(hash, static_cast<int>(format.style.slant));
        hash_combine(hash, format.style.underline);
        hash_combine(hash, format.style.strikethrough);
        hash_combine(hash, format.style.letter_spacing);
        hash_combine(hash, format.style.line_height);
        hash_combine(hash, format.style.has_line_height);
        hash_combine(hash, static_cast<int>(format.alignment));
        hash_combine(hash, static_cast<int>(format.paragraph_alignment));
        hash_combine(hash, static_cast<int>(format.wrapping));
        hash_combine(hash, width);
        hash_combine(hash, height);
        hash_combine(hash, ellipsis);
        return hash;
    }

    static bool equal_key(const TextLayoutCacheKey &key, const std::wstring &text, const TextFormatResource &format,
        float width, float height, bool ellipsis)
    {
        return key.text == text && key.style == format.style && key.alignment == format.alignment &&
               key.paragraph_alignment == format.paragraph_alignment && key.wrapping == format.wrapping &&
               key.width == width && key.height == height && key.ellipsis == ellipsis;
    }

    void evict(std::list<Entry>::iterator entry)
    {
        const auto [first, last] = m_index.equal_range(entry->hash);
        for (auto candidate = first; candidate != last; ++candidate)
        {
            if (candidate->second != entry) continue;
            m_index.erase(candidate);
            break;
        }
        m_lru.erase(entry);
    }

    std::uint64_t m_generation{};
    std::list<Entry> m_lru;
    std::unordered_multimap<size_t, std::list<Entry>::iterator> m_index;
};

struct TextMeasurement
{
    float width{};
    float height{};
    UINT32 line_count{};
    float baseline{};
    bool truncated{};
};

struct TextMeasurementCacheKey
{
    std::wstring text;
    TextStyle style{};
    float width{};
    float height{};
    lua_Integer max_lines{};
    DWRITE_WORD_WRAPPING wrapping{};
    bool has_width{};
    bool has_height{};
};

class TextMeasurementCache
{
  public:
    bool get(const std::wstring &text, const TextStyle &style, float width, float height, lua_Integer max_lines,
        DWRITE_WORD_WRAPPING wrapping, bool has_width, bool has_height, TextMeasurement *measurement)
    {
        const size_t hash = hash_key(text, style, width, height, max_lines, wrapping, has_width, has_height);
        const auto [first, last] = m_index.equal_range(hash);
        for (auto candidate = first; candidate != last; ++candidate)
        {
            const auto entry = candidate->second;
            if (!equal_key(entry->key, text, style, width, height, max_lines, wrapping, has_width, has_height))
                continue;
            *measurement = entry->measurement;
            m_lru.splice(m_lru.begin(), m_lru, entry);
            return true;
        }
        return false;
    }

    void add(const std::wstring &text, const TextStyle &style, float width, float height, lua_Integer max_lines,
        DWRITE_WORD_WRAPPING wrapping, bool has_width, bool has_height, const TextMeasurement &measurement)
    {
        const size_t hash = hash_key(text, style, width, height, max_lines, wrapping, has_width, has_height);
        TextMeasurementCacheKey key{text, style, width, height, max_lines, wrapping, has_width, has_height};
        m_lru.push_front({std::move(key), measurement, hash});
        m_index.emplace(hash, m_lru.begin());
        while (m_lru.size() > TEXT_MEASUREMENT_CACHE_CAPACITY) evict(std::prev(m_lru.end()));
    }

  private:
    struct Entry
    {
        TextMeasurementCacheKey key{};
        TextMeasurement measurement{};
        size_t hash{};
    };

    template <typename T> static void hash_combine(size_t &seed, const T &value)
    {
        seed ^= std::hash<T>{}(value) + static_cast<size_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
    }

    static size_t hash_key(const std::wstring &text, const TextStyle &style, float width, float height,
        lua_Integer max_lines, DWRITE_WORD_WRAPPING wrapping, bool has_width, bool has_height)
    {
        size_t hash = std::hash<std::wstring>{}(text);
        hash_combine(hash, style.family);
        hash_combine(hash, style.size);
        hash_combine(hash, static_cast<int>(style.weight));
        hash_combine(hash, static_cast<int>(style.slant));
        hash_combine(hash, style.underline);
        hash_combine(hash, style.strikethrough);
        hash_combine(hash, style.letter_spacing);
        hash_combine(hash, style.line_height);
        hash_combine(hash, style.has_line_height);
        hash_combine(hash, width);
        hash_combine(hash, height);
        hash_combine(hash, max_lines);
        hash_combine(hash, static_cast<int>(wrapping));
        hash_combine(hash, has_width);
        hash_combine(hash, has_height);
        return hash;
    }

    static bool equal_key(const TextMeasurementCacheKey &key, const std::wstring &text, const TextStyle &style,
        float width, float height, lua_Integer max_lines, DWRITE_WORD_WRAPPING wrapping, bool has_width,
        bool has_height)
    {
        return key.text == text && key.style == style && key.width == width && key.height == height &&
               key.max_lines == max_lines && key.wrapping == wrapping && key.has_width == has_width &&
               key.has_height == has_height;
    }

    void evict(std::list<Entry>::iterator entry)
    {
        const auto [first, last] = m_index.equal_range(entry->hash);
        for (auto candidate = first; candidate != last; ++candidate)
        {
            if (candidate->second != entry) continue;
            m_index.erase(candidate);
            break;
        }
        m_lru.erase(entry);
    }

    std::list<Entry> m_lru;
    std::unordered_multimap<size_t, std::list<Entry>::iterator> m_index;
};

inline void push_text_measurement(lua_State *L, const TextMeasurement &measurement)
{
    lua_createtable(L, 0, 5);
    lua_pushnumber(L, measurement.width);
    lua_setfield(L, -2, "w");
    lua_pushnumber(L, measurement.height);
    lua_setfield(L, -2, "h");
    lua_pushinteger(L, measurement.line_count);
    lua_setfield(L, -2, "line_count");
    lua_pushnumber(L, measurement.baseline);
    lua_setfield(L, -2, "baseline");
    lua_pushboolean(L, measurement.truncated);
    lua_setfield(L, -2, "truncated");
}

inline PathOp make_move(float x, float y)
{
    PathOp op{};
    op.kind = PathOp::Kind::Move;
    op.point0 = D2D1::Point2F(x, y);
    return op;
}

inline PathOp make_line(float x, float y)
{
    PathOp op{};
    op.kind = PathOp::Kind::Line;
    op.point0 = D2D1::Point2F(x, y);
    return op;
}

inline PathOp make_cubic(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
    PathOp op{};
    op.kind = PathOp::Kind::Cubic;
    op.point0 = D2D1::Point2F(c1x, c1y);
    op.curve.point1 = D2D1::Point2F(c2x, c2y);
    op.curve.point2 = D2D1::Point2F(x, y);
    return op;
}

inline PathOp make_quadratic(float cx, float cy, float x, float y)
{
    PathOp op{};
    op.kind = PathOp::Kind::Quadratic;
    op.point0 = D2D1::Point2F(cx, cy);
    op.curve.point1 = D2D1::Point2F(x, y);
    return op;
}

inline PathOp make_arc(float x, float y, float radius, float start_angle, float end_angle, bool ccw)
{
    PathOp op{};
    op.kind = PathOp::Kind::Arc;
    op.point0 = D2D1::Point2F(x, y);
    op.arc.radius = radius;
    op.arc.start_angle = start_angle;
    op.arc.end_angle = end_angle;
    op.arc.ccw = ccw;
    return op;
}

inline PathOp make_close()
{
    PathOp op{};
    op.kind = PathOp::Kind::Close;
    return op;
}

inline void append_rect(std::vector<PathOp> &ops, const D2D1_RECT_F &rect)
{
    ops.push_back(make_move(rect.left, rect.top));
    ops.push_back(make_line(rect.right, rect.top));
    ops.push_back(make_line(rect.right, rect.bottom));
    ops.push_back(make_line(rect.left, rect.bottom));
    ops.push_back(make_close());
}

inline void append_round_rect(std::vector<PathOp> &ops, const D2D1_RECT_F &rect, float radius)
{
    const float max_radius = std::min(rect.right - rect.left, rect.bottom - rect.top) * 0.5f;
    const float rad = std::clamp(radius, 0.0f, std::max(0.0f, max_radius));
    if (!(rad > 0))
    {
        append_rect(ops, rect);
        return;
    }
    const float left_center = rect.left + rad;
    const float right_center = rect.right - rad;
    const float top_center = rect.top + rad;
    const float bottom_center = rect.bottom - rad;
    ops.push_back(make_move(left_center, rect.top));
    ops.push_back(make_line(right_center, rect.top));
    ops.push_back(make_arc(right_center, top_center, rad, -std::numbers::pi * 0.5f, 0, false));
    ops.push_back(make_line(rect.right, bottom_center));
    ops.push_back(make_arc(right_center, bottom_center, rad, 0, std::numbers::pi * 0.5f, false));
    ops.push_back(make_line(left_center, rect.bottom));
    ops.push_back(make_arc(left_center, bottom_center, rad, std::numbers::pi * 0.5f, std::numbers::pi, false));
    ops.push_back(make_line(rect.left, top_center));
    ops.push_back(make_arc(left_center, top_center, rad, std::numbers::pi, std::numbers::pi * 1.5f, false));
    ops.push_back(make_close());
}

inline void append_circle(std::vector<PathOp> &ops, const D2D1_RECT_F &rect)
{
    const float center_x = (rect.left + rect.right) * 0.5f;
    const float center_y = (rect.top + rect.bottom) * 0.5f;
    const float radius = std::min(rect.right - rect.left, rect.bottom - rect.top) * 0.5f;
    if (!(radius > 0)) return;
    ops.push_back(make_move(center_x + radius, center_y));
    ops.push_back(make_arc(center_x, center_y, radius, 0, 2 * std::numbers::pi, false));
    ops.push_back(make_close());
}

inline bool points_equal(D2D1_POINT_2F a, D2D1_POINT_2F b)
{
    return std::fabs(a.x - b.x) < 1e-6f && std::fabs(a.y - b.y) < 1e-6f;
}

inline ComPtr<ID2D1PathGeometry> build_geometry(ID2D1Factory *factory, const std::vector<PathOp> &ops)
{
    ComPtr<ID2D1PathGeometry> geometry;
    need(factory->CreatePathGeometry(&geometry), "ID2D1Factory::CreatePathGeometry");
    need(geometry, "ID2D1Factory::CreatePathGeometry returned null");
    ComPtr<ID2D1GeometrySink> sink;
    need(geometry->Open(&sink), "ID2D1PathGeometry::Open");
    need(sink, "ID2D1PathGeometry::Open returned null");

    sink->SetFillMode(D2D1_FILL_MODE_WINDING);
    bool figure_open = false;
    D2D1_POINT_2F current{};

    auto begin = [&](D2D1_POINT_2F point) {
        sink->BeginFigure(point, D2D1_FIGURE_BEGIN_FILLED);
        figure_open = true;
        current = point;
    };
    auto end_figure = [&](bool close) {
        if (!figure_open) return;
        sink->EndFigure(close ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
        figure_open = false;
    };

    for (const auto &op : ops)
    {
        switch (op.kind)
        {
        case PathOp::Kind::Move:
            end_figure(false);
            begin(op.point0);
            break;
        case PathOp::Kind::Line:
            if (!figure_open)
                begin(op.point0);
            else
                sink->AddLine(op.point0);
            current = op.point0;
            break;
        case PathOp::Kind::Cubic:
            if (!figure_open) begin(op.point0);
            sink->AddBezier(D2D1_BEZIER_SEGMENT{op.point0, op.curve.point1, op.curve.point2});
            current = op.curve.point2;
            break;
        case PathOp::Kind::Quadratic: {
            const D2D1_POINT_2F start = figure_open ? current : op.point0;
            if (!figure_open) begin(start);
            const D2D1_POINT_2F control = op.point0;
            const D2D1_POINT_2F end = op.curve.point1;
            const D2D1_POINT_2F c1 = D2D1::Point2F(
                start.x + (2.0f / 3.0f) * (control.x - start.x), start.y + (2.0f / 3.0f) * (control.y - start.y));
            const D2D1_POINT_2F c2 =
                D2D1::Point2F(end.x + (2.0f / 3.0f) * (control.x - end.x), end.y + (2.0f / 3.0f) * (control.y - end.y));
            sink->AddBezier(D2D1_BEZIER_SEGMENT{c1, c2, end});
            current = end;
            break;
        }
        case PathOp::Kind::Arc: {
            if (!(op.arc.radius > 0)) break;
            const float two_pi = 2 * std::numbers::pi;
            const float delta = op.arc.end_angle - op.arc.start_angle;
            if (std::fabs(delta) < 1e-6f) break;
            float sweep = std::fmod(delta, two_pi);
            if (std::fabs(sweep) < 1e-6f)
                sweep = op.arc.ccw ? -two_pi : two_pi;
            else if (op.arc.ccw && sweep > 0)
                sweep -= two_pi;
            else if (!op.arc.ccw && sweep < 0)
                sweep += two_pi;
            const D2D1_POINT_2F center = op.point0;
            const D2D1_POINT_2F start = D2D1::Point2F(center.x + op.arc.radius * std::cos(op.arc.start_angle),
                center.y + op.arc.radius * std::sin(op.arc.start_angle));
            if (!figure_open)
                begin(start);
            else if (!points_equal(current, start))
                sink->AddLine(start);
            const int segments = std::max(1, static_cast<int>(std::ceil(std::fabs(sweep) / (std::numbers::pi * 0.5f))));
            const auto direction = op.arc.ccw ? D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE : D2D1_SWEEP_DIRECTION_CLOCKWISE;
            for (int i = 1; i <= segments; ++i)
            {
                const float angle = op.arc.start_angle + sweep * static_cast<float>(i) / static_cast<float>(segments);
                D2D1_ARC_SEGMENT segment{};
                segment.point = D2D1::Point2F(
                    center.x + op.arc.radius * std::cos(angle), center.y + op.arc.radius * std::sin(angle));
                segment.size = D2D1::SizeF(op.arc.radius, op.arc.radius);
                segment.rotationAngle = 0;
                segment.sweepDirection = direction;
                segment.arcSize = D2D1_ARC_SIZE_SMALL;
                sink->AddArc(segment);
            }
            current = D2D1::Point2F(center.x + op.arc.radius * std::cos(op.arc.start_angle + sweep),
                center.y + op.arc.radius * std::sin(op.arc.start_angle + sweep));
            break;
        }
        case PathOp::Kind::Close:
            end_figure(true);
            break;
        }
    }
    end_figure(false);
    need(sink->Close(), "ID2D1PathGeometry::Close");
    return geometry;
}

inline void create_text_factory(ComPtr<IDWriteFactory> &factory)
{
    factory.Reset();
    need(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
             reinterpret_cast<IUnknown **>(factory.GetAddressOf())),
        "DWriteCreateFactory");
    need(factory, "DWriteCreateFactory returned null");
}

inline void create_text_format(IDWriteFactory *factory, const TextStyle &style, ComPtr<IDWriteTextFormat> &format)
{
    format.Reset();
    need(factory->CreateTextFormat(style.family.c_str(), nullptr, style.weight, style.slant, DWRITE_FONT_STRETCH_NORMAL,
             style.size, L"", &format),
        "IDWriteFactory::CreateTextFormat");
    need(format, "IDWriteFactory::CreateTextFormat returned null");
    if (style.has_line_height)
    {
        const float spacing = style.size * style.line_height;
        need(format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, spacing, spacing * 0.8f),
            "IDWriteTextFormat::SetLineSpacing");
    }
}

inline void apply_text_style(IDWriteTextLayout *layout, const TextStyle &style, UINT32 length)
{
    const DWRITE_TEXT_RANGE range{0, length};
    if (style.underline) need(layout->SetUnderline(TRUE, range), "IDWriteTextLayout::SetUnderline");
    if (style.strikethrough) need(layout->SetStrikethrough(TRUE, range), "IDWriteTextLayout::SetStrikethrough");
    if (style.letter_spacing != 0)
    {
        ComPtr<IDWriteTextLayout1> layout1;
        need(layout->QueryInterface(IID_PPV_ARGS(&layout1)),
            "IDWriteTextLayout1 is unsupported by this DirectWrite version (letter spacing)");
        need(layout1, "IDWriteTextLayout1 QueryInterface returned null");
        need(
            layout1->SetCharacterSpacing(0, style.letter_spacing, 0, range), "IDWriteTextLayout1::SetCharacterSpacing");
    }
}

inline void create_image_target(ID2D1RenderTarget *parent, UINT width, UINT height, bool clear,
    ComPtr<ID2D1BitmapRenderTarget> &target, ComPtr<ID2D1Bitmap> &bitmap)
{
    target.Reset();
    bitmap.Reset();
    const D2D1_SIZE_F dip_size = D2D1::SizeF(static_cast<float>(width), static_cast<float>(height));
    const D2D1_SIZE_U pixel_size = D2D1::SizeU(width, height);
    need(parent->CreateCompatibleRenderTarget(
             &dip_size, &pixel_size, nullptr, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &target),
        "ID2D1RenderTarget::CreateCompatibleRenderTarget");
    need(target, "ID2D1RenderTarget::CreateCompatibleRenderTarget returned null");
    if (clear)
    {
        target->BeginDraw();
        target->Clear(D2D1::ColorF(0, 0, 0, 0));
        need(target->EndDraw(), "ID2D1BitmapRenderTarget::EndDraw");
    }
    need(target->GetBitmap(&bitmap), "ID2D1BitmapRenderTarget::GetBitmap");
    need(bitmap, "ID2D1BitmapRenderTarget::GetBitmap returned null");
}

inline Image *push_image(
    lua_State *L, ComPtr<ID2D1BitmapRenderTarget> &&target, ComPtr<ID2D1Bitmap> &&bitmap, UINT width, UINT height)
{
    auto *image = new (lua_newuserdata(L, sizeof(Image))) Image{};
    image->target = std::move(target);
    image->bitmap = std::move(bitmap);
    image->width = width;
    image->height = height;
    luaL_getmetatable(L, IMAGE_MT);
    lua_setmetatable(L, -2);
    return image;
}

inline int push_decode_error(lua_State *L, const char *operation, HRESULT hr)
{
    const auto message = hresult_message(operation, hr);
    lua_pushnil(L);
    lua_pushlstring(L, message.data(), message.size());
    return 2;
}

inline int decode_source(lua_State *L, IWICBitmapSource *source)
{
    UINT width{}, height{};
    HRESULT hr = source->GetSize(&width, &height);
    if (FAILED(hr) || !width || !height)
        return push_decode_error(L, "IWICBitmapSource::GetSize", FAILED(hr) ? hr : E_INVALIDARG);
    ID2D1RenderTarget *parent = check_current_target(L);
    const UINT max_bitmap_size = parent->GetMaximumBitmapSize();
    if (!max_bitmap_size || width > max_bitmap_size || height > max_bitmap_size)
        return push_decode_error(L, "image dimensions exceed the Direct2D bitmap limit", E_INVALIDARG);

    ComPtr<IWICImagingFactory> wic;
    need(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wic.GetAddressOf())),
        "CoCreateInstance(WICImagingFactory)");
    need(wic, "CoCreateInstance(WICImagingFactory) returned null");
    ComPtr<IWICFormatConverter> converter;
    need(wic->CreateFormatConverter(&converter), "IWICImagingFactory::CreateFormatConverter");
    need(converter, "IWICImagingFactory::CreateFormatConverter returned null");
    hr = converter->Initialize(
        source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return push_decode_error(L, "WIC pixel conversion", hr);

    ComPtr<ID2D1BitmapRenderTarget> target;
    ComPtr<ID2D1Bitmap> bitmap;
    create_image_target(parent, width, height, false, target, bitmap);
    ComPtr<ID2D1Bitmap> decoded;
    need(target->CreateBitmapFromWicBitmap(converter.Get(), nullptr, &decoded),
        "ID2D1RenderTarget::CreateBitmapFromWicBitmap");
    need(decoded, "ID2D1RenderTarget::CreateBitmapFromWicBitmap returned null");
    target->BeginDraw();
    target->Clear(D2D1::ColorF(0, 0, 0, 0));
    target->DrawBitmap(decoded.Get(), D2D1::RectF(0, 0, static_cast<float>(width), static_cast<float>(height)), 1,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
    need(target->EndDraw(), "ID2D1BitmapRenderTarget::EndDraw");
    push_image(L, std::move(target), std::move(bitmap), width, height);
    return 1;
}

inline int decode_decoder(lua_State *L, IWICBitmapDecoder *decoder)
{
    ComPtr<IWICBitmapFrameDecode> frame;
    const HRESULT hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) return push_decode_error(L, "IWICBitmapDecoder::GetFrame", FAILED(hr) ? hr : E_FAIL);
    return decode_source(L, frame.Get());
}

inline int image_close(lua_State *L)
{
    auto *image = static_cast<Image *>(luaL_checkudata(L, 1, IMAGE_MT));
    if (image->painting) return luaL_error(L, "cannot close a PainterImage while it is being painted");
    close_image(image);
    return 0;
}

inline int image_gc(lua_State *L)
{
    auto *image = static_cast<Image *>(luaL_checkudata(L, 1, IMAGE_MT));
    close_image(image);
    image->~Image();
    return 0;
}

inline int painter_gc(lua_State *L)
{
    auto *painter = static_cast<Painter *>(luaL_checkudata(L, 1, PAINTER_MT));
    invalidate_painter(painter);
    painter->~Painter();
    return 0;
}

inline int image_index(lua_State *L)
{
    auto *image = static_cast<Image *>(luaL_checkudata(L, 1, IMAGE_MT));
    const char *key = luaL_checkstring(L, 2);
    if (strcmp(key, "w") == 0 || strcmp(key, "h") == 0)
    {
        if (image->closed || !image->target || !image->bitmap)
            return luaL_error(L, "attempt to use a closed PainterImage");
        lua_pushinteger(L, strcmp(key, "w") == 0 ? image->width : image->height);
        return 1;
    }
    luaL_getmetatable(L, IMAGE_MT);
    lua_getfield(L, -1, key);
    return 1;
}

inline void emit_path_command(Painter *painter, CommandType type, const D2D1_COLOR_F &color, UINT32 stroke)
{
    Command command{};
    command.type = type;
    command.path.transform = {painter->transform._11, painter->transform._12, painter->transform._21,
        painter->transform._22, painter->transform._31, painter->transform._32};
    command.path.color = color;
    command.path.brush = intern_brush(painter, color);
    command.path.stroke = stroke;
    command.path.path = static_cast<UINT32>(painter->paths.size());
    painter->paths.push_back({painter->path_ops, painter->path_texts});
    painter->commands.push_back(std::move(command));
}

inline D2D1_POINT_2F transform_point(const D2D1::Matrix3x2F &matrix, D2D1_POINT_2F point)
{
    return D2D1::Point2F(point.x * matrix._11 + point.y * matrix._21 + matrix._31,
        point.x * matrix._12 + point.y * matrix._22 + matrix._32);
}

inline D2D1_RECT_F transform_rect(const D2D1::Matrix3x2F &matrix, const D2D1_RECT_F &rect)
{
    const D2D1_POINT_2F corners[4] = {transform_point(matrix, D2D1::Point2F(rect.left, rect.top)),
        transform_point(matrix, D2D1::Point2F(rect.right, rect.top)),
        transform_point(matrix, D2D1::Point2F(rect.right, rect.bottom)),
        transform_point(matrix, D2D1::Point2F(rect.left, rect.bottom))};
    float min_x = corners[0].x;
    float min_y = corners[0].y;
    float max_x = corners[0].x;
    float max_y = corners[0].y;
    for (int i = 1; i < 4; ++i)
    {
        min_x = std::min(min_x, corners[i].x);
        min_y = std::min(min_y, corners[i].y);
        max_x = std::max(max_x, corners[i].x);
        max_y = std::max(max_y, corners[i].y);
    }
    return D2D1::RectF(min_x, min_y, max_x, max_y);
}

inline int painter_clear(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    Command command{};
    command.type = CommandType::Clear;
    command.clear.color = check_color(L, 2);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_begin_path(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    painter->path_ops.clear();
    painter->path_texts.clear();
    return 0;
}

inline int painter_move_to(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    painter->path_ops.push_back(make_move(check_coordinate(L, 2, "x"), check_coordinate(L, 3, "y")));
    return 0;
}

inline int painter_line_to(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    painter->path_ops.push_back(make_line(check_coordinate(L, 2, "x"), check_coordinate(L, 3, "y")));
    return 0;
}

inline int painter_cubic_to(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    painter->path_ops.push_back(
        make_cubic(check_coordinate(L, 2, "c1x"), check_coordinate(L, 3, "c1y"), check_coordinate(L, 4, "c2x"),
            check_coordinate(L, 5, "c2y"), check_coordinate(L, 6, "x"), check_coordinate(L, 7, "y")));
    return 0;
}

inline int painter_quadratic_to(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    painter->path_ops.push_back(make_quadratic(check_coordinate(L, 2, "cx"), check_coordinate(L, 3, "cy"),
        check_coordinate(L, 4, "x"), check_coordinate(L, 5, "y")));
    return 0;
}

inline int painter_arc(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const float radius = check_coordinate(L, 4, "radius");
    if (radius < 0) luaL_error(L, "arc radius must be non-negative");
    const bool ccw = lua_isnoneornil(L, 7) ? false : lua_toboolean(L, 7) != 0;
    painter->path_ops.push_back(make_arc(check_coordinate(L, 2, "x"), check_coordinate(L, 3, "y"), radius,
        luaL_checkfinitenumber(L, 5, "start_angle"), luaL_checkfinitenumber(L, 6, "end_angle"), ccw));
    return 0;
}

inline int painter_close_path(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    painter->path_ops.push_back(make_close());
    return 0;
}

inline int painter_save(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    painter->states.push_back({painter->transform, painter->clips.size()});
    return 0;
}

inline int painter_restore(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    if (painter->states.empty()) return luaL_error(L, "restore() called without a matching save()");
    const StateSnapshot state = painter->states.back();
    painter->states.pop_back();
    while (painter->clips.size() > state.clip_depth)
    {
        painter->clips.pop_back();
        Command command{};
        command.type = CommandType::PopClip;
        painter->commands.push_back(std::move(command));
    }
    painter->transform = state.transform;
    return 0;
}

inline int painter_clip(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const D2D1_RECT_F device = transform_rect(painter->transform, check_rect(L, 2));
    Command command{};
    command.type = CommandType::PushClip;
    command.clip.bounds = device;
    painter->commands.push_back(std::move(command));
    painter->clips.push_back(device);
    return 0;
}

inline int painter_translate(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const float x = check_coordinate(L, 2, "x");
    const float y = check_coordinate(L, 3, "y");
    painter->transform = D2D1::Matrix3x2F::Translation(x, y) * painter->transform;
    validate_transform(L, painter->transform);
    return 0;
}

inline int painter_rotate(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const float radians = check_coordinate(L, 2, "angle");
    painter->transform = D2D1::Matrix3x2F::Rotation(radians * (180.0f / std::numbers::pi)) * painter->transform;
    validate_transform(L, painter->transform);
    return 0;
}

inline int painter_scale(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const float x = check_coordinate(L, 2, "x");
    const float y = check_coordinate(L, 3, "y");
    painter->transform = D2D1::Matrix3x2F::Scale(x, y) * painter->transform;
    validate_transform(L, painter->transform);
    return 0;
}

inline int painter_stroke(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto color = check_color(L, 2);
    const auto stroke = check_stroke(L, 3);
    emit_path_command(painter, CommandType::StrokePath, color, intern_stroke(painter, stroke));
    return 0;
}

inline int painter_fill(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto color = check_color(L, 2);
    emit_path_command(painter, CommandType::FillPath, color, 0);
    return 0;
}

inline int painter_text(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto text = luaL_checkstlwstring(L, 2);
    const auto rect = check_rect(L, 3);
    const auto style = check_text_style(L, 4);

    DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    DWRITE_PARAGRAPH_ALIGNMENT paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_WRAP;
    bool clip = true;
    std::string overflow = "clip";
    if (!lua_isnoneornil(L, 4))
    {
        const int absolute = lua_absindex(L, 4);
        const auto align_x = luaL_tablestring(L, absolute, "align_x", "left");
        if (align_x == "left")
            alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
        else if (align_x == "center")
            alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
        else if (align_x == "right")
            alignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
        else if (align_x == "justify")
            alignment = DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
        else
            luaL_error(L, "invalid horizontal text alignment '%s'", align_x.c_str());
        const auto align_y = luaL_tablestring(L, absolute, "align_y", "top");
        if (align_y == "top")
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        else if (align_y == "center")
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        else if (align_y == "bottom")
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
        else
            luaL_error(L, "invalid vertical text alignment '%s'", align_y.c_str());
        wrapping = parse_wrap(L, luaL_tablestring(L, absolute, "wrap", "word"));
        overflow = luaL_tablestring(L, absolute, "overflow", "clip");
        if (overflow != "visible" && overflow != "clip" && overflow != "ellipsis")
            luaL_error(L, "invalid text overflow mode '%s'", overflow.c_str());
        clip = luaL_tablebool(L, absolute, "clip", true);
    }

    TextRun run{};
    run.text = std::move(text);
    run.rect = rect;
    run.format = intern_text_format(painter, style, alignment, paragraph_alignment, wrapping);
    run.ellipsis = overflow == "ellipsis";
    run.fit = luaL_tablebool(L, 4, "fit", false);
    if (clip && overflow != "visible") run.options = D2D1_DRAW_TEXT_OPTIONS_CLIP;
    painter->path_texts.push_back(std::move(run));
    return 0;
}

inline int painter_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    append_rect(painter->path_ops, check_rect(L, 2));
    return 0;
}

inline int painter_round_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    append_round_rect(painter->path_ops, check_rect(L, 2), luaL_checkfinitenumber(L, 3, "radius"));
    return 0;
}

inline int painter_circle(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    append_circle(painter->path_ops, check_rect(L, 2));
    return 0;
}

inline int painter_line(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    painter->path_ops.push_back(make_move(check_coordinate(L, 2, "x1"), check_coordinate(L, 3, "y1")));
    painter->path_ops.push_back(make_line(check_coordinate(L, 4, "x2"), check_coordinate(L, 5, "y2")));
    return 0;
}

inline int painter_polyline(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto points = check_points(L, 2);
    painter->path_ops.push_back(make_move(points.front().x, points.front().y));
    for (size_t i = 1; i < points.size(); ++i) painter->path_ops.push_back(make_line(points[i].x, points[i].y));
    return 0;
}

inline int painter_polygon(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto points = check_points(L, 2);
    if (points.size() < 3) luaL_error(L, "a polygon requires at least three points");
    painter->path_ops.push_back(make_move(points.front().x, points.front().y));
    for (size_t i = 1; i < points.size(); ++i) painter->path_ops.push_back(make_line(points[i].x, points[i].y));
    painter->path_ops.push_back(make_close());
    return 0;
}

inline int painter_image(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto *image = check_image(L, 2);
    const auto destination = check_rect(L, 3);
    D2D1_RECT_F source = D2D1::RectF(0, 0, static_cast<float>(image->width), static_cast<float>(image->height));
    D2D1_RECT_F center{};
    bool has_source = false;
    bool nine_sliced = false;
    float opacity = 1;
    D2D1_BITMAP_INTERPOLATION_MODE interpolation = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
    bool tinted = false;
    D2D1_COLOR_F tint = D2D1::ColorF(1, 1, 1, 1);
    if (!lua_isnoneornil(L, 4))
    {
        luaL_checktype(L, 4, LUA_TTABLE);
        const int absolute = lua_absindex(L, 4);
        opacity = luaL_tablenumber(L, absolute, "opacity", 1);
        if (opacity < 0 || opacity > 1) luaL_error(L, "image opacity must be in the range [0, 1]");
        const auto sampling = luaL_tablestring(L, absolute, "sampling", "linear");
        if (sampling == "nearest")
            interpolation = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        else if (sampling != "linear")
            luaL_error(L, "invalid image sampling mode '%s'", sampling.c_str());
        lua_getfield(L, absolute, "source");
        if (!lua_isnil(L, -1))
        {
            source = check_rect(L, -1);
            has_source = true;
        }
        lua_pop(L, 1);
        lua_getfield(L, absolute, "center");
        if (!lua_isnil(L, -1))
        {
            center = check_rect(L, -1);
            nine_sliced = true;
        }
        lua_pop(L, 1);
        lua_getfield(L, absolute, "tint");
        if (!lua_isnil(L, -1))
        {
            tint = check_color(L, -1);
            tinted = tint.r != 1 || tint.g != 1 || tint.b != 1 || tint.a != 1;
        }
        lua_pop(L, 1);
    }
    if (source.left < 0 || source.top < 0 || source.right > static_cast<float>(image->width) ||
        source.bottom > static_cast<float>(image->height))
        luaL_error(L, "image source rectangle is outside the image");
    if (nine_sliced && !has_source) luaL_error(L, "nine-sliced images require a source rectangle");
    if (nine_sliced && (center.left < source.left || center.top < source.top || center.right > source.right ||
                           center.bottom > source.bottom))
        luaL_error(L, "image center rectangle is outside the source rectangle");
    if (tinted)
    {
        ComPtr<ID2D1DeviceContext> device_context;
        if (FAILED(painter->target->QueryInterface(IID_PPV_ARGS(&device_context))) || !device_context)
            luaL_error(L, "image tinting is unavailable on this drawing target");
    }

    ImagePayload image_payload{};
    image_payload.tint = tint;
    image_payload.opacity = opacity;
    image_payload.interpolation = interpolation;
    image_payload.tinted = tinted;
    const auto add_slice = [&](const ImageSlice &slice) { image_payload.slices[image_payload.slice_count++] = slice; };
    if (nine_sliced)
    {
        const float left_width = center.left - source.left;
        const float right_width = source.right - center.right;
        const float top_height = center.top - source.top;
        const float bottom_height = source.bottom - center.bottom;
        const float destination_width = destination.right - destination.left;
        const float destination_height = destination.bottom - destination.top;
        if (destination_width < left_width + right_width || destination_height < top_height + bottom_height)
        {
            add_slice({center, destination});
        }
        else
        {
            const float source_x[] = {source.left, center.left, center.right, source.right};
            const float source_y[] = {source.top, center.top, center.bottom, source.bottom};
            const float destination_x[] = {
                destination.left, destination.left + left_width, destination.right - right_width, destination.right};
            const float destination_y[] = {
                destination.top, destination.top + top_height, destination.bottom - bottom_height, destination.bottom};
            for (int y = 0; y < 3; ++y)
            {
                for (int x = 0; x < 3; ++x)
                {
                    const auto slice_source = D2D1::RectF(source_x[x], source_y[y], source_x[x + 1], source_y[y + 1]);
                    const auto slice_destination =
                        D2D1::RectF(destination_x[x], destination_y[y], destination_x[x + 1], destination_y[y + 1]);
                    if (slice_source.right > slice_source.left && slice_source.bottom > slice_source.top &&
                        slice_destination.right > slice_destination.left &&
                        slice_destination.bottom > slice_destination.top)
                        add_slice({slice_source, slice_destination});
                }
            }
        }
    }
    else
    {
        add_slice({source, destination});
    }

    const UINT32 image_resource = intern_image(painter, *image);
    Command command{};
    command.type = CommandType::Image;
    command.image.transform = {painter->transform._11, painter->transform._12, painter->transform._21,
        painter->transform._22, painter->transform._31, painter->transform._32};
    command.image.resource = image_resource;
    command.image.payload = static_cast<UINT32>(painter->image_payloads.size());
    painter->image_payloads.push_back(std::move(image_payload));
    painter->commands.push_back(std::move(command));
    return 0;
}

// Command execution.

inline void ensure_d2d_factory(Painter *painter, ComPtr<ID2D1Factory> &factory)
{
    if (!factory) painter->target->GetFactory(factory.ReleaseAndGetAddressOf());
    need(factory, "ID2D1RenderTarget::GetFactory returned null");
}

inline void realize_brush(Painter *painter, UINT32 index, ID2D1SolidColorBrush **brush)
{
    auto &resource = painter->brushes[index];
    if (!resource.native)
    {
        need(painter->target->CreateSolidColorBrush(resource.color, &resource.native),
            "ID2D1RenderTarget::CreateSolidColorBrush");
        need(resource.native, "ID2D1RenderTarget::CreateSolidColorBrush returned null");
    }
    *brush = resource.native.Get();
}

inline void realize_stroke(
    Painter *painter, UINT32 index, ComPtr<ID2D1Factory> &factory, ID2D1StrokeStyle **style, float *width)
{
    auto &resource = painter->strokes[index];
    *width = resource.value.width;
    if (!resource.value.specified)
    {
        *style = nullptr;
        return;
    }
    if (!resource.native)
    {
        ensure_d2d_factory(painter, factory);
        need(factory->CreateStrokeStyle(resource.value.properties,
                 resource.value.dashes.empty() ? nullptr : resource.value.dashes.data(),
                 static_cast<UINT32>(resource.value.dashes.size()), &resource.native),
            "ID2D1Factory::CreateStrokeStyle");
        need(resource.native, "ID2D1Factory::CreateStrokeStyle returned null");
    }
    *style = resource.native.Get();
}

inline void ensure_text_factory(ComPtr<IDWriteFactory> &factory)
{
    if (!factory) create_text_factory(factory);
}

inline void realize_text_format(
    Painter *painter, UINT32 index, ComPtr<IDWriteFactory> &factory, ComPtr<IDWriteTextFormat> &format)
{
    auto &resource = painter->text_formats[index];
    if (!resource.native)
    {
        ensure_text_factory(factory);
        create_text_format(factory.Get(), resource.style, resource.native);
        need(resource.native->SetTextAlignment(resource.alignment), "IDWriteTextFormat::SetTextAlignment");
        need(resource.native->SetParagraphAlignment(resource.paragraph_alignment),
            "IDWriteTextFormat::SetParagraphAlignment");
        need(resource.native->SetWordWrapping(resource.wrapping), "IDWriteTextFormat::SetWordWrapping");
    }
    format = resource.native;
}

inline void draw_text_runs(Painter *painter, const std::vector<TextRun> &runs, ID2D1SolidColorBrush *brush,
    ComPtr<IDWriteFactory> &text_factory, TextLayoutCache &cache, const D2D1::Matrix3x2F &command_transform)
{
    for (const auto &run : runs)
    {
        if (run.text.empty()) continue;
        const float width = run.rect.right - run.rect.left;
        const float height = run.rect.bottom - run.rect.top;
        if (!(width > 0) || !(height > 0)) continue;
        const auto &format_resource = painter->text_formats[run.format];
        const float layout_width = run.fit ? MAX_LAYOUT_SIZE : width;
        const float layout_height = run.fit ? MAX_LAYOUT_SIZE : height;
        const bool layout_ellipsis = run.ellipsis && !run.fit;
        ComPtr<IDWriteTextLayout> fit_layout;
        IDWriteTextLayout *layout = nullptr;
        if (run.fit)
        {
            ensure_text_factory(text_factory);
            ComPtr<IDWriteTextFormat> fit_format;
            create_text_format(text_factory.Get(), format_resource.style, fit_format);
            need(fit_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING), "IDWriteTextFormat::SetTextAlignment");
            need(fit_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR),
                "IDWriteTextFormat::SetParagraphAlignment");
            need(fit_format->SetWordWrapping(format_resource.wrapping), "IDWriteTextFormat::SetWordWrapping");
            const UINT32 length = static_cast<UINT32>(std::min<size_t>(run.text.size(), UINT32_MAX));
            need(text_factory->CreateTextLayout(
                     run.text.data(), length, fit_format.Get(), layout_width, layout_height, &fit_layout),
                "IDWriteFactory::CreateTextLayout");
            need(fit_layout, "IDWriteFactory::CreateTextLayout returned null");
            apply_text_style(fit_layout.Get(), format_resource.style, length);
            layout = fit_layout.Get();
        }
        else
        {
            layout = cache.get(run.text, format_resource, layout_width, layout_height, layout_ellipsis);
            if (!layout)
            {
                ComPtr<IDWriteTextFormat> format;
                realize_text_format(painter, run.format, text_factory, format);
                const UINT32 length = static_cast<UINT32>(std::min<size_t>(run.text.size(), UINT32_MAX));
                ComPtr<IDWriteTextLayout> new_layout;
                need(text_factory->CreateTextLayout(
                         run.text.data(), length, format.Get(), layout_width, layout_height, &new_layout),
                    "IDWriteFactory::CreateTextLayout");
                need(new_layout, "IDWriteFactory::CreateTextLayout returned null");
                apply_text_style(new_layout.Get(), format_resource.style, length);
                if (layout_ellipsis)
                {
                    ComPtr<IDWriteInlineObject> ellipsis;
                    need(text_factory->CreateEllipsisTrimmingSign(format.Get(), &ellipsis),
                        "IDWriteFactory::CreateEllipsisTrimmingSign");
                    need(ellipsis, "IDWriteFactory::CreateEllipsisTrimmingSign returned null");
                    const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
                    need(new_layout->SetTrimming(&trimming, ellipsis.Get()), "IDWriteTextLayout::SetTrimming");
                }
                layout = new_layout.Get();
                cache.add(
                    run.text, format_resource, layout_width, layout_height, layout_ellipsis, std::move(new_layout));
            }
        }

        if (!run.fit)
        {
            painter->target->DrawTextLayout(D2D1::Point2F(run.rect.left, run.rect.top), layout, brush, run.options);
            continue;
        }

        DWRITE_TEXT_METRICS metrics{};
        need(layout->GetMetrics(&metrics), "IDWriteTextLayout::GetMetrics");
        const float scale = std::min(1.0f,
            std::min(
                metrics.widthIncludingTrailingWhitespace > 0 ? width / metrics.widthIncludingTrailingWhitespace : 1.0f,
                metrics.height > 0 ? height / metrics.height : 1.0f));
        float x = run.rect.left;
        float y = run.rect.top;
        const float fitted_width = metrics.widthIncludingTrailingWhitespace * scale;
        const float fitted_height = metrics.height * scale;
        if (format_resource.alignment == DWRITE_TEXT_ALIGNMENT_CENTER)
            x += (width - fitted_width) * 0.5f;
        else if (format_resource.alignment == DWRITE_TEXT_ALIGNMENT_TRAILING)
            x += width - fitted_width;
        if (format_resource.paragraph_alignment == DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
            y += (height - fitted_height) * 0.5f;
        else if (format_resource.paragraph_alignment == DWRITE_PARAGRAPH_ALIGNMENT_FAR)
            y += height - fitted_height;

        if (scale != 1.0f)
        {
            painter->target->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale) * command_transform);
            x /= scale;
            y /= scale;
        }
        painter->target->DrawTextLayout(D2D1::Point2F(x, y), layout, brush, run.options);
        if (scale != 1.0f) painter->target->SetTransform(command_transform);
    }
}

inline void execute_tinted_image(
    Painter *painter, UINT32 image, const ImagePayload &payload, const D2D1::Matrix3x2F &transform)
{
    ComPtr<ID2D1DeviceContext> dc;
    need(painter->target->QueryInterface(IID_PPV_ARGS(&dc)), "image tinting requires an ID2D1DeviceContext");
    need(dc, "ID2D1DeviceContext QueryInterface returned null");
    ComPtr<ID2D1Effect> effect;
    need(dc->CreateEffect(CLSID_D2D1ColorMatrix, &effect), "ID2D1DeviceContext::CreateEffect");
    effect->SetInput(0, painter->images[image].bitmap.Get());
    const float alpha = payload.tint.a * payload.opacity;
    const D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(payload.tint.r * alpha, 0, 0, 0, 0, payload.tint.g * alpha, 0, 0,
        0, 0, payload.tint.b * alpha, 0, 0, 0, 0, alpha, 0, 0, 0, 0);
    need(effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix), "ID2D1Effect::SetValue");
    ComPtr<ID2D1Image> output;
    effect->GetOutput(&output);
    need(output, "ID2D1Effect::GetOutput returned null");
    const auto interpolation = payload.interpolation == D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                                   ? D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                                   : D2D1_INTERPOLATION_MODE_LINEAR;
    for (std::uint8_t i = 0; i < payload.slice_count; ++i)
    {
        const auto &slice = payload.slices[i];
        const float source_width = slice.source.right - slice.source.left;
        const float source_height = slice.source.bottom - slice.source.top;
        const float sx = source_width > 0 ? (slice.destination.right - slice.destination.left) / source_width : 1;
        const float sy = source_height > 0 ? (slice.destination.bottom - slice.destination.top) / source_height : 1;
        dc->SetTransform(D2D1::Matrix3x2F::Scale(sx, sy) *
                         D2D1::Matrix3x2F::Translation(slice.destination.left, slice.destination.top) * transform);
        dc->DrawImage(output.Get(), D2D1::Point2F(0, 0), slice.source, interpolation, D2D1_COMPOSITE_MODE_SOURCE_OVER);
    }
    dc->SetTransform(D2D1::Matrix3x2F::Identity());
}

inline void execute_commands(Painter *painter)
{
    ComPtr<ID2D1Factory> d2d_factory;
    ComPtr<IDWriteFactory> text_factory;
    std::vector<D2D1_RECT_F> active_clips;

    auto &text_layout_cache = painter->context->painter_text_layouts;
    if (!text_layout_cache) text_layout_cache = std::make_shared<TextLayoutCache>();
    text_layout_cache->begin_generation();

    for (auto &command : painter->commands)
    {
        if (command.type == CommandType::PushClip)
        {
            painter->target->SetTransform(D2D1::Matrix3x2F::Identity());
            painter->target->PushAxisAlignedClip(command.clip.bounds, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            active_clips.push_back(command.clip.bounds);
            continue;
        }
        if (command.type == CommandType::PopClip)
        {
            if (!active_clips.empty())
            {
                painter->target->PopAxisAlignedClip();
                active_clips.pop_back();
            }
            continue;
        }
        if (command.type == CommandType::Clear)
        {
            painter->target->SetTransform(D2D1::Matrix3x2F::Identity());
            for (size_t i = 0; i < active_clips.size(); ++i) painter->target->PopAxisAlignedClip();
            painter->target->Clear(command.clear.color);
            for (const auto &clip : active_clips)
                painter->target->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            continue;
        }

        painter->target->SetTransform(
            command.type == CommandType::Image ? command.image.transform : command.path.transform);

        if (command.type == CommandType::Image)
        {
            const auto &payload = painter->image_payloads[command.image.payload];
            if (!payload.tinted)
            {
                for (std::uint8_t i = 0; i < payload.slice_count; ++i)
                {
                    const auto &slice = payload.slices[i];
                    painter->target->DrawBitmap(painter->images[command.image.resource].bitmap.Get(), slice.destination,
                        payload.opacity, payload.interpolation, slice.source);
                }
            }
            else
                execute_tinted_image(painter, command.image.resource, payload, to_matrix(command.image.transform));
            continue;
        }

        ID2D1SolidColorBrush *brush = nullptr;
        realize_brush(painter, command.path.brush, &brush);

        const auto &payload = painter->paths[command.path.path];
        ComPtr<ID2D1PathGeometry> geometry;
        if (!payload.ops.empty())
        {
            ensure_d2d_factory(painter, d2d_factory);
            geometry = build_geometry(d2d_factory.Get(), payload.ops);
        }

        if (command.type == CommandType::FillPath)
        {
            if (geometry) painter->target->FillGeometry(geometry.Get(), brush);
            draw_text_runs(
                painter, payload.texts, brush, text_factory, *text_layout_cache, to_matrix(command.path.transform));
        }
        else
        {
            ID2D1StrokeStyle *stroke_style = nullptr;
            float stroke_width = 1.0f;
            realize_stroke(painter, command.path.stroke, d2d_factory, &stroke_style, &stroke_width);
            if (geometry) painter->target->DrawGeometry(geometry.Get(), brush, stroke_width, stroke_style);
        }
    }

    while (!active_clips.empty())
    {
        painter->target->PopAxisAlignedClip();
        active_clips.pop_back();
    }
}

inline int screen_paint(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    auto *context = check_context(L);
    auto *target = context->d2d_render_target_stack.top();

    target->BeginDraw();
    target->SetTransform(D2D1::Matrix3x2F::Identity());
    lua_pushvalue(L, 1);
    auto *painter = push_painter(L, context, target);
    const int status = lua_pcall(L, 1, 0, 0);
    if (status == LUA_OK) execute_commands(painter);
    invalidate_painter(painter);
    const HRESULT hr = target->EndDraw();

    if (status != LUA_OK) return lua_error(L);
    if (FAILED(hr)) return fail_hr(L, "ID2D1RenderTarget::EndDraw", hr);
    return 0;
}

inline int image_paint(lua_State *L)
{
    auto *image = check_image(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (image->painting) return luaL_error(L, "a PainterImage cannot recursively paint itself");
    auto *context = check_context(L);
    image->painting = true;
    image->target->BeginDraw();
    image->target->SetTransform(D2D1::Matrix3x2F::Identity());
    context->d2d_render_target_stack.push(image->target.Get());
    lua_pushvalue(L, 2);
    auto *painter = push_painter(L, context, image->target.Get());
    const int status = lua_pcall(L, 1, 0, 0);
    if (status == LUA_OK) execute_commands(painter);
    invalidate_painter(painter);
    context->d2d_render_target_stack.pop();
    const HRESULT hr = image->target->EndDraw();
    image->painting = false;
    if (status != LUA_OK) return lua_error(L);
    if (FAILED(hr)) return fail_hr(L, "ID2D1BitmapRenderTarget::EndDraw", hr);
    return 0;
}

} // namespace Detail

inline int new_image(lua_State *L)
{
    const lua_Integer width_value = luaL_checkinteger(L, 1);
    const lua_Integer height_value = luaL_checkinteger(L, 2);
    if (width_value <= 0 || height_value <= 0 || width_value > UINT_MAX || height_value > UINT_MAX)
        return luaL_error(L, "image dimensions must be positive 32-bit integers");
    auto *parent = Detail::check_current_target(L);
    const UINT max_bitmap_size = parent->GetMaximumBitmapSize();
    if (!max_bitmap_size || static_cast<lua_Unsigned>(width_value) > max_bitmap_size ||
        static_cast<lua_Unsigned>(height_value) > max_bitmap_size)
        return luaL_error(L, "image dimensions exceed the Direct2D bitmap limit");
    ComPtr<ID2D1BitmapRenderTarget> target;
    ComPtr<ID2D1Bitmap> bitmap;
    Detail::create_image_target(
        parent, static_cast<UINT>(width_value), static_cast<UINT>(height_value), true, target, bitmap);
    Detail::push_image(
        L, std::move(target), std::move(bitmap), static_cast<UINT>(width_value), static_cast<UINT>(height_value));
    return 1;
}

inline int load_image(lua_State *L)
{
    const auto path = luaL_checkstlwstring(L, 1);
    ComPtr<IWICImagingFactory> wic;
    need(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wic.GetAddressOf())),
        "CoCreateInstance(WICImagingFactory)");
    need(wic, "CoCreateInstance(WICImagingFactory) returned null");
    ComPtr<IWICBitmapDecoder> decoder;
    const HRESULT hr =
        wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr) || !decoder)
        return Detail::push_decode_error(L, "WIC image file decoding", FAILED(hr) ? hr : E_FAIL);
    return Detail::decode_decoder(L, decoder.Get());
}

inline int decode_image(lua_State *L)
{
    size_t size{};
    const char *data = luaL_checklstring(L, 1, &size);
    if (!size || size > UINT_MAX)
    {
        lua_pushnil(L);
        lua_pushstring(L, "image data must be a non-empty string no larger than 4 GiB");
        return 2;
    }
    ComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(reinterpret_cast<const BYTE *>(data), static_cast<UINT>(size)));
    if (!stream) return Detail::push_decode_error(L, "SHCreateMemStream", E_OUTOFMEMORY);
    ComPtr<IWICImagingFactory> wic;
    need(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wic.GetAddressOf())),
        "CoCreateInstance(WICImagingFactory)");
    need(wic, "CoCreateInstance(WICImagingFactory) returned null");
    ComPtr<IWICBitmapDecoder> decoder;
    const HRESULT hr = wic->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr) || !decoder)
        return Detail::push_decode_error(L, "WIC memory image decoding", FAILED(hr) ? hr : E_FAIL);
    return Detail::decode_decoder(L, decoder.Get());
}

inline int image_formats(lua_State *L)
{
    static constexpr const char *formats[] = {"bmp", "png", "ico", "jpeg", "gif", "tiff", "wdp"};
    lua_createtable(L, static_cast<int>(std::size(formats)), 0);
    for (size_t i = 0; i < std::size(formats); ++i)
    {
        lua_pushstring(L, formats[i]);
        lua_rawseti(L, -2, static_cast<lua_Integer>(i + 1));
    }
    return 1;
}

inline int measure_text(lua_State *L)
{
    const auto text = luaL_checkstlwstring(L, 1);
    const auto style = Detail::check_text_style(L, 2);
    float width = Detail::MAX_LAYOUT_SIZE;
    float height = Detail::MAX_LAYOUT_SIZE;
    bool has_width = false;
    bool has_height = false;
    lua_Integer max_lines = 0;
    std::string wrap = "none";
    if (!lua_isnoneornil(L, 3))
    {
        luaL_checktype(L, 3, LUA_TTABLE);
        const int absolute = lua_absindex(L, 3);
        lua_getfield(L, absolute, "w");
        if (!lua_isnil(L, -1))
        {
            width = luaL_checkfinitenumber(L, -1, "w");
            if (width < 0) luaL_error(L, "text constraint width must be non-negative");
            width = std::min(width, Detail::MAX_LAYOUT_SIZE);
            has_width = true;
        }
        lua_pop(L, 1);
        lua_getfield(L, absolute, "h");
        if (!lua_isnil(L, -1))
        {
            height = luaL_checkfinitenumber(L, -1, "h");
            if (height < 0) luaL_error(L, "text constraint height must be non-negative");
            height = std::min(height, Detail::MAX_LAYOUT_SIZE);
            has_height = true;
        }
        lua_pop(L, 1);
        wrap = luaL_tablestring(L, absolute, "wrap", has_width ? "word" : "none");
        lua_getfield(L, absolute, "max_lines");
        if (!lua_isnil(L, -1))
        {
            max_lines = luaL_checkinteger(L, -1);
            if (max_lines <= 0 || static_cast<lua_Unsigned>(max_lines) > UINT32_MAX)
                luaL_error(L, "max_lines is out of range");
        }
        lua_pop(L, 1);
    }

    const DWRITE_WORD_WRAPPING wrapping = Detail::parse_wrap(L, wrap);
    Detail::TextMeasurementCache *measurement_cache = nullptr;
    if (auto *environment = LuaManager::get_environment_for_state(L))
    {
        auto &cache = environment->rctx.painter_text_measurements;
        if (!cache) cache = std::make_shared<Detail::TextMeasurementCache>();
        measurement_cache = cache.get();
    }

    Detail::TextMeasurement measurement{};
    if (measurement_cache &&
        measurement_cache->get(text, style, width, height, max_lines, wrapping, has_width, has_height, &measurement))
    {
        Detail::push_text_measurement(L, measurement);
        return 1;
    }

    ComPtr<IDWriteFactory> factory;
    Detail::create_text_factory(factory);
    ComPtr<IDWriteTextFormat> format;
    Detail::create_text_format(factory.Get(), style, format);
    need(format->SetWordWrapping(wrapping), "IDWriteTextFormat::SetWordWrapping");
    ComPtr<IDWriteTextLayout> layout;
    const UINT32 length = static_cast<UINT32>(std::min<size_t>(text.size(), UINT32_MAX));
    need(factory->CreateTextLayout(text.data(), length, format.Get(), width, Detail::MAX_LAYOUT_SIZE, &layout),
        "IDWriteFactory::CreateTextLayout");
    need(layout, "IDWriteFactory::CreateTextLayout returned null");
    Detail::apply_text_style(layout.Get(), style, length);

    DWRITE_TEXT_METRICS metrics{};
    need(layout->GetMetrics(&metrics), "IDWriteTextLayout::GetMetrics");
    UINT32 line_count{};
    layout->GetLineMetrics(nullptr, 0, &line_count);
    std::vector<DWRITE_LINE_METRICS> lines(line_count);
    HRESULT hr = S_OK;
    if (line_count) hr = layout->GetLineMetrics(lines.data(), line_count, &line_count);
    need(hr, "IDWriteTextLayout::GetLineMetrics");

    UINT32 reported_lines = line_count;
    bool truncated = false;
    if (max_lines > 0 && reported_lines > static_cast<UINT32>(max_lines))
    {
        reported_lines = static_cast<UINT32>(max_lines);
        truncated = true;
    }
    float measured_height = 0;
    for (UINT32 i = 0; i < reported_lines; ++i) measured_height += lines[i].height;
    if (line_count == 0) measured_height = metrics.height;
    if (has_height && measured_height > height)
    {
        measured_height = height;
        truncated = true;
    }
    if (has_width && metrics.widthIncludingTrailingWhitespace > width && wrap == "none") truncated = true;
    if (reported_lines < line_count) truncated = true;
    for (const auto &line : lines)
        if (line.isTrimmed) truncated = true;

    measurement.width = has_width ? std::min(metrics.widthIncludingTrailingWhitespace, width)
                                  : metrics.widthIncludingTrailingWhitespace;
    measurement.height = measured_height;
    measurement.line_count = reported_lines;
    measurement.baseline = lines.empty() ? 0 : lines.front().baseline;
    measurement.truncated = truncated;
    if (measurement_cache)
        measurement_cache->add(text, style, width, height, max_lines, wrapping, has_width, has_height, measurement);
    Detail::push_text_measurement(L, measurement);
    return 1;
}

inline void register_types(lua_State *L)
{
    static const luaL_Reg image_methods[] = {
        {"paint", Detail::image_paint}, {"close", Detail::image_close}, {nullptr, nullptr}};
    static const luaL_Reg painter_methods[] = {{"clear", Detail::painter_clear},
        {"begin_path", Detail::painter_begin_path}, {"move_to", Detail::painter_move_to},
        {"line_to", Detail::painter_line_to}, {"cubic_to", Detail::painter_cubic_to},
        {"quadratic_to", Detail::painter_quadratic_to}, {"arc", Detail::painter_arc},
        {"close_path", Detail::painter_close_path}, {"save", Detail::painter_save},
        {"restore", Detail::painter_restore}, {"clip", Detail::painter_clip}, {"translate", Detail::painter_translate},
        {"rotate", Detail::painter_rotate}, {"scale", Detail::painter_scale}, {"stroke", Detail::painter_stroke},
        {"fill", Detail::painter_fill}, {"text", Detail::painter_text}, {"rect", Detail::painter_rect},
        {"round_rect", Detail::painter_round_rect}, {"circle", Detail::painter_circle}, {"line", Detail::painter_line},
        {"polyline", Detail::painter_polyline}, {"polygon", Detail::painter_polygon}, {"image", Detail::painter_image},
        {nullptr, nullptr}};
    luaL_create_metatable(L, Detail::IMAGE_MT, image_methods, Detail::image_index, Detail::image_gc);
    luaL_create_metatable(L, Detail::PAINTER_MT, painter_methods, nullptr, Detail::painter_gc);
}

inline int invoke_paint_callback(lua_State *L)
{
    lua_pushcfunction(L, Detail::screen_paint);
    lua_insert(L, -2);
    return lua_pcall(L, 1, 0, 0);
}
} // namespace LuaCore::Painter
