/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaRenderer.h>
#include <lua/LuaManager.h>

namespace LuaCore::D2D
{
typedef struct
{
    uint64_t text_hash;
    uint64_t font_name_hash;
    int font_weight;
    int font_style;
    float font_size;
    int horizontal_alignment;
    int vertical_alignment;
    float width;
    float height;
} t_text_layout_params;

typedef struct
{
    uint64_t text_hash;
    uint64_t font_name_hash;
    float font_size;
    float max_width;
    float max_height;
} t_text_measure_params;

typedef struct
{
    float r;
    float g;
    float b;
    float a;
} t_d2d_color;

typedef struct
{
    ID2D1Bitmap *bmp;
    D2D1_RECT_F destination_rectangle;
    D2D1_RECT_F source_rectangle;
    t_d2d_color color;
    int interpolation;
} t_draw_image_params;

#define D2D_GET_RECT(L, idx)                                                                                           \
    D2D1::RectF(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1), luaL_checknumber(L, idx + 2),                  \
                luaL_checknumber(L, idx + 3))

#define D2D_GET_COLOR(L, idx)                                                                                          \
    D2D1::ColorF(luaL_checknumber(L, idx), luaL_checknumber(L, idx + 1), luaL_checknumber(L, idx + 2),                 \
                 luaL_checknumber(L, idx + 3))

#define D2D_GET_POINT(L, idx)                                                                                          \
    D2D1_POINT_2F                                                                                                      \
    {                                                                                                                  \
        .x = (float)luaL_checknumber(L, idx), .y = (float)luaL_checknumber(L, idx + 1)                                 \
    }

#define D2D_GET_ELLIPSE(L, idx)                                                                                        \
    D2D1_ELLIPSE                                                                                                       \
    {                                                                                                                  \
        .point = D2D_GET_POINT(L, idx), .radiusX = (float)luaL_checknumber(L, idx + 2),                                \
        .radiusY = (float)luaL_checknumber(L, idx + 3)                                                                 \
    }

#define D2D_GET_ROUNDED_RECT(L, idx)                                                                                   \
    D2D1_ROUNDED_RECT(D2D_GET_RECT(L, idx), luaL_checknumber(L, idx + 5), luaL_checknumber(L, idx + 6))

static t_draw_image_params check_draw_image_params(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);

    t_draw_image_params params{};

    lua_getfield(L, index, "identifier");
    params.bmp = (ID2D1Bitmap *)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    D2D1_SIZE_U bmp_size = params.bmp->GetPixelSize();

    lua_getfield(L, index, "destx1");
    float destx1 = luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "desty1");
    float desty1 = luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "destx2");
    float destx2 = lua_isnoneornil(L, -1) ? destx1 + bmp_size.width : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "desty2");
    float desty2 = lua_isnoneornil(L, -1) ? desty1 + bmp_size.height : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "srcx1");
    float srcx1 = lua_isnoneornil(L, -1) ? 0 : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "srcy1");
    float srcy1 = lua_isnoneornil(L, -1) ? 0 : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "srcx2");
    float srcx2 = lua_isnoneornil(L, -1) ? srcx1 + bmp_size.width : lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "srcy2");
    float srcy2 = lua_isnoneornil(L, -1) ? srcy1 + bmp_size.height : lua_tonumber(L, -1);
    lua_pop(L, 1);

    params.destination_rectangle = D2D1::RectF(destx1, desty1, destx2, desty2);
    params.source_rectangle = D2D1::RectF(srcx1, srcy1, srcx2, srcy2);

    lua_getfield(L, index, "interpolation");
    params.interpolation = lua_isnoneornil(L, -1) ? 1 : lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "color");
    params.color = {1.0f, 1.0f, 1.0f, 1.0f};
    if (!lua_isnoneornil(L, -1))
    {
        lua_getfield(L, -1, "r");
        params.color.r = lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "g");
        params.color.g = lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "b");
        params.color.b = lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "a");
        params.color.a = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return params;
}

static int get_target_fps(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    if (lua->rctx.target_fps.has_value())
        lua_pushnumber(L, lua->rctx.target_fps.value());
    else
        lua_pushnil(L);

    return 1;
}

static int set_target_fps(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    std::optional<float> fps;
    if (!lua_isnoneornil(L, 1)) fps = (float)luaL_checknumber(L, 1);

    LuaRenderer::set_target_fps(&lua->rctx, fps);

    return 0;
}

static int create_brush(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1::ColorF color = D2D_GET_COLOR(L, 1);

    ID2D1SolidColorBrush *brush;
    lua->rctx.d2d_render_target_stack.top()->CreateSolidColorBrush(color, &brush);

    lua_pushinteger(L, (uint64_t)brush);
    return 1;
}

