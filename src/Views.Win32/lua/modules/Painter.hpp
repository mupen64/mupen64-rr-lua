/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.hpp>
#include <lua/LuaManager.hpp>
#include <lua/LuaRenderer.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
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
constexpr const char *BRUSH_MT = "mupen64.PainterBrush";
constexpr const char *IMAGE_MT = "mupen64.PainterImage";
constexpr const char *TEXT_STYLE_MT = "mupen64.PainterTextStyle";
constexpr const char *PAINTER_MT = "mupen64.Painter";
constexpr float MAX_LAYOUT_SIZE = 10000000.0f;
constexpr size_t TEXT_LAYOUT_CACHE_CAPACITY = 2048;
constexpr std::uint64_t TEXT_LAYOUT_CACHE_MAX_UNUSED_GENERATIONS = 120;
constexpr size_t TEXT_MEASUREMENT_CACHE_CAPACITY = 2048;

struct Brush
{
    D2D1_COLOR_F color{};
    bool closed{};
};

struct Image
{
    ID2D1BitmapRenderTarget *target{};
    ID2D1Bitmap *bitmap{};
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
    bool closed{};
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
    ID2D1SolidColorBrush *native{};
};

struct StrokeResource
{
    Stroke value{};
    ID2D1StrokeStyle *native{};
};

struct TextFormatResource
{
    TextStyle style{};
    DWRITE_TEXT_ALIGNMENT alignment{DWRITE_TEXT_ALIGNMENT_LEADING};
    DWRITE_PARAGRAPH_ALIGNMENT paragraph_alignment{DWRITE_PARAGRAPH_ALIGNMENT_NEAR};
    DWRITE_WORD_WRAPPING wrapping{DWRITE_WORD_WRAPPING_WRAP};
    IDWriteTextFormat *native{};
};

struct ImageResource
{
    ID2D1Bitmap *bitmap{};
    UINT width{};
    UINT height{};
};

struct ImageSlice
{
    D2D1_RECT_F source{};
    D2D1_RECT_F destination{};
};

enum class CommandType : std::uint8_t
{
    Clear,
    FillRect,
    StrokeRect,
    FillRoundRect,
    StrokeRoundRect,
    FillEllipse,
    StrokeEllipse,
    Line,
    Polyline,
    FillPolygon,
    StrokePolygon,
    Image,
    Text,
    PushClip,
    PopClip,
};

struct GeometryPayload
{
    std::vector<D2D1_POINT_2F> points;
};

struct ImagePayload
{
    std::vector<ImageSlice> slices;
    D2D1_COLOR_F tint{D2D1::ColorF(1, 1, 1, 1)};
    float opacity{1.0f};
    D2D1_BITMAP_INTERPOLATION_MODE interpolation{D2D1_BITMAP_INTERPOLATION_MODE_LINEAR};
    bool tinted{};
};

struct TextPayload
{
    std::wstring text;
    D2D1_DRAW_TEXT_OPTIONS options{D2D1_DRAW_TEXT_OPTIONS_NONE};
    bool ellipsis{};
};

struct Command
{
    CommandType type{};
    UINT32 brush{};
    UINT32 resource{};
    UINT32 payload{};
    D2D1_RECT_F bounds{};
    float scalar{};
};

struct Painter
{
    ID2D1RenderTarget *target{};
    LuaRenderingContext *context{};
    std::vector<D2D1_RECT_F> clips;
    std::vector<Command> commands;
    std::vector<GeometryPayload> geometry_payloads;
    std::vector<ImagePayload> image_payloads;
    std::vector<TextPayload> text_payloads;
    std::vector<BrushResource> brushes;
    std::vector<StrokeResource> strokes;
    std::vector<TextFormatResource> text_formats;
    std::vector<ImageResource> images;
    bool active{};
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

inline D2D1_RECT_F check_rect(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    const float x = luaL_tablenumber(L, index, "x", 0, true);
    const float y = luaL_tablenumber(L, index, "y", 0, true);
    const float width = luaL_tablenumber(L, index, "width", 0, true);
    const float height = luaL_tablenumber(L, index, "height", 0, true);
    if (width < 0 || height < 0) luaL_error(L, "rectangle width and height must be non-negative");
    return D2D1::RectF(x, y, x + width, y + height);
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

inline Brush *check_brush(lua_State *L, int index)
{
    auto *brush = static_cast<Brush *>(luaL_checkudata(L, index, BRUSH_MT));
    if (brush->closed) luaL_error(L, "attempt to use a closed PainterBrush");
    return brush;
}

inline Image *check_image(lua_State *L, int index)
{
    auto *image = static_cast<Image *>(luaL_checkudata(L, index, IMAGE_MT));
    if (image->closed || !image->target || !image->bitmap) luaL_error(L, "attempt to use a closed PainterImage");
    return image;
}

inline TextStyle *check_text_style(lua_State *L, int index)
{
    auto *style = static_cast<TextStyle *>(luaL_checkudata(L, index, TEXT_STYLE_MT));
    if (style->closed) luaL_error(L, "attempt to use a closed PainterTextStyle");
    return style;
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

inline UINT32 intern_brush(Painter *painter, const Brush &brush)
{
    for (UINT32 i = 0; i < painter->brushes.size(); ++i)
        if (painter->brushes[i].color == brush.color) return i;
    painter->brushes.push_back({brush.color, nullptr});
    return static_cast<UINT32>(painter->brushes.size() - 1);
}

inline void close_image(Image *image)
{
    if (image->closed) return;
    if (image->bitmap) image->bitmap->Release();
    if (image->target) image->target->Release();
    image->bitmap = nullptr;
    image->target = nullptr;
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
    for (auto &brush : painter->brushes)
        if (brush.native) brush.native->Release();
    for (auto &stroke : painter->strokes)
        if (stroke.native) stroke.native->Release();
    for (auto &format : painter->text_formats)
        if (format.native) format.native->Release();
    for (auto &image : painter->images)
        if (image.bitmap) image.bitmap->Release();
    painter->commands.clear();
    painter->geometry_payloads.clear();
    painter->image_payloads.clear();
    painter->text_payloads.clear();
    painter->brushes.clear();
    painter->strokes.clear();
    painter->text_formats.clear();
    painter->images.clear();
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

inline D2D1_RECT_F inset_rect(D2D1_RECT_F rect, float amount)
{
    rect.left += amount;
    rect.top += amount;
    rect.right -= amount;
    rect.bottom -= amount;
    if (rect.right < rect.left) rect.right = rect.left;
    if (rect.bottom < rect.top) rect.bottom = rect.top;
    return rect;
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
    if (!(result.width > 0)) luaL_error(L, "stroke width must be greater than zero");

    const auto cap = parse_cap(L, luaL_tablestring(L, index, "cap", "butt"));
    const auto join = parse_join(L, luaL_tablestring(L, index, "join", "miter"));
    const float miter = luaL_tablenumber(L, index, "miter_limit", 4);
    const float offset = luaL_tablenumber(L, index, "dash_offset", 0);
    if (!(miter > 0)) luaL_error(L, "miter_limit must be greater than zero");

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
            return entry->layout;
        }
        return nullptr;
    }

    void add(const std::wstring &text, const TextFormatResource &format, float width, float height, bool ellipsis,
        IDWriteTextLayout *layout)
    {
        const size_t hash = hash_key(text, format, width, height, ellipsis);
        TextLayoutCacheKey key{
            text, format.style, format.alignment, format.paragraph_alignment, format.wrapping, width, height, ellipsis};
        key.style.closed = false;
        m_lru.push_front({std::move(key), layout, m_generation, hash});
        m_index.emplace(hash, m_lru.begin());
        while (m_lru.size() > TEXT_LAYOUT_CACHE_CAPACITY) evict(std::prev(m_lru.end()));
    }

    void clear()
    {
        for (const auto &entry : m_lru) entry.layout->Release();
        m_index.clear();
        m_lru.clear();
        m_generation = 0;
    }

  private:
    struct Entry
    {
        TextLayoutCacheKey key{};
        IDWriteTextLayout *layout{};
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
        entry->layout->Release();
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
        key.style.closed = false;
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
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, measurement.height);
    lua_setfield(L, -2, "height");
    lua_pushinteger(L, measurement.line_count);
    lua_setfield(L, -2, "line_count");
    lua_pushnumber(L, measurement.baseline);
    lua_setfield(L, -2, "baseline");
    lua_pushboolean(L, measurement.truncated);
    lua_setfield(L, -2, "truncated");
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
    resource.style.closed = false;
    resource.alignment = alignment;
    resource.paragraph_alignment = paragraph_alignment;
    resource.wrapping = wrapping;
    painter->text_formats.push_back(std::move(resource));
    return static_cast<UINT32>(painter->text_formats.size() - 1);
}

inline UINT32 intern_image(Painter *painter, const Image &image)
{
    for (UINT32 i = 0; i < painter->images.size(); ++i)
        if (painter->images[i].bitmap == image.bitmap) return i;
    image.bitmap->AddRef();
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
        points.push_back(D2D1::Point2F(x, y));
    }
    return points;
}

inline HRESULT make_geometry(ID2D1Factory *factory, const std::vector<D2D1_POINT_2F> &points, bool closed, bool filled,
    ID2D1PathGeometry **geometry)
{
    *geometry = nullptr;
    HRESULT hr = factory ? factory->CreatePathGeometry(geometry) : E_NOINTERFACE;
    if (FAILED(hr) || !*geometry) return FAILED(hr) ? hr : E_FAIL;
    ID2D1GeometrySink *sink = nullptr;
    hr = (*geometry)->Open(&sink);
    if (SUCCEEDED(hr) && sink)
    {
        sink->SetFillMode(D2D1_FILL_MODE_WINDING);
        sink->BeginFigure(points.front(), filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
        if (points.size() > 1) sink->AddLines(points.data() + 1, static_cast<UINT32>(points.size() - 1));
        sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
        hr = sink->Close();
        sink->Release();
    }
    else if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
    }
    if (FAILED(hr))
    {
        (*geometry)->Release();
        *geometry = nullptr;
    }
    return hr;
}

inline HRESULT create_text_factory(IDWriteFactory **factory)
{
    *factory = nullptr;
    return DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown **>(factory));
}

