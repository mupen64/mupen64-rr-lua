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
#include <new>
#include <string>
#include <vector>

namespace LuaCore::Painter
{
namespace Detail
{
constexpr const char *BRUSH_MT = "mupen64.PainterBrush";
constexpr const char *IMAGE_MT = "mupen64.PainterImage";
constexpr const char *TEXT_STYLE_MT = "mupen64.PainterTextStyle";
constexpr const char *PAINTER_MT = "mupen64.Painter";
constexpr float MAX_LAYOUT_SIZE = 10000000.0f;

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

struct Painter
{
    ID2D1RenderTarget *target{};
    LuaRenderingContext *context{};
    std::vector<D2D1_RECT_F> clips{};
    bool active{};
};

struct Stroke
{
    float width{1.0f};
    ID2D1StrokeStyle *style{};
};

inline std::string hresult_message(const char *operation, HRESULT hr)
{
    char *system_message = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
        static_cast<DWORD>(hr), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char *>(&system_message), 0,
        nullptr);
    std::string result(operation);
    result += " failed (HRESULT 0x";
    char code[16]{};
    snprintf(code, sizeof(code), "%08lX", static_cast<unsigned long>(hr));
    result += code;
    result += ")";
    if (system_message)
    {
        result += ": ";
        result += system_message;
        while (!result.empty() && (result.back() == '\r' || result.back() == '\n')) result.pop_back();
        LocalFree(system_message);
    }
    return result;
}

inline int fail_hr(lua_State *L, const char *operation, HRESULT hr)
{
    const auto message = hresult_message(operation, hr);
    return luaL_error(L, "%s", message.c_str());
}

inline float finite_number(lua_State *L, int index, const char *name)
{
    const auto value = static_cast<float>(luaL_checknumber(L, index));
    if (!std::isfinite(value)) luaL_error(L, "%s must be finite", name);
    return value;
}

inline float table_number(lua_State *L, int table, const char *field, float fallback, bool required = false)
{
    table = lua_absindex(L, table);
    lua_getfield(L, table, field);
    float result = fallback;
    if (lua_isnil(L, -1))
    {
        if (required) luaL_error(L, "field '%s' is required", field);
    }
    else
    {
        result = finite_number(L, -1, field);
    }
    lua_pop(L, 1);
    return result;
}

inline bool table_bool(lua_State *L, int table, const char *field, bool fallback)
{
    table = lua_absindex(L, table);
    lua_getfield(L, table, field);
    const bool result = lua_isnil(L, -1) ? fallback : lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return result;
}

inline std::string table_string(lua_State *L, int table, const char *field, const char *fallback)
{
    table = lua_absindex(L, table);
    lua_getfield(L, table, field);
    std::string result = fallback;
    if (!lua_isnil(L, -1)) result = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    return result;
}

inline D2D1_COLOR_F check_color(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    const float r = table_number(L, index, "r", 0, true);
    const float g = table_number(L, index, "g", 0, true);
    const float b = table_number(L, index, "b", 0, true);
    const float a = table_number(L, index, "a", 1);
    if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1 || a < 0 || a > 1)
        luaL_error(L, "color components must be in the range [0, 1]");
    return D2D1::ColorF(r, g, b, a);
}

inline D2D1_RECT_F check_rect(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    const float x = table_number(L, index, "x", 0, true);
    const float y = table_number(L, index, "y", 0, true);
    const float width = table_number(L, index, "width", 0, true);
    const float height = table_number(L, index, "height", 0, true);
    if (width < 0 || height < 0) luaL_error(L, "rectangle width and height must be non-negative");
    return D2D1::RectF(x, y, x + width, y + height);
}