static int free_brush(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 1);
    brush->Release();
    return 0;
}

static int clear(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1::ColorF color = D2D_GET_COLOR(L, 1);

    // COMPAT: This is really what this did! It just ignores the specified color and uses the mask!
    // We should probably do something about this hahaha
    if (g_config.presenter_type == (int32_t)t_config::PresenterType::DirectComposition)
        lua->rctx.d2d_render_target_stack.top()->Clear(color);
    else
        lua->rctx.d2d_render_target_stack.top()->Clear(D2D1::ColorF(LuaRenderer::LUA_GDI_COLOR_MASK));

    return 0;
}

static int fill_rectangle(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1_RECT_F rectangle = D2D_GET_RECT(L, 1);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 5);

    lua->rctx.d2d_render_target_stack.top()->FillRectangle(&rectangle, brush);

    return 0;
}

static int draw_rectangle(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1_RECT_F rectangle = D2D_GET_RECT(L, 1);
    float thickness = luaL_checknumber(L, 5);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 6);

    lua->rctx.d2d_render_target_stack.top()->DrawRectangle(&rectangle, brush, thickness);

    return 0;
}

static int fill_ellipse(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    D2D1_ELLIPSE ellipse = D2D_GET_ELLIPSE(L, 1);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 5);

    lua->rctx.d2d_render_target_stack.top()->FillEllipse(&ellipse, brush);

    return 0;
}

static int draw_ellipse(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1_ELLIPSE ellipse = D2D_GET_ELLIPSE(L, 1);
    float thickness = luaL_checknumber(L, 5);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 6);

    lua->rctx.d2d_render_target_stack.top()->DrawEllipse(&ellipse, brush, thickness);

    return 0;
}

static int draw_line(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1_POINT_2F point_a = D2D_GET_POINT(L, 1);
    D2D1_POINT_2F point_b = D2D_GET_POINT(L, 3);
    float thickness = luaL_checknumber(L, 5);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 6);

    lua->rctx.d2d_render_target_stack.top()->DrawLine(point_a, point_b, brush, thickness);

    return 0;
}

static int draw_text(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1_RECT_F rectangle = D2D_GET_RECT(L, 1);
    auto text = std::string(luaL_checkstring(L, 5));
    auto font_name = std::string(luaL_checkstring(L, 6));
    auto font_size = static_cast<float>(luaL_checknumber(L, 7));
    auto font_weight = static_cast<int>(luaL_checknumber(L, 8));
    auto font_style = static_cast<int>(luaL_checkinteger(L, 9));
    auto horizontal_alignment = static_cast<int>(luaL_checkinteger(L, 10));
    auto vertical_alignment = static_cast<int>(luaL_checkinteger(L, 11));
    int options = luaL_checkinteger(L, 12);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 13);

    uint64_t font_name_hash = xxh64::hash(font_name.data(), font_name.size(), 0);
    uint64_t text_hash = xxh64::hash(text.data(), text.size(), 0);

    t_text_layout_params params = {
        .text_hash = text_hash,
        .font_name_hash = font_name_hash,
        .font_weight = font_weight,
        .font_style = font_style,
        .font_size = font_size,
        .horizontal_alignment = horizontal_alignment,
        .vertical_alignment = vertical_alignment,
        .width = rectangle.right - rectangle.left,
        .height = rectangle.bottom - rectangle.top,
    };

    if (params.width < 0.0f || params.height < 0.0f)
    {
        return 0;
    }

    uint64_t params_hash = xxh64::hash((const char *)&params, sizeof(params), 0);

    if (!lua->rctx.dw_text_layouts.contains(params_hash))
    {
        // g_view_logger->info("[Lua] Adding layout to cache... ({} elements)\n", lua->rctx.dw_text_layouts.size());

        IDWriteTextFormat *text_format;

        lua->rctx.dw_factory->CreateTextFormat(
            IOUtils::to_wide_string(font_name).c_str(), nullptr, static_cast<DWRITE_FONT_WEIGHT>(font_weight),
            static_cast<DWRITE_FONT_STYLE>(font_style), DWRITE_FONT_STRETCH_NORMAL, font_size, L"", &text_format);

        text_format->SetTextAlignment(static_cast<DWRITE_TEXT_ALIGNMENT>(horizontal_alignment));
        text_format->SetParagraphAlignment(static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(vertical_alignment));

        IDWriteTextLayout *text_layout;

        auto wtext = IOUtils::to_wide_string(text);
        lua->rctx.dw_factory->CreateTextLayout(wtext.c_str(), wtext.length(), text_format,
                                               rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,
                                               &text_layout);

        lua->rctx.dw_text_layouts.add(params_hash, text_layout);
        text_format->Release();
    }

    auto layout = lua->rctx.dw_text_layouts.get(params_hash);
    lua->rctx.d2d_render_target_stack.top()->DrawTextLayout(
        {
            .x = rectangle.left,
            .y = rectangle.top,
        },
        layout.value(), brush, static_cast<D2D1_DRAW_TEXT_OPTIONS>(options));

    return 0;
}