inline HRESULT create_text_format(IDWriteFactory *factory, const TextStyle *style, IDWriteTextFormat **format)
{
    *format = nullptr;
    HRESULT hr = factory->CreateTextFormat(style->family.c_str(), nullptr, style->weight, style->slant,
        DWRITE_FONT_STRETCH_NORMAL, style->size, L"", format);
    if (FAILED(hr) || !*format) return FAILED(hr) ? hr : E_FAIL;
    if (style->has_line_height)
    {
        const float spacing = style->size * style->line_height;
        hr = (*format)->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, spacing, spacing * 0.8f);
    }
    return hr;
}

inline DWRITE_WORD_WRAPPING parse_wrap(lua_State *L, const std::string &wrap)
{
    if (wrap == "none") return DWRITE_WORD_WRAPPING_NO_WRAP;
    if (wrap == "word") return DWRITE_WORD_WRAPPING_WRAP;
    if (wrap == "character") return DWRITE_WORD_WRAPPING_CHARACTER;
    luaL_error(L, "invalid text wrap mode '%s'", wrap.c_str());
    return DWRITE_WORD_WRAPPING_WRAP;
}

inline void apply_text_style(lua_State *L, IDWriteTextLayout *layout, const TextStyle *style, UINT32 length)
{
    const DWRITE_TEXT_RANGE range{0, length};
    HRESULT hr = S_OK;
    if (style->underline) hr = layout->SetUnderline(TRUE, range);
    if (SUCCEEDED(hr) && style->strikethrough) hr = layout->SetStrikethrough(TRUE, range);
    if (FAILED(hr)) fail_hr(L, "IDWriteTextLayout text decoration", hr);
    if (style->letter_spacing != 0)
    {
        IDWriteTextLayout1 *layout1 = nullptr;
        hr = layout->QueryInterface(IID_PPV_ARGS(&layout1));
        if (FAILED(hr) || !layout1)
            fail_hr(L, "IDWriteTextLayout1 (letter spacing is unsupported by this DirectWrite version)",
                FAILED(hr) ? hr : E_NOINTERFACE);
        hr = layout1->SetCharacterSpacing(0, style->letter_spacing, 0, range);
        layout1->Release();
        if (FAILED(hr)) fail_hr(L, "IDWriteTextLayout1::SetCharacterSpacing", hr);
    }
}

inline HRESULT create_image_target(ID2D1RenderTarget *parent, UINT width, UINT height, bool clear,
    ID2D1BitmapRenderTarget **target, ID2D1Bitmap **bitmap)
{
    *target = nullptr;
    *bitmap = nullptr;
    const D2D1_SIZE_F dip_size = D2D1::SizeF(static_cast<float>(width), static_cast<float>(height));
    const D2D1_SIZE_U pixel_size = D2D1::SizeU(width, height);
    HRESULT hr = parent->CreateCompatibleRenderTarget(
        &dip_size, &pixel_size, nullptr, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, target);
    if (FAILED(hr) || !*target) return FAILED(hr) ? hr : E_FAIL;
    if (clear)
    {
        (*target)->BeginDraw();
        (*target)->Clear(D2D1::ColorF(0, 0, 0, 0));
        hr = (*target)->EndDraw();
        if (FAILED(hr))
        {
            (*target)->Release();
            *target = nullptr;
            return hr;
        }
    }
    hr = (*target)->GetBitmap(bitmap);
    if (FAILED(hr) || !*bitmap)
    {
        (*target)->Release();
        *target = nullptr;
        return FAILED(hr) ? hr : E_FAIL;
    }
    return S_OK;
}

