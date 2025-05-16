/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaConsole.h>
#include <lua/LuaRenderer.h>

namespace LuaCore::Wgui
{
    const int hexTable[256] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    10,
    11,
    12,
    13,
    14,
    15,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    10,
    11,
    12,
    13,
    14,
    15,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    };

    const std::unordered_map<std::wstring, COLORREF> color_map = {
    {L"white", 0xFFFFFFFF},
    {L"black", 0xFF000000},
    {L"clear", 0x00000000},
    {L"gray", 0xFF808080},
    {L"red", 0xFF0000FF},
    {L"orange", 0xFF0080FF},
    {L"yellow", 0xFF00FFFF},
    {L"chartreuse", 0xFF00FF80},
    {L"green", 0xFF00FF00},
    {L"teal", 0xFF80FF00},
    {L"cyan", 0xFFFFFF00},
    {L"blue", 0xFFFF0000},
    {L"purple", 0xFFFF0080},
    };

    static int GetGUIInfo(lua_State* L)
    {
        auto lua = get_lua_class(L);

        RECT rect;
        GetClientRect(g_main_hwnd, &rect);

        lua_newtable(L);
        lua_pushinteger(L, rect.right - rect.left);
        lua_setfield(L, -2, "width");
        lua_pushinteger(L, rect.bottom - rect.top);
        lua_setfield(L, -2, "height");
        return 1;
    }

    static int ResizeWindow(lua_State* L)
    {
        assert(is_on_gui_thread());

        auto lua = get_lua_class(L);

        RECT clientRect, wndRect;
        GetWindowRect(g_main_hwnd, &wndRect);
        GetClientRect(g_main_hwnd, &clientRect);
        wndRect.bottom -= wndRect.top;
        wndRect.right -= wndRect.left;
        int w = luaL_checkinteger(L, 1),
            h = luaL_checkinteger(L, 2);
        SetWindowPos(g_main_hwnd, 0, 0, 0, w + (wndRect.right - clientRect.right), h + (wndRect.bottom - clientRect.bottom), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

        // we need to recreate the renderer to accomodate for size changes (this cant be done in-place)
        LuaRenderer::destroy_renderer(&lua->rctx);
        LuaRenderer::create_renderer(&lua->rctx, lua);

        return 0;
    }

    static COLORREF StrToColorA(const std::wstring& s, bool alpha = false, COLORREF def = 0)
    {
        if (s[0] == '#')
        {
            int l = s.size();
            if (l == 4)
            {
                return (hexTable[s[1]] * 0x10 + hexTable[s[1]]) |
                ((hexTable[s[2]] * 0x10 + hexTable[s[2]]) << 8) |
                ((hexTable[s[3]] * 0x10 + hexTable[s[3]]) << 16) |
                (alpha ? 0xFF000000 : 0);
            }
            else if (alpha && l == 5)
            {
                return (hexTable[s[1]] * 0x10 + hexTable[s[1]]) |
                ((hexTable[s[2]] * 0x10 + hexTable[s[2]]) << 8) |
                ((hexTable[s[3]] * 0x10 + hexTable[s[3]]) << 16) |
                ((hexTable[s[4]] * 0x10 + hexTable[s[4]]) << 24);
            }
            else if (l == 7)
            {
                return (hexTable[s[1]] * 0x10 + hexTable[s[2]]) |
                ((hexTable[s[3]] * 0x10 + hexTable[s[4]]) << 8) |
                ((hexTable[s[5]] * 0x10 + hexTable[s[6]]) << 16) |
                (alpha ? 0xFF000000 : 0);
            }
            else if (alpha && l == 9)
            {
                return (hexTable[s[1]] * 0x10 + hexTable[s[2]]) |
                ((hexTable[s[3]] * 0x10 + hexTable[s[4]]) << 8) |
                ((hexTable[s[5]] * 0x10 + hexTable[s[6]]) << 16) |
                ((hexTable[s[7]] * 0x10 + hexTable[s[8]]) << 24);
            }
        }
        else
        {
            if (color_map.contains(s))
            {
                return (alpha ? 0xFFFFFFFF : 0xFFFFFF) & color_map.at(s);
            }
        }
        return (alpha ? 0xFF000000 : 0x00000000) | def;
    }

    static COLORREF StrToColor(const std::wstring& s, bool alpha = false, COLORREF def = 0)
    {
        return StrToColorA(s, alpha, def);
    }

    static int set_brush(lua_State* L)
    {
        auto lua = get_lua_class(L);

        if (lua->rctx.brush)
        {
            DeleteObject(lua->rctx.brush);
        }

        auto s = string_to_wstring(lua_tostring(L, 1));
        if (iequals(s, L"null"))
            lua->rctx.brush = (HBRUSH)GetStockObject(NULL_BRUSH);
        else
            lua->rctx.brush = CreateSolidBrush(StrToColor(s));

        return 0;
    }

    static int set_pen(lua_State* L)
    {
        auto lua = get_lua_class(L);

        if (lua->rctx.pen)
        {
            DeleteObject(lua->rctx.pen);
        }

        auto s = string_to_wstring(lua_tostring(L, 1));
        int width = luaL_optnumber(L, 2, 1);

        if (iequals(s, L"null"))
            lua->rctx.pen = (HPEN)GetStockObject(NULL_PEN);
        else
            lua->rctx.pen = CreatePen(PS_SOLID, width, StrToColor(s));

        return 0;
    }

    static int set_text_color(lua_State* L)
    {
        auto lua = get_lua_class(L);
        lua->rctx.col = StrToColor(string_to_wstring(lua_tostring(L, 1)));
        return 0;
    }

    static int SetBackgroundColor(lua_State* L)
    {
        auto lua = get_lua_class(L);

        auto s = string_to_wstring(lua_tostring(L, 1));

        if (iequals(s, L"null"))
        {
            lua->rctx.bkmode = TRANSPARENT;
        }
        else
        {
            lua->rctx.bkcol = StrToColor(s);
            lua->rctx.bkmode = OPAQUE;
        }

        return 0;
    }

    static int SetFont(lua_State* L)
    {
        auto lua = get_lua_class(L);
        LOGFONT font = {0};

        if (lua->rctx.font)
        {
            DeleteObject(lua->rctx.font);
        }

        auto font_size = luaL_checknumber(L, 1);
        auto font_name = string_to_wstring(luaL_optstring(L, 2, "MS Gothic"));
        auto style = string_to_wstring(luaL_optstring(L, 3, ""));

        // set the size of the font
        font.lfHeight = -MulDiv(font_size, GetDeviceCaps(lua->rctx.gdi_back_dc, LOGPIXELSY), 72);
        lstrcpyn(font.lfFaceName, font_name.c_str(), LF_FACESIZE);
        font.lfCharSet = DEFAULT_CHARSET;

        for (const auto p : style)
        {
            switch (p)
            {
            case 'b':
                font.lfWeight = FW_BOLD;
                break;
            case 'i':
                font.lfItalic = TRUE;
                break;
            case 'u':
                font.lfUnderline = TRUE;
                break;
            case 's':
                font.lfStrikeOut = TRUE;
                break;
            case 'a':
                font.lfQuality = ANTIALIASED_QUALITY;
                break;
            default:
                break;
            }
        }

        lua->rctx.font = CreateFontIndirect(&font);
        return 0;
    }

    static int LuaTextOut(lua_State* L)
    {
        auto lua = get_lua_class(L);

        SetBkMode(lua->rctx.gdi_back_dc, lua->rctx.bkmode);
        SetBkColor(lua->rctx.gdi_back_dc, lua->rctx.bkcol);
        SetTextColor(lua->rctx.gdi_back_dc, lua->rctx.col);
        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.font);

        int x = luaL_checknumber(L, 1);
        int y = luaL_checknumber(L, 2);
        auto text = string_to_wstring(lua_tostring(L, 3));

        ::TextOut(lua->rctx.gdi_back_dc, x, y, text.c_str(), text.size());
        return 0;
    }

    static bool GetRectLua(lua_State* L, int idx, RECT* rect)
    {
        switch (lua_type(L, idx))
        {
        case LUA_TTABLE:
            lua_getfield(L, idx, "l");
            rect->left = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, idx, "t");
            rect->top = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, idx, "r");
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 1);
                lua_getfield(L, idx, "w");
                if (lua_isnil(L, -1))
                {
                    return false;
                }
                rect->right = rect->left + lua_tointeger(L, -1);
                lua_pop(L, 1);
                lua_getfield(L, idx, "h");
                rect->bottom = rect->top + lua_tointeger(L, -1);
                lua_pop(L, 1);
            }
            else
            {
                rect->right = lua_tointeger(L, -1);
                lua_pop(L, 1);
                lua_getfield(L, idx, "b");
                rect->bottom = lua_tointeger(L, -1);
                lua_pop(L, 1);
            }
            return true;
        default:
            return false;
        }
    }

    static int GetTextExtent(lua_State* L)
    {
        auto lua = get_lua_class(L);
        auto string = string_to_wstring(luaL_checkstring(L, 1));

        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.font);

        SIZE size = {0};
        GetTextExtentPoint32(lua->rctx.gdi_back_dc, string.c_str(), string.size(), &size);

        lua_newtable(L);
        lua_pushinteger(L, size.cx);
        lua_setfield(L, -2, "width");
        lua_pushinteger(L, size.cy);
        lua_setfield(L, -2, "height");
        return 1;
    }

    static int LuaDrawText(lua_State* L)
    {
        auto lua = get_lua_class(L);

        SetBkMode(lua->rctx.gdi_back_dc, lua->rctx.bkmode);
        SetBkColor(lua->rctx.gdi_back_dc, lua->rctx.bkcol);
        SetTextColor(lua->rctx.gdi_back_dc, lua->rctx.col);
        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.font);

        RECT rect = {0};
        UINT format = DT_NOPREFIX | DT_WORDBREAK;
        if (!GetRectLua(L, 2, &rect))
        {
            format |= DT_NOCLIP;
        }
        if (!lua_isnil(L, 3))
        {
            const char* p = lua_tostring(L, 3);
            for (; p && *p; p++)
            {
                switch (*p)
                {
                case 'l':
                    format |= DT_LEFT;
                    break;
                case 'r':
                    format |= DT_RIGHT;
                    break;
                case 't':
                    format |= DT_TOP;
                    break;
                case 'b':
                    format |= DT_BOTTOM;
                    break;
                case 'c':
                    format |= DT_CENTER;
                    break;
                case 'v':
                    format |= DT_VCENTER;
                    break;
                case 'e':
                    format |= DT_WORD_ELLIPSIS;
                    break;
                case 's':
                    format |= DT_SINGLELINE;
                    break;
                case 'n':
                    format &= ~DT_WORDBREAK;
                    break;
                }
            }
        }
        auto str = string_to_wstring(lua_tostring(L, 1));

        ::DrawText(lua->rctx.gdi_back_dc, str.c_str(), -1, &rect, format);
        return 0;
    }

    static int LuaDrawTextAlt(lua_State* L)
    {
        auto lua = get_lua_class(L);

        SetBkMode(lua->rctx.gdi_back_dc, lua->rctx.bkmode);
        SetBkColor(lua->rctx.gdi_back_dc, lua->rctx.bkcol);
        SetTextColor(lua->rctx.gdi_back_dc, lua->rctx.col);
        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.font);

        RECT rect = {0};
        auto string = string_to_wstring(lua_tostring(L, 1));
        UINT format = luaL_checkinteger(L, 2);
        rect.left = luaL_checkinteger(L, 3);
        rect.top = luaL_checkinteger(L, 4);
        rect.right = luaL_checkinteger(L, 5);
        rect.bottom = luaL_checkinteger(L, 6);

        DrawTextEx(lua->rctx.gdi_back_dc, string.data(), -1, &rect, format, NULL);
        return 0;
    }

    static int DrawRect(lua_State* L)
    {
        auto lua = get_lua_class(L);
        int left = luaL_checknumber(L, 1);
        int top = luaL_checknumber(L, 2);
        int right = luaL_checknumber(L, 3);
        int bottom = luaL_checknumber(L, 4);
        int cornerW = luaL_optnumber(L, 5, 0);
        int cornerH = luaL_optnumber(L, 6, 0);

        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.brush);
        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.pen);
        RoundRect(lua->rctx.gdi_back_dc, left, top, right, bottom, cornerW, cornerH);
        return 0;
    }

    static int LuaLoadImage(lua_State* L)
    {
        auto lua = get_lua_class(L);
        std::wstring path = string_to_wstring(luaL_checkstring(L, 1));

        auto img = new Gdiplus::Bitmap(path.c_str());
        if (!img || img->GetLastStatus())
        {
            luaL_error(L, "Couldn't find image '%s'", path.c_str());
            return 0;
        }

        lua->rctx.image_pool_index++;
        lua->rctx.image_pool[lua->rctx.image_pool_index] = img;

        lua_pushinteger(L, lua->rctx.image_pool_index);
        return 1;
    }

    static int DeleteImage(lua_State* L)
    {
        auto lua = get_lua_class(L);
        size_t key = luaL_checkinteger(L, 1);

        if (key == 0)
        {
            g_view_logger->info("Deleting all images");
            for (auto& [_, val] : lua->rctx.image_pool)
            {
                delete val;
            }
            lua->rctx.image_pool.clear();
        }
        else
        {
            if (!lua->rctx.image_pool.contains(key))
            {
                luaL_error(L, "Argument #1: Image index doesn't exist");
                return 0;
            }

            delete lua->rctx.image_pool[key];
            lua->rctx.image_pool[key] = nullptr;
            lua->rctx.image_pool.erase(key);
        }
        return 0;
    }

    static int DrawImage(lua_State* L)
    {
        auto lua = get_lua_class(L);
        size_t key = luaL_checkinteger(L, 1);

        if (!lua->rctx.image_pool.contains(key))
        {
            luaL_error(L, "Argument #1: Image index doesn't exist");
            return 0;
        }

        // Gets the number of arguments
        unsigned int args = lua_gettop(L);

        Gdiplus::Graphics gfx(lua->rctx.gdi_back_dc);
        Gdiplus::Bitmap* img = lua->rctx.image_pool[key];

        // Original DrawImage
        if (args == 3)
        {
            int x = luaL_checknumber(L, 2);
            int y = luaL_checknumber(L, 3);

            gfx.DrawImage(img, x, y); // Gdiplus::Image *image, int x, int y
            return 0;
        }
        else if (args == 4)
        {
            int x = luaL_checknumber(L, 2);
            int y = luaL_checknumber(L, 3);
            float scale = luaL_checknumber(L, 4);
            if (scale == 0)
                return 0;

            // Create a Rect at x and y and scale the image's width and height
            Gdiplus::Rect dest(x, y, img->GetWidth() * scale, img->GetHeight() * scale);

            gfx.DrawImage(img, dest);
            // Gdiplus::Image *image, const Gdiplus::Rect &rect
            return 0;
        }
        // In original DrawImage
        else if (args == 5)
        {
            int x = luaL_checknumber(L, 2);
            int y = luaL_checknumber(L, 3);
            int w = luaL_checknumber(L, 4);
            int h = luaL_checknumber(L, 5);
            if (w == 0 or h == 0)
                return 0;

            Gdiplus::Rect dest(x, y, w, h);

            gfx.DrawImage(img, dest);
            // Gdiplus::Image *image, const Gdiplus::Rect &rect
            return 0;
        }
        else if (args == 10)
        {
            int x = luaL_checknumber(L, 2);
            int y = luaL_checknumber(L, 3);
            int w = luaL_checknumber(L, 4);
            int h = luaL_checknumber(L, 5);
            int srcx = luaL_checknumber(L, 6);
            int srcy = luaL_checknumber(L, 7);
            int srcw = luaL_checknumber(L, 8);
            int srch = luaL_checknumber(L, 9);
            float rotate = luaL_checknumber(L, 10);
            if (w == 0 or h == 0 or srcw == 0 or srch == 0)
                return 0;
            bool shouldrotate = ((int)rotate % 360) != 0;
            // Only rotate if the angle isn't a multiple of 360 Modulo only works with int

            Gdiplus::Rect dest(x, y, w, h);

            // Rotate
            if (shouldrotate)
            {
                Gdiplus::PointF center(x + (w / 2), y + (h / 2));
                // The center of dest
                Gdiplus::Matrix matrix;
                matrix.RotateAt(rotate, center);
                // rotate "rotate" number of degrees around "center"
                gfx.SetTransform(&matrix);
            }

            gfx.DrawImage(img, dest, srcx, srcy, srcw, srch, Gdiplus::UnitPixel);
            // Gdiplus::Image *image, const Gdiplus::Rect &destRect, int srcx, int srcy, int srcheight, Gdiplus::srcUnit

            if (shouldrotate)
                gfx.ResetTransform();
            return 0;
        }
        luaL_error(L, "Incorrect number of arguments");
        return 0;
    }


    static int LoadScreen(lua_State* L)
    {
        auto lua = get_lua_class(L);

        // Copy screen into the loadscreen dc
        auto dc = GetDC(g_main_hwnd);
        BitBlt(lua->rctx.loadscreen_dc, 0, 0, lua->rctx.dc_size.width, lua->rctx.dc_size.height, dc, 0, 0, SRCCOPY);
        ReleaseDC(g_main_hwnd, dc);

        Gdiplus::Bitmap* out = new Gdiplus::Bitmap(lua->rctx.loadscreen_bmp, nullptr);

        lua->rctx.image_pool_index++;
        lua->rctx.image_pool[lua->rctx.image_pool_index] = out;

        lua_pushinteger(L, lua->rctx.image_pool_index);
        return 1;
    }

    static int LoadScreenReset(lua_State* L)
    {
        auto lua = get_lua_class(L);
        LuaRenderer::loadscreen_reset(&lua->rctx);
        return 0;
    }

    static int GetImageInfo(lua_State* L)
    {
        auto lua = get_lua_class(L);
        size_t key = luaL_checkinteger(L, 1);

        if (!lua->rctx.image_pool.contains(key))
        {
            luaL_error(L, "Argument #1: Image index doesn't exist");
            return 0;
        }

        Gdiplus::Bitmap* img = lua->rctx.image_pool[key];

        lua_newtable(L);
        lua_pushinteger(L, img->GetWidth());
        lua_setfield(L, -2, "width");
        lua_pushinteger(L, img->GetHeight());
        lua_setfield(L, -2, "height");

        return 1;
    }

    // 1st arg is table of points
    // 2nd arg is color #xxxxxxxx
    static int FillPolygonAlpha(lua_State* L)
    {
        // Get lua instance stored in script class
        auto lua = get_lua_class(L);


        // stack should look like
        //--------
        // 2: color string
        //--------
        // 1: table of points
        //--------
        // assert that first argument is table
        luaL_checktype(L, 1, LUA_TTABLE);

        int n = luaL_len(L, 1); // length of the table, doesnt modify stack
        if (n > 255)
        {
            // hard cap, the vector can handle more but dont try
            lua_pushfstring(L, "wgui.polygon: too many points (%d > %d)", n, 255);
            return lua_error(L);
        }

        std::vector<Gdiplus::PointF> pts(n); // list of points that make the poly

        // do n times
        for (int i = 0; i < n; i++)
        {
            // push current index +1 because lua
            lua_pushinteger(L, i + 1);
            // get index i+1 from table at the bottom of the stack (index 1 is bottom, 2 is next etc, -1 is top)
            // pops the index and places the element inside table on top, which again is a table [x,y]
            lua_gettable(L, 1);
            // make sure its a table
            luaL_checktype(L, -1, LUA_TTABLE);
            // push '1'
            lua_pushinteger(L, 1);
            // get index 1 from table that is second from top, because '1' is on top right now
            // then remove '1' and put table contents, its the X coord
            lua_gettable(L, -2);
            // read it
            pts[i].X = lua_tointeger(L, -1);
            // remove X coord
            lua_pop(L, 1);
            // push '2'
            lua_pushinteger(L, 2);
            // same thing
            lua_gettable(L, -2);
            pts[i].Y = lua_tointeger(L, -1);
            lua_pop(L, 2);
            // now stack again has only table at the bottom and color string on top, repeat
        }

        Gdiplus::Graphics gfx(lua->rctx.gdi_back_dc);
        Gdiplus::SolidBrush brush(Gdiplus::Color(
        luaL_checkinteger(L, 2),
        luaL_checkinteger(L, 3),
        luaL_checkinteger(L, 4),
        luaL_checkinteger(L, 5)));
        gfx.FillPolygon(&brush, pts.data(), n);

        return 0;
    }


    static int FillEllipseAlpha(lua_State* L)
    {
        auto lua = get_lua_class(L);

        int x = luaL_checknumber(L, 1);
        int y = luaL_checknumber(L, 2);
        int w = luaL_checknumber(L, 3);
        int h = luaL_checknumber(L, 4);
        auto col = string_to_wstring(luaL_checkstring(L, 5));

        Gdiplus::Graphics gfx(lua->rctx.gdi_back_dc);
        Gdiplus::SolidBrush brush(Gdiplus::Color(StrToColorA(col, true)));

        gfx.FillEllipse(&brush, x, y, w, h);

        return 0;
    }

    static int FillRectAlpha(lua_State* L)
    {
        auto lua = get_lua_class(L);


        int x = luaL_checknumber(L, 1);
        int y = luaL_checknumber(L, 2);
        int w = luaL_checknumber(L, 3);
        int h = luaL_checknumber(L, 4);
        auto col = string_to_wstring(luaL_checkstring(L, 5));

        Gdiplus::Graphics gfx(lua->rctx.gdi_back_dc);
        Gdiplus::SolidBrush brush(Gdiplus::Color(StrToColorA(col, true)));

        gfx.FillRectangle(&brush, x, y, w, h);

        return 0;
    }

    static int FillRect(lua_State* L)
    {
        auto lua = get_lua_class(L);


        COLORREF color = RGB(
        luaL_checknumber(L, 5),
        luaL_checknumber(L, 6),
        luaL_checknumber(L, 7));
        COLORREF colorold = SetBkColor(lua->rctx.gdi_back_dc, color);
        RECT rect;
        rect.left = luaL_checknumber(L, 1);
        rect.top = luaL_checknumber(L, 2);
        rect.right = luaL_checknumber(L, 3);
        rect.bottom = luaL_checknumber(L, 4);
        ExtTextOut(lua->rctx.gdi_back_dc, 0, 0, ETO_OPAQUE, &rect, L"", 0, 0);
        SetBkColor(lua->rctx.gdi_back_dc, colorold);
        return 0;
    }

    static int DrawEllipse(lua_State* L)
    {
        auto lua = get_lua_class(L);

        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.brush);
        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.pen);

        int left = luaL_checknumber(L, 1);
        int top = luaL_checknumber(L, 2);
        int right = luaL_checknumber(L, 3);
        int bottom = luaL_checknumber(L, 4);

        ::Ellipse(lua->rctx.gdi_back_dc, left, top, right, bottom);
        return 0;
    }

    static int DrawPolygon(lua_State* L)
    {
        auto lua = get_lua_class(L);


        POINT p[0x100];
        luaL_checktype(L, 1, LUA_TTABLE);
        int n = luaL_len(L, 1);
        if (n >= sizeof(p) / sizeof(p[0]))
        {
            lua_pushfstring(L, "wgui.polygon: too many points (%d < %d)", sizeof(p) / sizeof(p[0]), n);
            return lua_error(L);
        }
        for (int i = 0; i < n; i++)
        {
            lua_pushinteger(L, i + 1);
            lua_gettable(L, 1);
            luaL_checktype(L, -1, LUA_TTABLE);
            lua_pushinteger(L, 1);
            lua_gettable(L, -2);
            p[i].x = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_pushinteger(L, 2);
            lua_gettable(L, -2);
            p[i].y = lua_tointeger(L, -1);
            lua_pop(L, 2);
        }
        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.brush);
        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.pen);
        ::Polygon(lua->rctx.gdi_back_dc, p, n);
        return 0;
    }

    static int DrawLine(lua_State* L)
    {
        auto lua = get_lua_class(L);

        SelectObject(lua->rctx.gdi_back_dc, lua->rctx.pen);
        ::MoveToEx(lua->rctx.gdi_back_dc, luaL_checknumber(L, 1), luaL_checknumber(L, 2), NULL);
        ::LineTo(lua->rctx.gdi_back_dc, luaL_checknumber(L, 3), luaL_checknumber(L, 4));
        return 0;
    }

    static int SetClip(lua_State* L)
    {
        auto lua = get_lua_class(L);


        auto rgn = CreateRectRgn(luaL_checkinteger(L, 1),
                                 luaL_checkinteger(L, 2),
                                 luaL_checkinteger(L, 1) +
                                 luaL_checkinteger(L, 3),
                                 luaL_checkinteger(L, 2) + luaL_checkinteger(L, 4));
        SelectClipRgn(lua->rctx.gdi_back_dc, rgn);
        DeleteObject(rgn);
        return 0;
    }

    static int ResetClip(lua_State* L)
    {
        auto lua = get_lua_class(L);

        SelectClipRgn(lua->rctx.gdi_back_dc, NULL);
        return 0;
    }
} // namespace LuaCore::Wgui