static int set_text_antialias_mode(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);
    float mode = luaL_checkinteger(L, 1);
    lua->rctx.d2d_render_target_stack.top()->SetTextAntialiasMode((D2D1_TEXT_ANTIALIAS_MODE)mode);
    return 0;
}

static int set_antialias_mode(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);
    float mode = luaL_checkinteger(L, 1);
    lua->rctx.d2d_render_target_stack.top()->SetAntialiasMode((D2D1_ANTIALIAS_MODE)mode);
    return 0;
}

static int measure_text(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    std::wstring text = IOUtils::to_wide_string(std::string(luaL_checkstring(L, 1)));
    std::string font_name = std::string(luaL_checkstring(L, 2));
    float font_size = luaL_checknumber(L, 3);
    float max_width = luaL_checknumber(L, 4);
    float max_height = luaL_checknumber(L, 5);

    uint64_t font_name_hash = xxh64::hash(font_name.data(), font_name.size(), 0);
    uint64_t text_hash = xxh64::hash((char *)text.data(), text.size() * sizeof(wchar_t), 0);

    t_text_measure_params params = {
        .text_hash = text_hash,
        .font_name_hash = font_name_hash,
        .font_size = font_size,
        .max_width = max_width,
        .max_height = max_height,
    };

    uint64_t params_hash = xxh64::hash((const char *)&params, sizeof(params), 0);

    if (!lua->rctx.dw_text_sizes.contains(params_hash))
    {
        IDWriteTextFormat *text_format;

        lua->rctx.dw_factory->CreateTextFormat(IOUtils::to_wide_string(font_name).c_str(), NULL,
                                               DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                               DWRITE_FONT_STRETCH_NORMAL, font_size, L"", &text_format);

        IDWriteTextLayout *text_layout;

        lua->rctx.dw_factory->CreateTextLayout(text.c_str(), text.length(), text_format, max_width, max_height,
                                               &text_layout);

        DWRITE_TEXT_METRICS text_metrics;
        text_layout->GetMetrics(&text_metrics);

        lua->rctx.dw_text_sizes.add(params_hash, text_metrics);

        text_format->Release();
        text_layout->Release();
    }

    const auto text_metrics = lua->rctx.dw_text_sizes.get(params_hash).value();

    lua_newtable(L);
    lua_pushinteger(L, text_metrics.widthIncludingTrailingWhitespace);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, text_metrics.height);
    lua_setfield(L, -2, "height");

    return 1;
}

static int push_clip(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1_RECT_F rectangle = D2D_GET_RECT(L, 1);

    lua->rctx.d2d_render_target_stack.top()->PushAxisAlignedClip(rectangle, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    return 0;
}

static int pop_clip(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    lua->rctx.d2d_render_target_stack.top()->PopAxisAlignedClip();

    return 0;
}

static int fill_rounded_rectangle(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1_ROUNDED_RECT rounded_rectangle = D2D_GET_ROUNDED_RECT(L, 1);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 7);

    lua->rctx.d2d_render_target_stack.top()->FillRoundedRectangle(&rounded_rectangle, brush);

    return 0;
}

static int draw_rounded_rectangle(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    D2D1_ROUNDED_RECT rounded_rectangle = D2D_GET_ROUNDED_RECT(L, 1);
    float thickness = luaL_checknumber(L, 7);
    auto brush = (ID2D1SolidColorBrush *)luaL_checkinteger(L, 8);

    lua->rctx.d2d_render_target_stack.top()->DrawRoundedRectangle(&rounded_rectangle, brush, thickness);

    return 0;
}

static int load_image(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    std::string path(luaL_checkstring(L, 1));

    IWICImagingFactory *pIWICFactory = NULL;
    IWICBitmapDecoder *pDecoder = NULL;
    IWICBitmapFrameDecode *pSource = NULL;
    IWICFormatConverter *pConverter = NULL;
    ID2D1Bitmap *bmp = NULL;

    CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICFactory));

    HRESULT hr = pIWICFactory->CreateDecoderFromFilename(IOUtils::to_wide_string(path).c_str(), NULL, GENERIC_READ,
                                                         WICDecodeMetadataCacheOnLoad, &pDecoder);

    if (!SUCCEEDED(hr))
    {
        g_view_logger->info("D2D image fail HRESULT {}", hr);
        pIWICFactory->Release();
        return 0;
    }

    pIWICFactory->CreateFormatConverter(&pConverter);
    pDecoder->GetFrame(0, &pSource);
    pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f,
                           WICBitmapPaletteTypeMedianCut);

    lua->rctx.d2d_render_target_stack.top()->CreateBitmapFromWicBitmap(pConverter, NULL, &bmp);

    pIWICFactory->Release();
    pDecoder->Release();
    pSource->Release();
    pConverter->Release();

    lua_pushinteger(L, (uint64_t)bmp);
    return 1;
}