inline Image *push_image(lua_State *L, ID2D1BitmapRenderTarget *target, ID2D1Bitmap *bitmap, UINT width, UINT height)
{
    auto *image = new (lua_newuserdata(L, sizeof(Image))) Image{};
    image->target = target;
    image->bitmap = bitmap;
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

    IWICImagingFactory *wic = nullptr;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    if (FAILED(hr) || !wic)
        return push_decode_error(L, "CoCreateInstance(WICImagingFactory)", FAILED(hr) ? hr : E_FAIL);
    IWICFormatConverter *converter = nullptr;
    hr = wic->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr) && converter)
        hr = converter->Initialize(
            source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
    wic->Release();
    if (FAILED(hr) || !converter)
    {
        if (converter) converter->Release();
        return push_decode_error(L, "WIC pixel conversion", FAILED(hr) ? hr : E_FAIL);
    }

    ID2D1RenderTarget *parent = check_current_target(L);
    ID2D1BitmapRenderTarget *target = nullptr;
    ID2D1Bitmap *bitmap = nullptr;
    hr = create_image_target(parent, width, height, false, &target, &bitmap);
    if (FAILED(hr))
    {
        converter->Release();
        return push_decode_error(L, "CreateCompatibleRenderTarget", hr);
    }
    ID2D1Bitmap *decoded = nullptr;
    hr = target->CreateBitmapFromWicBitmap(converter, nullptr, &decoded);
    converter->Release();
    if (SUCCEEDED(hr) && decoded)
    {
        target->BeginDraw();
        target->Clear(D2D1::ColorF(0, 0, 0, 0));
        target->DrawBitmap(decoded, D2D1::RectF(0, 0, static_cast<float>(width), static_cast<float>(height)), 1,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
        hr = target->EndDraw();
        decoded->Release();
    }
    if (FAILED(hr) || !decoded)
    {
        bitmap->Release();
        target->Release();
        return push_decode_error(L, "Direct2D image upload", FAILED(hr) ? hr : E_FAIL);
    }
    push_image(L, target, bitmap, width, height);
    return 1;
}

inline int decode_decoder(lua_State *L, IWICBitmapDecoder *decoder)
{
    IWICBitmapFrameDecode *frame = nullptr;
    const HRESULT hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) return push_decode_error(L, "IWICBitmapDecoder::GetFrame", FAILED(hr) ? hr : E_FAIL);
    const int result = decode_source(L, frame);
    frame->Release();
    return result;
}

inline int brush_close(lua_State *L)
{
    auto *brush = static_cast<Brush *>(luaL_checkudata(L, 1, BRUSH_MT));
    brush->closed = true;
    return 0;
}

inline int image_close(lua_State *L)
{
    auto *image = static_cast<Image *>(luaL_checkudata(L, 1, IMAGE_MT));
    if (image->painting) return luaL_error(L, "cannot close a PainterImage while it is being painted");
    close_image(image);
    return 0;
}

inline int text_style_close(lua_State *L)
{
    auto *style = static_cast<TextStyle *>(luaL_checkudata(L, 1, TEXT_STYLE_MT));
    style->closed = true;
    return 0;
}

inline int brush_gc(lua_State *L)
{
    static_cast<Brush *>(luaL_checkudata(L, 1, BRUSH_MT))->~Brush();
    return 0;
}

inline int image_gc(lua_State *L)
{
    auto *image = static_cast<Image *>(luaL_checkudata(L, 1, IMAGE_MT));
    close_image(image);
    image->~Image();
    return 0;
}

inline int text_style_gc(lua_State *L)
{
    static_cast<TextStyle *>(luaL_checkudata(L, 1, TEXT_STYLE_MT))->~TextStyle();
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
    if (strcmp(key, "width") == 0 || strcmp(key, "height") == 0)
    {
        if (image->closed || !image->target || !image->bitmap)
            return luaL_error(L, "attempt to use a closed PainterImage");
        lua_pushinteger(L, strcmp(key, "width") == 0 ? image->width : image->height);
        return 1;
    }
    luaL_getmetatable(L, IMAGE_MT);
    lua_getfield(L, -1, key);
    return 1;
}

inline UINT32 command_brush(lua_State *L, Painter *painter, int index)
{
    if (lua_type(L, index) != LUA_TUSERDATA) return intern_brush(painter, Brush{check_color(L, index), false});
    return intern_brush(painter, *check_brush(L, index));
}

inline UINT32 command_stroke(lua_State *L, Painter *painter, int index)
{
    return intern_stroke(painter, check_stroke(L, index));
}