inline std::wstring utf8_to_wide(lua_State *L, int index)
{
    size_t length{};
    const char *text = luaL_checklstring(L, index, &length);
    if (!length) return {};
    if (length > static_cast<size_t>(INT_MAX)) luaL_error(L, "string is too large");
    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, static_cast<int>(length), nullptr, 0);
    if (required <= 0) luaL_error(L, "string is not valid UTF-8");
    std::wstring result(static_cast<size_t>(required), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, static_cast<int>(length), result.data(), required))
        luaL_error(L, "failed to convert UTF-8 string");
    return result;
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

inline ID2D1SolidColorBrush *realize_brush(lua_State *L, Painter *painter, int brush_index)
{
    const auto *brush = check_brush(L, brush_index);
    ID2D1SolidColorBrush *native = nullptr;
    const HRESULT hr = painter->target->CreateSolidColorBrush(brush->color, &native);
    if (FAILED(hr) || !native) fail_hr(L, "ID2D1RenderTarget::CreateSolidColorBrush", FAILED(hr) ? hr : E_FAIL);
    return native;
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

inline void invalidate_painter(Painter *painter)
{
    if (!painter || !painter->active) return;
    while (!painter->clips.empty())
    {
        painter->target->PopAxisAlignedClip();
        painter->clips.pop_back();
    }
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

inline Stroke check_stroke(lua_State *L, Painter *painter, int index)
{
    Stroke result{};
    if (lua_isnoneornil(L, index)) return result;
    luaL_checktype(L, index, LUA_TTABLE);
    result.width = table_number(L, index, "width", 1);
    if (!(result.width > 0)) luaL_error(L, "stroke width must be greater than zero");

    const auto cap = parse_cap(L, table_string(L, index, "cap", "butt"));
    const auto join = parse_join(L, table_string(L, index, "join", "miter"));
    const float miter = table_number(L, index, "miter_limit", 4);
    const float offset = table_number(L, index, "dash_offset", 0);
    if (!(miter > 0)) luaL_error(L, "miter_limit must be greater than zero");

    std::vector<float> dashes;
    const int absolute = lua_absindex(L, index);
    lua_getfield(L, absolute, "dashes");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        const size_t count = lua_rawlen(L, -1);
        dashes.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));
            const float dash = finite_number(L, -1, "dash length");
            if (!(dash > 0)) luaL_error(L, "dash lengths must be greater than zero");
            dashes.push_back(dash / result.width);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    D2D1_STROKE_STYLE_PROPERTIES properties = D2D1::StrokeStyleProperties(cap, cap, cap, join, miter,
        dashes.empty() ? D2D1_DASH_STYLE_SOLID : D2D1_DASH_STYLE_CUSTOM, offset / result.width);
    ID2D1Factory *factory = nullptr;
    painter->target->GetFactory(&factory);
    if (!factory) luaL_error(L, "Direct2D target has no factory");
    const HRESULT hr = factory->CreateStrokeStyle(
        properties, dashes.empty() ? nullptr : dashes.data(), static_cast<UINT32>(dashes.size()), &result.style);
    factory->Release();
    if (FAILED(hr) || !result.style) fail_hr(L, "ID2D1Factory::CreateStrokeStyle", FAILED(hr) ? hr : E_FAIL);
    return result;
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
        const float x = finite_number(L, -1, "point x");
        lua_pop(L, 1);
        lua_rawgeti(L, index, static_cast<lua_Integer>(i + 2));
        const float y = finite_number(L, -1, "point y");
        lua_pop(L, 1);
        points.push_back(D2D1::Point2F(x, y));
    }
    return points;
}