static int free_image(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    auto bmp = (ID2D1Bitmap *)luaL_checkinteger(L, 1);
    bmp->Release();
    return 0;
}

static int draw_image2(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    auto params = check_draw_image_params(L, 1);
    const auto color = params.color;

    // Fast path: no tint.
    if (params.color.r == 1.0f && params.color.g == 1.0f && params.color.b == 1.0f)
    {
        lua->rctx.d2d_render_target_stack.top()->DrawBitmap(params.bmp, params.destination_rectangle, params.color.a,
                                                            (D2D1_BITMAP_INTERPOLATION_MODE)params.interpolation,
                                                            params.source_rectangle);
        return 0;
    }

    ComPtr<ID2D1DeviceContext> dc;
    const auto hr = lua->rctx.d2d_render_target_stack.top()->QueryInterface(IID_PPV_ARGS(dc.GetAddressOf()));
    RT_ASSERT_HR(hr, L"Failed to get ID2D1DeviceContext from render target");

    ComPtr<ID2D1Effect> effect;
    dc->CreateEffect(CLSID_D2D1ColorMatrix, effect.GetAddressOf());

    effect->SetInput(0, params.bmp);
    effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, D2D1::Matrix5x4F(color.r, 0, 0, 0, 0, color.g, 0, 0, 0, 0,
                                                                          color.b, 0, 0, 0, 0, color.a, 0, 0, 0, 0));

    D2D1_MATRIX_3X2_F old_transform;
    dc->GetTransform(&old_transform);

    float src_w = params.source_rectangle.right - params.source_rectangle.left;
    float src_h = params.source_rectangle.bottom - params.source_rectangle.top;
    float dst_w = params.destination_rectangle.right - params.destination_rectangle.left;
    float dst_h = params.destination_rectangle.bottom - params.destination_rectangle.top;
    float scale_x = src_w > 0 ? dst_w / src_w : 1.0f;
    float scale_y = src_h > 0 ? dst_h / src_h : 1.0f;

    dc->SetTransform(
        D2D1::Matrix3x2F::Scale(scale_x, scale_y) *
        D2D1::Matrix3x2F::Translation(params.destination_rectangle.left, params.destination_rectangle.top) *
        old_transform);

    ComPtr<ID2D1Image> output;
    effect->GetOutput(output.GetAddressOf());

    D2D1_POINT_2F target_offset = D2D1::Point2F(0, 0);
    dc->DrawImage(output.Get(), &target_offset, &params.source_rectangle, (D2D1_INTERPOLATION_MODE)params.interpolation,
                  D2D1_COMPOSITE_MODE_SOURCE_OVER);

    dc->SetTransform(old_transform);

    return 0;
}

static int get_image_info(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    auto bmp = (ID2D1Bitmap *)luaL_checkinteger(L, 1);

    D2D1_SIZE_U size = bmp->GetPixelSize();
    lua_newtable(L);
    lua_pushinteger(L, size.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, size.height);
    lua_setfield(L, -2, "height");

    return 1;
}

static int draw_to_image(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    LuaRenderer::ensure_d2d_renderer_created(&lua->rctx);

    float width = std::max((float)luaL_checknumber(L, 1), 1.0f);
    float height = std::max((float)luaL_checknumber(L, 2), 1.0f);

    ID2D1BitmapRenderTarget *render_target;
    lua->rctx.d2d_render_target_stack.top()->CreateCompatibleRenderTarget(D2D1::SizeF(width, height), &render_target);

    // With render target at top of stack, we hand control back to script and let it run its callback with rt-scoped
    // drawing
    lua->rctx.d2d_render_target_stack.push(render_target);
    render_target->BeginDraw();
    render_target->Clear(D2D1::ColorF(0, 0, 0, 0));
    lua_call(L, 0, 0);
    render_target->EndDraw();
    lua->rctx.d2d_render_target_stack.pop();

    ID2D1Bitmap *bmp;
    render_target->GetBitmap(&bmp);

    render_target->Release();
    lua_pushinteger(L, (uint64_t)bmp);
    return 1;
}

#undef D2D_GET_RECT
#undef D2D_GET_COLOR
#undef D2D_GET_POINT
#undef D2D_GET_ELLIPS
#undef D2D_GET_ROUNDED_RECT
} // namespace LuaCore::D2D