inline int painter_clear(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    Command command{};
    command.type = CommandType::Clear;
    const auto color = check_color(L, 2);
    command.bounds = D2D1::RectF(color.r, color.g, color.b, color.a);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_fill_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    Command command{};
    command.type = CommandType::FillRect;
    command.bounds = check_rect(L, 2);
    command.brush = command_brush(L, painter, 3);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_stroke_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto rect = check_rect(L, 2);
    const auto stroke = check_stroke(L, 4);
    rect = inset_rect(rect, stroke.width * 0.5f);
    Command command{};
    command.type = CommandType::StrokeRect;
    command.bounds = rect;
    command.brush = command_brush(L, painter, 3);
    command.resource = intern_stroke(painter, stroke);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_fill_round_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    Command command{};
    command.type = CommandType::FillRoundRect;
    command.bounds = check_rect(L, 2);
    command.scalar = luaL_checkfinitenumber(L, 3, "radius");
    if (command.scalar < 0) luaL_error(L, "radius must be non-negative");
    command.scalar = std::min(command.scalar,
        std::min(command.bounds.right - command.bounds.left, command.bounds.bottom - command.bounds.top) * 0.5f);
    command.brush = command_brush(L, painter, 4);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_stroke_round_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto rect = check_rect(L, 2);
    float radius = luaL_checkfinitenumber(L, 3, "radius");
    if (radius < 0) luaL_error(L, "radius must be non-negative");
    const auto stroke = check_stroke(L, 5);
    rect = inset_rect(rect, stroke.width * 0.5f);
    radius = std::max(0.0f, radius - stroke.width * 0.5f);
    radius = std::min(radius, std::min(rect.right - rect.left, rect.bottom - rect.top) * 0.5f);
    Command command{};
    command.type = CommandType::StrokeRoundRect;
    command.bounds = rect;
    command.scalar = radius;
    command.brush = command_brush(L, painter, 4);
    command.resource = intern_stroke(painter, stroke);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_fill_ellipse(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    Command command{};
    command.type = CommandType::FillEllipse;
    command.bounds = check_rect(L, 2);
    command.brush = command_brush(L, painter, 3);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_stroke_ellipse(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto rect = check_rect(L, 2);
    const auto stroke = check_stroke(L, 4);
    rect = inset_rect(rect, stroke.width * 0.5f);
    Command command{};
    command.type = CommandType::StrokeEllipse;
    command.bounds = rect;
    command.brush = command_brush(L, painter, 3);
    command.resource = intern_stroke(painter, stroke);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_fill_circle(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const float x = luaL_checkfinitenumber(L, 2, "x");
    const float y = luaL_checkfinitenumber(L, 3, "y");
    const float radius = luaL_checkfinitenumber(L, 4, "radius");
    if (radius < 0) luaL_error(L, "radius must be non-negative");
    Command command{};
    command.type = CommandType::FillEllipse;
    command.bounds = D2D1::RectF(x - radius, y - radius, x + radius, y + radius);
    command.brush = command_brush(L, painter, 5);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_stroke_circle(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const float x = luaL_checkfinitenumber(L, 2, "x");
    const float y = luaL_checkfinitenumber(L, 3, "y");
    float radius = luaL_checkfinitenumber(L, 4, "radius");
    if (radius < 0) luaL_error(L, "radius must be non-negative");
    const auto stroke = check_stroke(L, 6);
    radius = std::max(0.0f, radius - stroke.width * 0.5f);
    Command command{};
    command.type = CommandType::StrokeEllipse;
    command.bounds = D2D1::RectF(x - radius, y - radius, x + radius, y + radius);
    command.brush = command_brush(L, painter, 5);
    command.resource = intern_stroke(painter, stroke);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_line(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    Command command{};
    command.type = CommandType::Line;
    command.bounds = D2D1::RectF(
        luaL_checkfinitenumber(L, 2, "x1"), luaL_checkfinitenumber(L, 3, "y1"), luaL_checkfinitenumber(L, 4, "x2"), luaL_checkfinitenumber(L, 5, "y2"));
    command.brush = command_brush(L, painter, 6);
    command.resource = command_stroke(L, painter, 7);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_polyline(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto points = check_points(L, 2);
    const UINT32 brush = command_brush(L, painter, 3);
    const UINT32 stroke = command_stroke(L, painter, 4);
    Command command{};
    command.type = CommandType::Polyline;
    command.brush = brush;
    command.resource = stroke;
    command.payload = static_cast<UINT32>(painter->geometry_payloads.size());
    painter->geometry_payloads.push_back({std::move(points)});
    painter->commands.push_back(command);
    return 0;
}

inline int painter_fill_polygon(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto points = check_points(L, 2);
    if (points.size() < 3) luaL_error(L, "a polygon requires at least three points");
    const UINT32 brush = command_brush(L, painter, 3);
    Command command{};
    command.type = CommandType::FillPolygon;
    command.brush = brush;
    command.payload = static_cast<UINT32>(painter->geometry_payloads.size());
    painter->geometry_payloads.push_back({std::move(points)});
    painter->commands.push_back(command);
    return 0;
}

inline int painter_stroke_polygon(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto points = check_points(L, 2);
    if (points.size() < 3) luaL_error(L, "a polygon requires at least three points");
    const UINT32 brush = command_brush(L, painter, 3);
    const UINT32 stroke = command_stroke(L, painter, 4);
    Command command{};
    command.type = CommandType::StrokePolygon;
    command.brush = brush;
    command.resource = stroke;
    command.payload = static_cast<UINT32>(painter->geometry_payloads.size());
    painter->geometry_payloads.push_back({std::move(points)});
    painter->commands.push_back(command);
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
        opacity = luaL_tablenumber(L, 4, "opacity", 1);
        if (opacity < 0 || opacity > 1) luaL_error(L, "image opacity must be in the range [0, 1]");
        const auto sampling = luaL_tablestring(L, 4, "sampling", "linear");
        if (sampling == "nearest")
            interpolation = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        else if (sampling != "linear")
            luaL_error(L, "invalid image sampling mode '%s'", sampling.c_str());
        lua_getfield(L, 4, "source");
        if (!lua_isnil(L, -1))
        {
            source = check_rect(L, -1);
            has_source = true;
        }
        lua_pop(L, 1);
        lua_getfield(L, 4, "center");
        if (!lua_isnil(L, -1))
        {
            center = check_rect(L, -1);
            nine_sliced = true;
        }
        lua_pop(L, 1);
        lua_getfield(L, 4, "tint");
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

    std::vector<ImageSlice> slices;
    slices.reserve(nine_sliced ? 9 : 1);
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
            slices.push_back({center, destination});
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
                        slices.push_back({slice_source, slice_destination});
                }
            }
        }
    }
    else
    {
        slices.push_back({source, destination});
    }

    const UINT32 image_resource = intern_image(painter, *image);
    Command command{};
    command.type = CommandType::Image;
    command.resource = image_resource;
    command.payload = static_cast<UINT32>(painter->image_payloads.size());
    painter->image_payloads.push_back({std::move(slices), tint, opacity, interpolation, tinted});
    painter->commands.push_back(command);
    return 0;
}

inline int painter_text(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto text = luaL_checkstlwstring(L, 2);
    const auto rect = check_rect(L, 3);
    const auto *style = check_text_style(L, 4);

    DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    DWRITE_PARAGRAPH_ALIGNMENT paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_WRAP;
    bool clip = true;
    std::string overflow = "clip";
    if (!lua_isnoneornil(L, 6))
    {
        luaL_checktype(L, 6, LUA_TTABLE);
        const auto align_x = luaL_tablestring(L, 6, "align_x", "left");
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
        const auto align_y = luaL_tablestring(L, 6, "align_y", "top");
        if (align_y == "top")
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        else if (align_y == "center")
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        else if (align_y == "bottom")
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
        else
            luaL_error(L, "invalid vertical text alignment '%s'", align_y.c_str());
        wrapping = parse_wrap(L, luaL_tablestring(L, 6, "wrap", "word"));
        overflow = luaL_tablestring(L, 6, "overflow", "clip");
        if (overflow != "visible" && overflow != "clip" && overflow != "ellipsis")
            luaL_error(L, "invalid text overflow mode '%s'", overflow.c_str());
        clip = luaL_tablebool(L, 6, "clip", true);
    }
    const UINT32 brush = command_brush(L, painter, 5);
    const UINT32 format = intern_text_format(painter, *style, alignment, paragraph_alignment, wrapping);
    TextPayload payload{};
    payload.text = std::move(text);
    payload.ellipsis = overflow == "ellipsis";
    if (clip && overflow != "visible") payload.options = D2D1_DRAW_TEXT_OPTIONS_CLIP;
    Command command{};
    command.type = CommandType::Text;
    command.bounds = rect;
    command.brush = brush;
    command.resource = format;
    command.payload = static_cast<UINT32>(painter->text_payloads.size());
    painter->text_payloads.push_back(std::move(payload));
    painter->commands.push_back(command);
    return 0;
}

inline int painter_push_clip(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    Command command{};
    command.type = CommandType::PushClip;
    command.bounds = check_rect(L, 2);
    painter->clips.push_back(command.bounds);
    painter->commands.push_back(std::move(command));
    return 0;
}

inline int painter_pop_clip(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    if (painter->clips.empty()) return luaL_error(L, "pop_clip called without a matching push_clip");
    painter->clips.pop_back();
    Command command{};
    command.type = CommandType::PopClip;
    painter->commands.push_back(std::move(command));
    return 0;
}

inline bool is_clip_command(CommandType type)
{
    return type == CommandType::PushClip || type == CommandType::PopClip;
}

inline bool is_provable_noop(const Painter *painter, const Command &command)
{
    const float width = command.bounds.right - command.bounds.left;
    const float height = command.bounds.bottom - command.bounds.top;
    switch (command.type)
    {
    case CommandType::FillRect:
    case CommandType::FillRoundRect:
    case CommandType::FillEllipse:
        return width == 0 || height == 0 || painter->brushes[command.brush].color.a == 0;
    case CommandType::FillPolygon:
        return painter->brushes[command.brush].color.a == 0;
    case CommandType::Text:
        return painter->brushes[command.brush].color.a == 0 || painter->text_payloads[command.payload].text.empty();
    case CommandType::Image: {
        const auto &payload = painter->image_payloads[command.payload];
        return payload.slices.empty() || payload.opacity == 0 || (payload.tinted && payload.tint.a == 0);
    }
    default:
        return false;
    }
}

inline void optimize_commands(Painter *painter)
{
    size_t latest_clear = painter->commands.size();
    for (size_t i = 0; i < painter->commands.size(); ++i)
        if (painter->commands[i].type == CommandType::Clear) latest_clear = i;

    std::vector<Command> optimized;
    std::vector<GeometryPayload> geometry_payloads;
    std::vector<ImagePayload> image_payloads;
    std::vector<TextPayload> text_payloads;
    optimized.reserve(painter->commands.size());
    geometry_payloads.reserve(painter->geometry_payloads.size());
    image_payloads.reserve(painter->image_payloads.size());
    text_payloads.reserve(painter->text_payloads.size());
    for (size_t i = 0; i < painter->commands.size(); ++i)
    {
        auto command = painter->commands[i];
        if (latest_clear != painter->commands.size() && i < latest_clear && !is_clip_command(command.type)) continue;
        if (is_provable_noop(painter, command)) continue;
        if (command.type == CommandType::Polyline || command.type == CommandType::FillPolygon ||
            command.type == CommandType::StrokePolygon)
        {
            auto &payload = painter->geometry_payloads[command.payload];
            command.payload = static_cast<UINT32>(geometry_payloads.size());
            geometry_payloads.push_back(std::move(payload));
        }
        else if (command.type == CommandType::Image)
        {
            auto &payload = painter->image_payloads[command.payload];
            command.payload = static_cast<UINT32>(image_payloads.size());
            image_payloads.push_back(std::move(payload));
        }
        else if (command.type == CommandType::Text)
        {
            auto &payload = painter->text_payloads[command.payload];
            command.payload = static_cast<UINT32>(text_payloads.size());
            text_payloads.push_back(std::move(payload));
        }
        optimized.push_back(command);
    }
    painter->commands = std::move(optimized);
    painter->geometry_payloads = std::move(geometry_payloads);
    painter->image_payloads = std::move(image_payloads);
    painter->text_payloads = std::move(text_payloads);
}

inline bool set_execution_error(std::string &error, const char *operation, HRESULT hr)
{
    error = hresult_message(operation, hr);
    return false;
}

inline bool ensure_d2d_factory(Painter *painter, ID2D1Factory **factory, std::string &error)
{
    if (*factory) return true;
    painter->target->GetFactory(factory);
    if (!*factory) return set_execution_error(error, "ID2D1RenderTarget::GetFactory", E_NOINTERFACE);
    return true;
}

inline bool realize_command_brush(Painter *painter, UINT32 index, ID2D1SolidColorBrush **brush, std::string &error)
{
    auto &resource = painter->brushes[index];
    if (!resource.native)
    {
        const HRESULT hr = painter->target->CreateSolidColorBrush(resource.color, &resource.native);
        if (FAILED(hr) || !resource.native)
            return set_execution_error(error, "ID2D1RenderTarget::CreateSolidColorBrush", FAILED(hr) ? hr : E_FAIL);
    }
    *brush = resource.native;
    return true;
}

inline bool realize_command_stroke(
    Painter *painter, UINT32 index, ID2D1Factory **factory, ID2D1StrokeStyle **style, float *width, std::string &error)
{
    auto &resource = painter->strokes[index];
    *width = resource.value.width;
    if (!resource.value.specified)
    {
        *style = nullptr;
        return true;
    }
    if (!resource.native)
    {
        if (!ensure_d2d_factory(painter, factory, error)) return false;
        const HRESULT hr = (*factory)->CreateStrokeStyle(resource.value.properties,
            resource.value.dashes.empty() ? nullptr : resource.value.dashes.data(),
            static_cast<UINT32>(resource.value.dashes.size()), &resource.native);
        if (FAILED(hr) || !resource.native)
            return set_execution_error(error, "ID2D1Factory::CreateStrokeStyle", FAILED(hr) ? hr : E_FAIL);
    }
    *style = resource.native;
    return true;
}

inline bool ensure_text_factory(IDWriteFactory **factory, std::string &error)
{
    if (*factory) return true;
    const HRESULT hr = create_text_factory(factory);
    if (FAILED(hr) || !*factory) return set_execution_error(error, "DWriteCreateFactory", FAILED(hr) ? hr : E_FAIL);
    return true;
}

inline bool realize_text_format(
    Painter *painter, UINT32 index, IDWriteFactory **factory, IDWriteTextFormat **format, std::string &error)
{
    auto &resource = painter->text_formats[index];
    if (!resource.native)
    {
        if (!ensure_text_factory(factory, error)) return false;
        HRESULT hr = create_text_format(*factory, &resource.style, &resource.native);
        if (SUCCEEDED(hr)) hr = resource.native->SetTextAlignment(resource.alignment);
        if (SUCCEEDED(hr)) hr = resource.native->SetParagraphAlignment(resource.paragraph_alignment);
        if (SUCCEEDED(hr)) hr = resource.native->SetWordWrapping(resource.wrapping);
        if (FAILED(hr) || !resource.native)
        {
            if (resource.native)
            {
                resource.native->Release();
                resource.native = nullptr;
            }
            return set_execution_error(error, "IDWriteTextFormat configuration", FAILED(hr) ? hr : E_FAIL);
        }
    }
    *format = resource.native;
    return true;
}

inline bool apply_deferred_text_style(
    IDWriteTextLayout *layout, const TextStyle &style, UINT32 length, std::string &error)
{
    const DWRITE_TEXT_RANGE range{0, length};
    HRESULT hr = S_OK;
    if (style.underline) hr = layout->SetUnderline(TRUE, range);
    if (SUCCEEDED(hr) && style.strikethrough) hr = layout->SetStrikethrough(TRUE, range);
    if (FAILED(hr)) return set_execution_error(error, "IDWriteTextLayout text decoration", hr);
    if (style.letter_spacing != 0)
    {
        IDWriteTextLayout1 *layout1 = nullptr;
        hr = layout->QueryInterface(IID_PPV_ARGS(&layout1));
        if (FAILED(hr) || !layout1)
            return set_execution_error(error,
                "IDWriteTextLayout1 (letter spacing is unsupported by this DirectWrite version)",
                FAILED(hr) ? hr : E_NOINTERFACE);
        hr = layout1->SetCharacterSpacing(0, style.letter_spacing, 0, range);
        layout1->Release();
        if (FAILED(hr)) return set_execution_error(error, "IDWriteTextLayout1::SetCharacterSpacing", hr);
    }
    return true;
}

inline bool execute_tinted_image(Painter *painter, UINT32 image, const ImagePayload &payload, std::string &error)
{
    ID2D1DeviceContext *dc = nullptr;
    HRESULT hr = painter->target->QueryInterface(IID_PPV_ARGS(&dc));
    if (FAILED(hr) || !dc)
        return set_execution_error(
            error, "image tinting requires an ID2D1DeviceContext", FAILED(hr) ? hr : E_NOINTERFACE);
    ID2D1Effect *effect = nullptr;
    hr = dc->CreateEffect(CLSID_D2D1ColorMatrix, &effect);
    if (SUCCEEDED(hr) && effect)
    {
        effect->SetInput(0, painter->images[image].bitmap);
        const float alpha = payload.tint.a * payload.opacity;
        const D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(payload.tint.r * alpha, 0, 0, 0, 0, payload.tint.g * alpha, 0,
            0, 0, 0, payload.tint.b * alpha, 0, 0, 0, 0, alpha, 0, 0, 0, 0);
        hr = effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
    }
    if (FAILED(hr) || !effect)
    {
        if (effect) effect->Release();
        dc->Release();
        return set_execution_error(error, "Direct2D color-matrix effect", FAILED(hr) ? hr : E_FAIL);
    }
    ID2D1Image *output = nullptr;
    effect->GetOutput(&output);
    if (!output)
    {
        effect->Release();
        dc->Release();
        return set_execution_error(error, "ID2D1Effect::GetOutput", E_FAIL);
    }
    D2D1_MATRIX_3X2_F old_transform{};
    dc->GetTransform(&old_transform);
    const auto interpolation = payload.interpolation == D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                                   ? D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                                   : D2D1_INTERPOLATION_MODE_LINEAR;
    for (const auto &slice : payload.slices)
    {
        const float source_width = slice.source.right - slice.source.left;
        const float source_height = slice.source.bottom - slice.source.top;
        const float sx = source_width > 0 ? (slice.destination.right - slice.destination.left) / source_width : 1;
        const float sy = source_height > 0 ? (slice.destination.bottom - slice.destination.top) / source_height : 1;
        dc->SetTransform(D2D1::Matrix3x2F::Scale(sx, sy) *
                         D2D1::Matrix3x2F::Translation(slice.destination.left, slice.destination.top) * old_transform);
        dc->DrawImage(output, D2D1::Point2F(0, 0), slice.source, interpolation, D2D1_COMPOSITE_MODE_SOURCE_OVER);
    }
    dc->SetTransform(old_transform);
    output->Release();
    effect->Release();
    dc->Release();
    return true;
}

inline bool execute_commands(Painter *painter, std::string &error)
{
    ID2D1Factory *d2d_factory = nullptr;
    IDWriteFactory *text_factory = nullptr;
    std::vector<D2D1_RECT_F> active_clips;
    bool succeeded = true;

    auto &text_layout_cache = painter->context->painter_text_layouts;
    if (!text_layout_cache) text_layout_cache = std::make_shared<TextLayoutCache>();
    text_layout_cache->begin_generation();

    for (const auto &command : painter->commands)
    {
        ID2D1SolidColorBrush *brush = nullptr;
        ID2D1StrokeStyle *stroke_style = nullptr;
        float stroke_width = 1.0f;
        if (command.type == CommandType::PushClip)
        {
            painter->target->PushAxisAlignedClip(command.bounds, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            active_clips.push_back(command.bounds);
            continue;
        }
        if (command.type == CommandType::PopClip)
        {
            painter->target->PopAxisAlignedClip();
            active_clips.pop_back();
            continue;
        }
        if (command.type == CommandType::Clear)
        {
            for (size_t i = 0; i < active_clips.size(); ++i) painter->target->PopAxisAlignedClip();
            painter->target->Clear(
                D2D1::ColorF(command.bounds.left, command.bounds.top, command.bounds.right, command.bounds.bottom));
            for (const auto &clip : active_clips)
                painter->target->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            continue;
        }
        if (command.type != CommandType::Image && !realize_command_brush(painter, command.brush, &brush, error))
        {
            succeeded = false;
            break;
        }
        const bool stroked = command.type == CommandType::StrokeRect || command.type == CommandType::StrokeRoundRect ||
                             command.type == CommandType::StrokeEllipse || command.type == CommandType::Line ||
                             command.type == CommandType::Polyline || command.type == CommandType::StrokePolygon;
        if (stroked &&
            !realize_command_stroke(painter, command.resource, &d2d_factory, &stroke_style, &stroke_width, error))
        {
            succeeded = false;
            break;
        }

        const auto ellipse = D2D1::Ellipse(D2D1::Point2F((command.bounds.left + command.bounds.right) * 0.5f,
                                               (command.bounds.top + command.bounds.bottom) * 0.5f),
            (command.bounds.right - command.bounds.left) * 0.5f, (command.bounds.bottom - command.bounds.top) * 0.5f);
        switch (command.type)
        {
        case CommandType::FillRect:
            painter->target->FillRectangle(command.bounds, brush);
            break;
        case CommandType::StrokeRect:
            painter->target->DrawRectangle(command.bounds, brush, stroke_width, stroke_style);
            break;
        case CommandType::FillRoundRect:
            painter->target->FillRoundedRectangle(
                D2D1::RoundedRect(command.bounds, command.scalar, command.scalar), brush);
            break;
        case CommandType::StrokeRoundRect:
            painter->target->DrawRoundedRectangle(
                D2D1::RoundedRect(command.bounds, command.scalar, command.scalar), brush, stroke_width, stroke_style);
            break;
        case CommandType::FillEllipse:
            painter->target->FillEllipse(ellipse, brush);
            break;
        case CommandType::StrokeEllipse:
            painter->target->DrawEllipse(ellipse, brush, stroke_width, stroke_style);
            break;
        case CommandType::Line:
            painter->target->DrawLine(D2D1::Point2F(command.bounds.left, command.bounds.top),
                D2D1::Point2F(command.bounds.right, command.bounds.bottom), brush, stroke_width, stroke_style);
            break;
        case CommandType::Polyline:
        case CommandType::FillPolygon:
        case CommandType::StrokePolygon: {
            if (!ensure_d2d_factory(painter, &d2d_factory, error))
            {
                succeeded = false;
                break;
            }
            ID2D1PathGeometry *geometry = nullptr;
            const bool closed = command.type != CommandType::Polyline;
            const bool filled = command.type == CommandType::FillPolygon;
            const HRESULT hr = make_geometry(
                d2d_factory, painter->geometry_payloads[command.payload].points, closed, filled, &geometry);
            if (FAILED(hr) || !geometry)
            {
                set_execution_error(error, "Direct2D path geometry creation", FAILED(hr) ? hr : E_FAIL);
                succeeded = false;
                break;
            }
            if (filled)
                painter->target->FillGeometry(geometry, brush);
            else
                painter->target->DrawGeometry(geometry, brush, stroke_width, stroke_style);
            geometry->Release();
            break;
        }
        case CommandType::Image: {
            const auto &payload = painter->image_payloads[command.payload];
            if (!payload.tinted)
            {
                for (const auto &slice : payload.slices)
                    painter->target->DrawBitmap(painter->images[command.resource].bitmap, slice.destination,
                        payload.opacity, payload.interpolation, slice.source);
            }
            else if (!execute_tinted_image(painter, command.resource, payload, error))
            {
                succeeded = false;
            }
            break;
        }
        case CommandType::Text: {
            const auto &payload = painter->text_payloads[command.payload];
            const auto &format_resource = painter->text_formats[command.resource];
            const float width = command.bounds.right - command.bounds.left;
            const float height = command.bounds.bottom - command.bounds.top;
            IDWriteTextLayout *layout =
                text_layout_cache->get(payload.text, format_resource, width, height, payload.ellipsis);
            const bool cache_miss = layout == nullptr;
            if (cache_miss)
            {
                IDWriteTextFormat *format = nullptr;
                if (!realize_text_format(painter, command.resource, &text_factory, &format, error))
                {
                    succeeded = false;
                    break;
                }
                const UINT32 length = static_cast<UINT32>(std::min<size_t>(payload.text.size(), UINT32_MAX));
                HRESULT hr =
                    text_factory->CreateTextLayout(payload.text.data(), length, format, width, height, &layout);
                if (FAILED(hr) || !layout)
                {
                    set_execution_error(error, "DirectWrite text layout", FAILED(hr) ? hr : E_FAIL);
                    succeeded = false;
                }
                if (succeeded) succeeded = apply_deferred_text_style(layout, format_resource.style, length, error);
                if (succeeded && payload.ellipsis)
                {
                    IDWriteInlineObject *ellipsis = nullptr;
                    hr = text_factory->CreateEllipsisTrimmingSign(format, &ellipsis);
                    if (SUCCEEDED(hr) && ellipsis)
                    {
                        const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
                        hr = layout->SetTrimming(&trimming, ellipsis);
                        ellipsis->Release();
                    }
                    if (FAILED(hr))
                    {
                        set_execution_error(error, "DirectWrite ellipsis trimming", hr);
                        succeeded = false;
                    }
                }
                if (succeeded)
                    text_layout_cache->add(payload.text, format_resource, width, height, payload.ellipsis, layout);
            }
            if (succeeded)
                painter->target->DrawTextLayout(
                    D2D1::Point2F(command.bounds.left, command.bounds.top), layout, brush, payload.options);
            if (!succeeded && cache_miss && layout) layout->Release();
            break;
        }
        default:
            break;
        }
        if (!succeeded) break;
    }

    while (!active_clips.empty())
    {
        painter->target->PopAxisAlignedClip();
        active_clips.pop_back();
    }
    if (text_factory) text_factory->Release();
    if (d2d_factory) d2d_factory->Release();
    return succeeded;
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
    std::string execution_error;
    if (status == LUA_OK)
    {
        optimize_commands(painter);
        execute_commands(painter, execution_error);
    }
    invalidate_painter(painter);
    const HRESULT hr = target->EndDraw();

    if (status != LUA_OK) return lua_error(L);
    if (!execution_error.empty()) return luaL_error(L, "%s", execution_error.c_str());
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
    context->d2d_render_target_stack.push(image->target);
    lua_pushvalue(L, 2);
    auto *painter = push_painter(L, context, image->target);
    const int status = lua_pcall(L, 1, 0, 0);
    std::string execution_error;
    if (status == LUA_OK)
    {
        optimize_commands(painter);
        execute_commands(painter, execution_error);
    }
    invalidate_painter(painter);
    context->d2d_render_target_stack.pop();
    const HRESULT hr = image->target->EndDraw();
    image->painting = false;
    if (status != LUA_OK) return lua_error(L);
    if (!execution_error.empty()) return luaL_error(L, "%s", execution_error.c_str());
    if (FAILED(hr)) return fail_hr(L, "ID2D1BitmapRenderTarget::EndDraw", hr);
    return 0;
}

} // namespace Detail

inline int brush(lua_State *L)
{
    const auto color = Detail::check_color(L, 1);
    auto *brush = new (lua_newuserdata(L, sizeof(Detail::Brush))) Detail::Brush{};
    brush->color = color;
    luaL_getmetatable(L, Detail::BRUSH_MT);
    lua_setmetatable(L, -2);
    return 1;
}

inline int text_style(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    auto *style = new (lua_newuserdata(L, sizeof(Detail::TextStyle))) Detail::TextStyle{};
    luaL_getmetatable(L, Detail::TEXT_STYLE_MT);
    lua_setmetatable(L, -2);

    lua_getfield(L, 1, "family");
    if (lua_isstring(L, -1))
    {
        style->family = luaL_checkstlwstring(L, -1);
    }
    else if (lua_istable(L, -1))
    {
        const size_t count = lua_rawlen(L, -1);
        for (size_t i = 0; i < count; ++i)
        {
            lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));
            if (lua_isstring(L, -1))
            {
                const auto candidate = luaL_checkstlwstring(L, -1);
                if (!candidate.empty() && style->family == L"Segoe UI") style->family = candidate;
            }
            else
            {
                luaL_error(L, "font family list entries must be strings");
            }
            lua_pop(L, 1);
        }
    }
    else if (!lua_isnil(L, -1))
    {
        luaL_error(L, "font family must be a string or an array of strings");
    }
    lua_pop(L, 1);
    if (style->family.empty()) style->family = L"Segoe UI";

    style->size = luaL_tablenumber(L, 1, "size", 12);
    if (!(style->size > 0)) luaL_error(L, "font size must be greater than zero");
    const float weight = luaL_tablenumber(L, 1, "weight", 400);
    if (weight < 1 || weight > 1000 || std::floor(weight) != weight)
        luaL_error(L, "font weight must be an integer from 1 through 1000");
    style->weight = static_cast<DWRITE_FONT_WEIGHT>(static_cast<int>(weight));
    const auto slant = luaL_tablestring(L, 1, "slant", "normal");
    if (slant == "normal")
        style->slant = DWRITE_FONT_STYLE_NORMAL;
    else if (slant == "italic")
        style->slant = DWRITE_FONT_STYLE_ITALIC;
    else if (slant == "oblique")
        style->slant = DWRITE_FONT_STYLE_OBLIQUE;
    else
        luaL_error(L, "invalid font slant '%s'", slant.c_str());
    style->underline = luaL_tablebool(L, 1, "underline", false);
    style->strikethrough = luaL_tablebool(L, 1, "strikethrough", false);
    style->letter_spacing = luaL_tablenumber(L, 1, "letter_spacing", 0);
    lua_getfield(L, 1, "line_height");
    if (!lua_isnil(L, -1))
    {
        style->line_height = luaL_checkfinitenumber(L, -1, "line_height");
        if (!(style->line_height > 0)) luaL_error(L, "line_height must be greater than zero");
        style->has_line_height = true;
    }
    lua_pop(L, 1);
    return 1;
}