inline ID2D1PathGeometry *make_geometry(
    lua_State *L, Painter *painter, const std::vector<D2D1_POINT_2F> &points, bool closed, bool filled)
{
    ID2D1Factory *factory = nullptr;
    painter->target->GetFactory(&factory);
    if (!factory) luaL_error(L, "Direct2D target has no factory");
    ID2D1PathGeometry *geometry = nullptr;
    HRESULT hr = factory->CreatePathGeometry(&geometry);
    factory->Release();
    if (FAILED(hr) || !geometry) fail_hr(L, "ID2D1Factory::CreatePathGeometry", FAILED(hr) ? hr : E_FAIL);
    ID2D1GeometrySink *sink = nullptr;
    hr = geometry->Open(&sink);
    if (FAILED(hr) || !sink)
    {
        geometry->Release();
        fail_hr(L, "ID2D1PathGeometry::Open", FAILED(hr) ? hr : E_FAIL);
    }
    sink->SetFillMode(D2D1_FILL_MODE_WINDING);
    sink->BeginFigure(points.front(), filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
    if (points.size() > 1) sink->AddLines(points.data() + 1, static_cast<UINT32>(points.size() - 1));
    sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
    hr = sink->Close();
    sink->Release();
    if (FAILED(hr))
    {
        geometry->Release();
        fail_hr(L, "ID2D1GeometrySink::Close", hr);
    }
    return geometry;
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

inline int painter_clear(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto color = check_color(L, 2);
    const auto clips = painter->clips;
    for (size_t i = 0; i < clips.size(); ++i) painter->target->PopAxisAlignedClip();
    painter->target->Clear(color);
    for (const auto &clip : clips) painter->target->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    return 0;
}

inline int painter_fill_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto rect = check_rect(L, 2);
    auto *native = realize_brush(L, painter, 3);
    painter->target->FillRectangle(rect, native);
    native->Release();
    return 0;
}

inline int painter_stroke_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto rect = check_rect(L, 2);
    auto *native = realize_brush(L, painter, 3);
    auto stroke = check_stroke(L, painter, 4);
    rect = inset_rect(rect, stroke.width * 0.5f);
    painter->target->DrawRectangle(rect, native, stroke.width, stroke.style);
    if (stroke.style) stroke.style->Release();
    native->Release();
    return 0;
}

inline int painter_fill_round_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto rect = check_rect(L, 2);
    float radius = finite_number(L, 3, "radius");
    if (radius < 0) luaL_error(L, "radius must be non-negative");
    radius = std::min(radius, std::min(rect.right - rect.left, rect.bottom - rect.top) * 0.5f);
    const D2D1_ROUNDED_RECT rounded{rect, radius, radius};
    auto *native = realize_brush(L, painter, 4);
    painter->target->FillRoundedRectangle(rounded, native);
    native->Release();
    return 0;
}

inline int painter_stroke_round_rect(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto rect = check_rect(L, 2);
    float radius = finite_number(L, 3, "radius");
    if (radius < 0) luaL_error(L, "radius must be non-negative");
    auto *native = realize_brush(L, painter, 4);
    auto stroke = check_stroke(L, painter, 5);
    rect = inset_rect(rect, stroke.width * 0.5f);
    radius = std::max(0.0f, radius - stroke.width * 0.5f);
    radius = std::min(radius, std::min(rect.right - rect.left, rect.bottom - rect.top) * 0.5f);
    const D2D1_ROUNDED_RECT rounded{rect, radius, radius};
    painter->target->DrawRoundedRectangle(rounded, native, stroke.width, stroke.style);
    if (stroke.style) stroke.style->Release();
    native->Release();
    return 0;
}

inline int painter_fill_ellipse(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto rect = check_rect(L, 2);
    const D2D1_ELLIPSE ellipse{D2D1::Point2F((rect.left + rect.right) * 0.5f, (rect.top + rect.bottom) * 0.5f),
        (rect.right - rect.left) * 0.5f, (rect.bottom - rect.top) * 0.5f};
    auto *native = realize_brush(L, painter, 3);
    painter->target->FillEllipse(ellipse, native);
    native->Release();
    return 0;
}

inline int painter_stroke_ellipse(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    auto rect = check_rect(L, 2);
    auto *native = realize_brush(L, painter, 3);
    auto stroke = check_stroke(L, painter, 4);
    rect = inset_rect(rect, stroke.width * 0.5f);
    const D2D1_ELLIPSE ellipse{D2D1::Point2F((rect.left + rect.right) * 0.5f, (rect.top + rect.bottom) * 0.5f),
        (rect.right - rect.left) * 0.5f, (rect.bottom - rect.top) * 0.5f};
    painter->target->DrawEllipse(ellipse, native, stroke.width, stroke.style);
    if (stroke.style) stroke.style->Release();
    native->Release();
    return 0;
}

inline int painter_fill_circle(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const float x = finite_number(L, 2, "x");
    const float y = finite_number(L, 3, "y");
    const float radius = finite_number(L, 4, "radius");
    if (radius < 0) luaL_error(L, "radius must be non-negative");
    auto *native = realize_brush(L, painter, 5);
    painter->target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), native);
    native->Release();
    return 0;
}

inline int painter_stroke_circle(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const float x = finite_number(L, 2, "x");
    const float y = finite_number(L, 3, "y");
    float radius = finite_number(L, 4, "radius");
    if (radius < 0) luaL_error(L, "radius must be non-negative");
    auto *native = realize_brush(L, painter, 5);
    auto stroke = check_stroke(L, painter, 6);
    radius = std::max(0.0f, radius - stroke.width * 0.5f);
    painter->target->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), native, stroke.width, stroke.style);
    if (stroke.style) stroke.style->Release();
    native->Release();
    return 0;
}

inline int painter_line(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto a = D2D1::Point2F(finite_number(L, 2, "x1"), finite_number(L, 3, "y1"));
    const auto b = D2D1::Point2F(finite_number(L, 4, "x2"), finite_number(L, 5, "y2"));
    auto *native = realize_brush(L, painter, 6);
    auto stroke = check_stroke(L, painter, 7);
    painter->target->DrawLine(a, b, native, stroke.width, stroke.style);
    if (stroke.style) stroke.style->Release();
    native->Release();
    return 0;
}

inline int painter_polyline(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto points = check_points(L, 2);
    auto *native = realize_brush(L, painter, 3);
    auto stroke = check_stroke(L, painter, 4);
    auto *geometry = make_geometry(L, painter, points, false, false);
    painter->target->DrawGeometry(geometry, native, stroke.width, stroke.style);
    geometry->Release();
    if (stroke.style) stroke.style->Release();
    native->Release();
    return 0;
}

inline int painter_fill_polygon(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto points = check_points(L, 2);
    if (points.size() < 3) luaL_error(L, "a polygon requires at least three points");
    auto *native = realize_brush(L, painter, 3);
    auto *geometry = make_geometry(L, painter, points, true, true);
    painter->target->FillGeometry(geometry, native);
    geometry->Release();
    native->Release();
    return 0;
}