inline int new_image(lua_State *L)
{
    const lua_Integer width_value = luaL_checkinteger(L, 1);
    const lua_Integer height_value = luaL_checkinteger(L, 2);
    if (width_value <= 0 || height_value <= 0 || width_value > UINT_MAX || height_value > UINT_MAX)
        return luaL_error(L, "image dimensions must be positive 32-bit integers");
    auto *parent = Detail::check_current_target(L);
    ID2D1BitmapRenderTarget *target = nullptr;
    ID2D1Bitmap *bitmap = nullptr;
    const HRESULT hr = Detail::create_image_target(
        parent, static_cast<UINT>(width_value), static_cast<UINT>(height_value), true, &target, &bitmap);
    if (FAILED(hr)) return Detail::fail_hr(L, "CreateCompatibleRenderTarget", hr);
    Detail::push_image(L, target, bitmap, static_cast<UINT>(width_value), static_cast<UINT>(height_value));
    return 1;
}

inline int load_image(lua_State *L)
{
    const auto path = luaL_checkstlwstring(L, 1);
    IWICImagingFactory *wic = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    if (FAILED(hr) || !wic)
        return Detail::push_decode_error(L, "CoCreateInstance(WICImagingFactory)", FAILED(hr) ? hr : E_FAIL);
    IWICBitmapDecoder *decoder = nullptr;
    hr = wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    wic->Release();
    if (FAILED(hr) || !decoder)
        return Detail::push_decode_error(L, "WIC image file decoding", FAILED(hr) ? hr : E_FAIL);
    const int result = Detail::decode_decoder(L, decoder);
    decoder->Release();
    return result;
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
    IStream *stream = SHCreateMemStream(reinterpret_cast<const BYTE *>(data), static_cast<UINT>(size));
    if (!stream) return Detail::push_decode_error(L, "SHCreateMemStream", E_OUTOFMEMORY);
    IWICImagingFactory *wic = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    IWICBitmapDecoder *decoder = nullptr;
    if (SUCCEEDED(hr) && wic)
        hr = wic->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder);
    stream->Release();
    if (wic) wic->Release();
    if (FAILED(hr) || !decoder)
        return Detail::push_decode_error(L, "WIC memory image decoding", FAILED(hr) ? hr : E_FAIL);
    const int result = Detail::decode_decoder(L, decoder);
    decoder->Release();
    return result;
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
    auto *style = Detail::check_text_style(L, 2);
    float width = Detail::MAX_LAYOUT_SIZE;
    float height = Detail::MAX_LAYOUT_SIZE;
    bool has_width = false;
    bool has_height = false;
    lua_Integer max_lines = 0;
    std::string wrap = "none";
    if (!lua_isnoneornil(L, 3))
    {
        luaL_checktype(L, 3, LUA_TTABLE);
        lua_getfield(L, 3, "width");
        if (!lua_isnil(L, -1))
        {
            width = luaL_checkfinitenumber(L, -1, "width");
            if (width < 0) luaL_error(L, "text constraint width must be non-negative");
            has_width = true;
        }
        lua_pop(L, 1);
        lua_getfield(L, 3, "height");
        if (!lua_isnil(L, -1))
        {
            height = luaL_checkfinitenumber(L, -1, "height");
            if (height < 0) luaL_error(L, "text constraint height must be non-negative");
            has_height = true;
        }
        lua_pop(L, 1);
        wrap = luaL_tablestring(L, 3, "wrap", has_width ? "word" : "none");
        lua_getfield(L, 3, "max_lines");
        if (!lua_isnil(L, -1))
        {
            max_lines = luaL_checkinteger(L, -1);
            if (max_lines <= 0) luaL_error(L, "max_lines must be greater than zero");
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
        measurement_cache->get(text, *style, width, height, max_lines, wrapping, has_width, has_height, &measurement))
    {
        Detail::push_text_measurement(L, measurement);
        return 1;
    }

    IDWriteFactory *factory = nullptr;
    HRESULT hr = Detail::create_text_factory(&factory);
    if (FAILED(hr) || !factory) return Detail::fail_hr(L, "DWriteCreateFactory", FAILED(hr) ? hr : E_FAIL);
    IDWriteTextFormat *format = nullptr;
    hr = Detail::create_text_format(factory, style, &format);
    if (SUCCEEDED(hr)) hr = format->SetWordWrapping(wrapping);
    IDWriteTextLayout *layout = nullptr;
    const UINT32 length = static_cast<UINT32>(std::min<size_t>(text.size(), UINT32_MAX));
    if (SUCCEEDED(hr))
        hr = factory->CreateTextLayout(text.data(), length, format, width, Detail::MAX_LAYOUT_SIZE, &layout);
    if (SUCCEEDED(hr) && layout) Detail::apply_text_style(L, layout, style, length);
    if (format) format->Release();
    factory->Release();
    if (FAILED(hr) || !layout)
    {
        if (layout) layout->Release();
        return Detail::fail_hr(L, "DirectWrite text measurement", FAILED(hr) ? hr : E_FAIL);
    }

    DWRITE_TEXT_METRICS metrics{};
    hr = layout->GetMetrics(&metrics);
    UINT32 line_count{};
    layout->GetLineMetrics(nullptr, 0, &line_count);
    std::vector<DWRITE_LINE_METRICS> lines(line_count);
    if (line_count) hr = layout->GetLineMetrics(lines.data(), line_count, &line_count);
    layout->Release();
    if (FAILED(hr)) return Detail::fail_hr(L, "IDWriteTextLayout::GetMetrics", hr);

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
        measurement_cache->add(text, *style, width, height, max_lines, wrapping, has_width, has_height, measurement);
    Detail::push_text_measurement(L, measurement);
    return 1;
}