inline int painter_stroke_polygon(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto points = check_points(L, 2);
    if (points.size() < 3) luaL_error(L, "a polygon requires at least three points");
    auto *native = realize_brush(L, painter, 3);
    auto stroke = check_stroke(L, painter, 4);
    auto *geometry = make_geometry(L, painter, points, true, false);
    painter->target->DrawGeometry(geometry, native, stroke.width, stroke.style);
    geometry->Release();
    if (stroke.style) stroke.style->Release();
    native->Release();
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
        opacity = table_number(L, 4, "opacity", 1);
        if (opacity < 0 || opacity > 1) luaL_error(L, "image opacity must be in the range [0, 1]");
        const auto sampling = table_string(L, 4, "sampling", "linear");
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

    struct ImageSlice
    {
        D2D1_RECT_F source;
        D2D1_RECT_F destination;
    };
    std::vector<ImageSlice> slices;
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

    if (!tinted)
    {
        for (const auto &slice : slices)
            painter->target->DrawBitmap(image->bitmap, slice.destination, opacity, interpolation, slice.source);
        return 0;
    }

    ID2D1DeviceContext *dc = nullptr;
    HRESULT hr = painter->target->QueryInterface(IID_PPV_ARGS(&dc));
    if (FAILED(hr) || !dc)
        return fail_hr(L, "image tinting requires an ID2D1DeviceContext", FAILED(hr) ? hr : E_NOINTERFACE);
    ID2D1Effect *effect = nullptr;
    hr = dc->CreateEffect(CLSID_D2D1ColorMatrix, &effect);
    if (SUCCEEDED(hr) && effect)
    {
        effect->SetInput(0, image->bitmap);
        const float alpha = tint.a * opacity;
        const D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
            tint.r * alpha, 0, 0, 0, 0, tint.g * alpha, 0, 0, 0, 0, tint.b * alpha, 0, 0, 0, 0, alpha, 0, 0, 0, 0);
        hr = effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
    }
    if (FAILED(hr) || !effect)
    {
        if (effect) effect->Release();
        dc->Release();
        return fail_hr(L, "Direct2D color-matrix effect", FAILED(hr) ? hr : E_FAIL);
    }
    ID2D1Image *output = nullptr;
    effect->GetOutput(&output);
    if (!output)
    {
        effect->Release();
        dc->Release();
        return fail_hr(L, "ID2D1Effect::GetOutput", E_FAIL);
    }
    D2D1_MATRIX_3X2_F old_transform{};
    dc->GetTransform(&old_transform);
    const auto effect_interpolation = interpolation == D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                                          ? D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                                          : D2D1_INTERPOLATION_MODE_LINEAR;
    for (const auto &slice : slices)
    {
        const float source_width = slice.source.right - slice.source.left;
        const float source_height = slice.source.bottom - slice.source.top;
        const float sx = source_width > 0 ? (slice.destination.right - slice.destination.left) / source_width : 1;
        const float sy = source_height > 0 ? (slice.destination.bottom - slice.destination.top) / source_height : 1;
        dc->SetTransform(D2D1::Matrix3x2F::Scale(sx, sy) *
                         D2D1::Matrix3x2F::Translation(slice.destination.left, slice.destination.top) * old_transform);
        dc->DrawImage(output, D2D1::Point2F(0, 0), slice.source, effect_interpolation, D2D1_COMPOSITE_MODE_SOURCE_OVER);
    }
    dc->SetTransform(old_transform);
    output->Release();
    effect->Release();
    dc->Release();
    return 0;
}

inline int painter_text(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto text = utf8_to_wide(L, 2);
    const auto rect = check_rect(L, 3);
    auto *style = check_text_style(L, 4);
    auto *native = realize_brush(L, painter, 5);

    IDWriteFactory *factory = nullptr;
    HRESULT hr = create_text_factory(&factory);
    if (FAILED(hr) || !factory)
    {
        native->Release();
        return fail_hr(L, "DWriteCreateFactory", FAILED(hr) ? hr : E_FAIL);
    }
    IDWriteTextFormat *format = nullptr;
    hr = create_text_format(factory, style, &format);
    if (FAILED(hr) || !format)
    {
        factory->Release();
        native->Release();
        return fail_hr(L, "IDWriteFactory::CreateTextFormat", FAILED(hr) ? hr : E_FAIL);
    }

    bool clip = true;
    std::string overflow = "clip";
    if (!lua_isnoneornil(L, 6))
    {
        luaL_checktype(L, 6, LUA_TTABLE);
        const auto align_x = table_string(L, 6, "align_x", "left");
        if (align_x == "left")
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        else if (align_x == "center")
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        else if (align_x == "right")
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        else if (align_x == "justify")
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
        else
            luaL_error(L, "invalid horizontal text alignment '%s'", align_x.c_str());
        const auto align_y = table_string(L, 6, "align_y", "top");
        if (align_y == "top")
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        else if (align_y == "center")
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        else if (align_y == "bottom")
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        else
            luaL_error(L, "invalid vertical text alignment '%s'", align_y.c_str());
        hr = format->SetWordWrapping(parse_wrap(L, table_string(L, 6, "wrap", "word")));
        overflow = table_string(L, 6, "overflow", "clip");
        if (overflow != "visible" && overflow != "clip" && overflow != "ellipsis")
            luaL_error(L, "invalid text overflow mode '%s'", overflow.c_str());
        clip = table_bool(L, 6, "clip", true);
    }
    if (FAILED(hr))
    {
        format->Release();
        factory->Release();
        native->Release();
        return fail_hr(L, "IDWriteTextFormat configuration", hr);
    }

    IDWriteTextLayout *layout = nullptr;
    const UINT32 length = static_cast<UINT32>(std::min<size_t>(text.size(), UINT32_MAX));
    hr =
        factory->CreateTextLayout(text.data(), length, format, rect.right - rect.left, rect.bottom - rect.top, &layout);
    if (SUCCEEDED(hr) && layout) apply_text_style(L, layout, style, length);
    if (SUCCEEDED(hr) && overflow == "ellipsis")
    {
        IDWriteInlineObject *ellipsis = nullptr;
        hr = factory->CreateEllipsisTrimmingSign(format, &ellipsis);
        if (SUCCEEDED(hr) && ellipsis)
        {
            const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
            hr = layout->SetTrimming(&trimming, ellipsis);
            ellipsis->Release();
        }
    }
    format->Release();
    factory->Release();
    if (FAILED(hr) || !layout)
    {
        if (layout) layout->Release();
        native->Release();
        return fail_hr(L, "DirectWrite text layout", FAILED(hr) ? hr : E_FAIL);
    }
    D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE;
    if (clip && overflow != "visible") options = D2D1_DRAW_TEXT_OPTIONS_CLIP;
    painter->target->DrawTextLayout(D2D1::Point2F(rect.left, rect.top), layout, native, options);
    layout->Release();
    native->Release();
    return 0;
}