inline void register_types(lua_State *L)
{
    static const luaL_Reg brush_methods[] = {{"close", Detail::brush_close}, {nullptr, nullptr}};
    static const luaL_Reg image_methods[] = {
        {"paint", Detail::image_paint}, {"close", Detail::image_close}, {nullptr, nullptr}};
    static const luaL_Reg text_style_methods[] = {{"close", Detail::text_style_close}, {nullptr, nullptr}};
    static const luaL_Reg painter_methods[] = {{"clear", Detail::painter_clear},
        {"fill_rect", Detail::painter_fill_rect}, {"stroke_rect", Detail::painter_stroke_rect},
        {"fill_round_rect", Detail::painter_fill_round_rect}, {"stroke_round_rect", Detail::painter_stroke_round_rect},
        {"fill_ellipse", Detail::painter_fill_ellipse}, {"stroke_ellipse", Detail::painter_stroke_ellipse},
        {"fill_circle", Detail::painter_fill_circle}, {"stroke_circle", Detail::painter_stroke_circle},
        {"line", Detail::painter_line}, {"polyline", Detail::painter_polyline},
        {"fill_polygon", Detail::painter_fill_polygon}, {"stroke_polygon", Detail::painter_stroke_polygon},
        {"image", Detail::painter_image}, {"text", Detail::painter_text}, {"push_clip", Detail::painter_push_clip},
        {"pop_clip", Detail::painter_pop_clip}, {nullptr, nullptr}};
    luaL_create_metatable(L, Detail::BRUSH_MT, brush_methods, nullptr, Detail::brush_gc);
    luaL_create_metatable(L, Detail::IMAGE_MT, image_methods, Detail::image_index, Detail::image_gc);
    luaL_create_metatable(L, Detail::TEXT_STYLE_MT, text_style_methods, nullptr, Detail::text_style_gc);
    luaL_create_metatable(L, Detail::PAINTER_MT, painter_methods, nullptr, Detail::painter_gc);
}

inline int invoke_paint_callback(lua_State *L)
{
    lua_pushcfunction(L, Detail::screen_paint);
    lua_insert(L, -2);
    return lua_pcall(L, 1, 0, 0);
}
} // namespace LuaCore::Painter