inline int painter_push_clip(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    const auto rect = check_rect(L, 2);
    painter->target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    painter->clips.push_back(rect);
    return 0;
}

inline int painter_pop_clip(lua_State *L)
{
    auto *painter = check_painter(L, 1);
    if (painter->clips.empty()) return luaL_error(L, "pop_clip called without a matching push_clip");
    painter->target->PopAxisAlignedClip();
    painter->clips.pop_back();
    return 0;
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
    context->d2d_render_target_stack.push(image->target);
    lua_pushvalue(L, 2);
    auto *painter = push_painter(L, context, image->target);
    const int status = lua_pcall(L, 1, 0, 0);
    invalidate_painter(painter);
    context->d2d_render_target_stack.pop();
    const HRESULT hr = image->target->EndDraw();
    image->painting = false;
    if (status != LUA_OK) return lua_error(L);
    if (FAILED(hr)) return fail_hr(L, "ID2D1BitmapRenderTarget::EndDraw", hr);
    return 0;
}

inline void create_metatable(
    lua_State *L, const char *name, const luaL_Reg *methods, lua_CFunction index, lua_CFunction gc)
{
    if (luaL_newmetatable(L, name))
    {
        luaL_setfuncs(L, methods, 0);
        if (index)
        {
            lua_pushcfunction(L, index);
            lua_setfield(L, -2, "__index");
        }
        else
        {
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
        }
        lua_pushcfunction(L, gc);
        lua_setfield(L, -2, "__gc");
        lua_pushstring(L, name);
        lua_setfield(L, -2, "__name");
    }
    lua_pop(L, 1);
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
        style->family = Detail::utf8_to_wide(L, -1);
    }
    else if (lua_istable(L, -1))
    {
        const size_t count = lua_rawlen(L, -1);
        for (size_t i = 0; i < count; ++i)
        {
            lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));
            if (lua_isstring(L, -1))
            {
                const auto candidate = Detail::utf8_to_wide(L, -1);
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

    style->size = Detail::table_number(L, 1, "size", 12);
    if (!(style->size > 0)) luaL_error(L, "font size must be greater than zero");
    const float weight = Detail::table_number(L, 1, "weight", 400);
    if (weight < 1 || weight > 1000 || std::floor(weight) != weight)
        luaL_error(L, "font weight must be an integer from 1 through 1000");
    style->weight = static_cast<DWRITE_FONT_WEIGHT>(static_cast<int>(weight));
    const auto slant = Detail::table_string(L, 1, "slant", "normal");
    if (slant == "normal")
        style->slant = DWRITE_FONT_STYLE_NORMAL;
    else if (slant == "italic")
        style->slant = DWRITE_FONT_STYLE_ITALIC;
    else if (slant == "oblique")
        style->slant = DWRITE_FONT_STYLE_OBLIQUE;
    else
        luaL_error(L, "invalid font slant '%s'", slant.c_str());
    style->underline = Detail::table_bool(L, 1, "underline", false);
    style->strikethrough = Detail::table_bool(L, 1, "strikethrough", false);
    style->letter_spacing = Detail::table_number(L, 1, "letter_spacing", 0);
    lua_getfield(L, 1, "line_height");
    if (!lua_isnil(L, -1))
    {
        style->line_height = Detail::finite_number(L, -1, "line_height");
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
    const auto path = Detail::utf8_to_wide(L, 1);
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
    const auto text = Detail::utf8_to_wide(L, 1);
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
            width = Detail::finite_number(L, -1, "width");
            if (width < 0) luaL_error(L, "text constraint width must be non-negative");
            has_width = true;
        }
        lua_pop(L, 1);
        lua_getfield(L, 3, "height");
        if (!lua_isnil(L, -1))
        {
            height = Detail::finite_number(L, -1, "height");
            if (height < 0) luaL_error(L, "text constraint height must be non-negative");
            has_height = true;
        }
        lua_pop(L, 1);
        wrap = Detail::table_string(L, 3, "wrap", has_width ? "word" : "none");
        lua_getfield(L, 3, "max_lines");
        if (!lua_isnil(L, -1))
        {
            max_lines = luaL_checkinteger(L, -1);
            if (max_lines <= 0) luaL_error(L, "max_lines must be greater than zero");
        }
        lua_pop(L, 1);
    }

    IDWriteFactory *factory = nullptr;
    HRESULT hr = Detail::create_text_factory(&factory);
    if (FAILED(hr) || !factory) return Detail::fail_hr(L, "DWriteCreateFactory", FAILED(hr) ? hr : E_FAIL);
    IDWriteTextFormat *format = nullptr;
    hr = Detail::create_text_format(factory, style, &format);
    if (SUCCEEDED(hr)) hr = format->SetWordWrapping(Detail::parse_wrap(L, wrap));
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

    lua_createtable(L, 0, 5);
    lua_pushnumber(L, has_width ? std::min(metrics.widthIncludingTrailingWhitespace, width)
                                : metrics.widthIncludingTrailingWhitespace);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, measured_height);
    lua_setfield(L, -2, "height");
    lua_pushinteger(L, reported_lines);
    lua_setfield(L, -2, "line_count");
    lua_pushnumber(L, lines.empty() ? 0 : lines.front().baseline);
    lua_setfield(L, -2, "baseline");
    lua_pushboolean(L, truncated);
    lua_setfield(L, -2, "truncated");
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
    Detail::create_metatable(L, Detail::BRUSH_MT, brush_methods, nullptr, Detail::brush_gc);
    Detail::create_metatable(L, Detail::IMAGE_MT, image_methods, Detail::image_index, Detail::image_gc);
    Detail::create_metatable(L, Detail::TEXT_STYLE_MT, text_style_methods, nullptr, Detail::text_style_gc);
    Detail::create_metatable(L, Detail::PAINTER_MT, painter_methods, nullptr, Detail::painter_gc);
}

inline int invoke_paint_callback(lua_State *L)
{
    lua_pushcfunction(L, Detail::screen_paint);
    lua_insert(L, -2);
    return lua_pcall(L, 1, 0, 0);
}
} // namespace LuaCore::Painter
