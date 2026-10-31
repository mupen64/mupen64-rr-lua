/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <lua/LuaWindow.hpp>
#include <lua/LuaRealmManager.hpp>

namespace
{
constexpr const char *WINDOW_MT = "Mupen.Window";
constexpr const char *WINDOW_CLASS = "MupenLuaWindow";

LuaWindow *check_window(lua_State *L, bool allow_closed = false)
{
    auto *window = *static_cast<LuaWindow **>(luaL_checkudata(L, 1, WINDOW_MT));
    if (window->closed && !allow_closed) luaL_error(L, "window is closed");
    return window;
}

void push_window(lua_State *L, LuaWindow *window)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, window);
    if (!lua_isnil(L, -1)) return;
    lua_pop(L, 1);
    auto **value = static_cast<LuaWindow **>(lua_newuserdata(L, sizeof(LuaWindow *)));
    *value = window;
    luaL_setmetatable(L, WINDOW_MT);
    lua_newtable(L);
    lua_setuservalue(L, -2);
    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, window);
}

int subscribe(lua_State *L, uint8_t key)
{
    check_window(L, key == LuaRealmManager::REG_CLOSED && lua_toboolean(L, 3));
    luaL_checktype(L, 2, LUA_TFUNCTION);
    const bool unregister = lua_toboolean(L, 3);
    lua_getuservalue(L, 1);
    lua_rawgeti(L, -1, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_rawseti(L, -3, key);
    }
    const auto count = lua_rawlen(L, -1);
    if (unregister)
    {
        for (size_t i = 1; i <= count; ++i)
        {
            lua_rawgeti(L, -1, i);
            const bool match = lua_rawequal(L, -1, 2);
            lua_pop(L, 1);
            if (!match) continue;
            for (size_t j = i; j < count; ++j)
            {
                lua_rawgeti(L, -1, j + 1);
                lua_rawseti(L, -2, j);
            }
            lua_pushnil(L);
            lua_rawseti(L, -2, count);
            return 0;
        }
        return luaL_error(L, "callback is not registered on this window");
    }
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, count + 1);
    return 0;
}

int atpaint(lua_State *L)
{
    return subscribe(L, LuaRealmManager::REG_ATPAINT);
}
int atkey(lua_State *L)
{
    return subscribe(L, LuaRealmManager::REG_ATKEY);
}
int atmouse(lua_State *L)
{
    return subscribe(L, LuaRealmManager::REG_ATMOUSE);
}
int closed(lua_State *L)
{
    return subscribe(L, LuaRealmManager::REG_CLOSED);
}

int resize(lua_State *L)
{
    auto *window = check_window(L);
    const auto width = std::clamp<lua_Integer>(luaL_checkinteger(L, 2), 1, 32767);
    const auto height = std::clamp<lua_Integer>(luaL_checkinteger(L, 3), 1, 32767);

    RECT client_rect{}, window_rect{};
    GetClientRect(window->hwnd, &client_rect);
    GetWindowRect(window->hwnd, &window_rect);
    const auto frame_width = (window_rect.right - window_rect.left) - (client_rect.right - client_rect.left);
    const auto frame_height = (window_rect.bottom - window_rect.top) - (client_rect.bottom - client_rect.top);
    if (!SetWindowPos(window->hwnd, nullptr, 0, 0, static_cast<int>(width) + frame_width,
            static_cast<int>(height) + frame_height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE))
        return luaL_error(L, "failed to resize window");
    return 0;
}

bool window_option(lua_State *L, const char *name, bool fallback)
{
    lua_getfield(L, 1, name);
    const bool value = lua_isnil(L, -1) ? fallback : luaL_checkboolean(L, -1);
    lua_pop(L, 1);
    return value;
}
} // namespace

bool LuaWindow::call(uint8_t key, const std::function<int(lua_State *)> &invoke)
{
    if (closed && key != LuaRealmManager::REG_CLOSED) return true;
    auto *L = realm->l();
    const int top = lua_gettop(L);
    lua_rawgetp(L, LUA_REGISTRYINDEX, this);
    if (lua_isnil(L, -1))
    {
        lua_settop(L, top);
        return true;
    }
    lua_getuservalue(L, -1);
    lua_rawgeti(L, -1, key);
    if (lua_isnil(L, -1))
    {
        lua_settop(L, top);
        return true;
    }
    const auto count = lua_rawlen(L, -1);
    lua_newtable(L);
    for (size_t i = 1; i <= count; ++i)
    {
        lua_rawgeti(L, -2, i);
        lua_rawseti(L, -2, i);
    }
    auto *previous = realm->active_renderer;
    realm->active_renderer = renderer ? renderer.get() : &realm->renderer;
    bool success = true;
    for (size_t i = 1; i <= count && (!closed || key == LuaRealmManager::REG_CLOSED); ++i)
    {
        lua_rawgeti(L, -1, i);
        if (invoke(L) == LUA_OK) continue;
        const char *error = lua_tostring(L, -1);
        const std::string message = error ? error : "Lua window callback failed";
        realm->print(realm, message + "\r\n");
        g_view_logger->info("Lua error: {}", message);
        success = false;
        break;
    }
    realm->active_renderer = previous;
    lua_settop(L, top);
    return success;
}

void LuaWindow::reposition()
{
    if (closed || anchor.empty() || IsIconic(hwnd) || IsZoomed(hwnd)) return;
    RECT main_rect{}, rect{};
    GetWindowRect(g_main_ctx.hwnd, &main_rect);
    GetWindowRect(hwnd, &rect);
    int x = main_rect.left, y = main_rect.top;
    if (anchor == "right") x = main_rect.right;
    if (anchor == "left") x -= rect.right - rect.left;
    if (anchor == "bottom") y = main_rect.bottom;
    if (anchor == "top") y -= rect.bottom - rect.top;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool LuaWindow::close()
{
    if (closed) return true;
    closed = true;
    if (renderer) ShowWindow(hwnd, SW_HIDE);
    return call(LuaRealmManager::REG_CLOSED, [](lua_State *L) { return lua_pcall(L, 0, 0, 0); });
}

LRESULT CALLBACK LuaWindow::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto *window = reinterpret_cast<LuaWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE)
    {
        window = static_cast<LuaWindow *>(reinterpret_cast<CREATESTRUCT *>(lparam)->lpCreateParams);
        window->hwnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    if (msg == WM_NCDESTROY)
    {
        handle_lua_window_events(hwnd, msg, wparam, lparam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        if (window)
        {
            const auto realm = window->realm->shared_from_this();
            if (!window->close() && !realm->is_stopping) realm->stop();
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    if (window && !window->closed)
    {
        if (msg == WM_EXITSIZEMOVE) window->reposition();
        if (msg == WM_SIZE && wparam != SIZE_MINIMIZED && window->renderer)
        {
            RECT rect{};
            GetClientRect(hwnd, &rect);
            window->renderer->resize(rect.right - rect.left, rect.bottom - rect.top);
            window->reposition();
        }
        if (msg == WM_CLOSE)
        {
            if (!window->closable) return 0;
            const auto realm = window->realm->shared_from_this();
            if (!window->close()) realm->stop();
            return 0;
        }
        handle_lua_window_events(hwnd, msg, wparam, lparam);
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

namespace LuaCore::Window
{
void register_types(lua_State *L)
{
    static const luaL_Reg methods[] = {{"atpaint", atpaint}, {"atkey", atkey}, {"atmouse", atmouse}, {"closed", closed},
        {"resize", resize}, {nullptr, nullptr}};
    luaL_newmetatable(L, WINDOW_MT);
    luaL_setfuncs(L, methods, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "Window");
    lua_setfield(L, -2, "__metatable");
    lua_pop(L, 1);
}

int main(lua_State *L)
{
    auto *realm = LuaRealmManager::instance().get_by_state(L);
    if (!realm->main_window)
    {
        realm->main_window = std::make_unique<LuaWindow>();
        realm->main_window->realm = realm;
        realm->main_window->hwnd = g_main_ctx.hwnd;
    }
    push_window(L, realm->main_window.get());
    return 1;
}

int create(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "id");
    luaL_checktype(L, -1, LUA_TSTRING);
    size_t id_length{};
    const char *id = lua_tolstring(L, -1, &id_length);
    luaL_argcheck(L, id_length > 0 && strlen(id) == id_length, 1, "id must be nonempty and contain no NUL bytes");
    auto *realm = LuaRealmManager::instance().get_by_state(L);
    if (realm->is_stopping) return luaL_error(L, "cannot create windows while the script is stopping");
    for (const auto &window : realm->windows)
        if (window->id == id) return luaL_error(L, "window id already exists: %s", id);
    lua_getfield(L, 1, "anchor");
    const char *anchor = lua_isnil(L, -1) ? "" : luaL_checkstring(L, -1);
    luaL_argcheck(L,
        !strcmp(anchor, "") || !strcmp(anchor, "left") || !strcmp(anchor, "right") || !strcmp(anchor, "top") ||
            !strcmp(anchor, "bottom"),
        1, "invalid anchor");
    lua_getfield(L, 1, "size");
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_getfield(L, -1, "x");
    const auto width = std::clamp<lua_Integer>(luaL_checkinteger(L, -1), 1, 32767);
    lua_getfield(L, -2, "y");
    const auto height = std::clamp<lua_Integer>(luaL_checkinteger(L, -1), 1, 32767);

    const bool minimizable = window_option(L, "minimizable", true);
    const bool maximizable = window_option(L, "maximizable", false);
    const bool closable = window_option(L, "closable", true);

    WNDCLASS wc{};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = LuaWindow::wndproc;
    wc.hInstance = g_main_ctx.hinst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = WINDOW_CLASS;
    if (!RegisterClass(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return luaL_error(L, "failed to register Lua window class");

    auto window = std::make_unique<LuaWindow>();
    window->realm = realm;
    window->id = id;
    window->anchor = anchor;
    window->closable = closable;
    window->renderer = std::make_unique<LuaRenderer>();
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | (minimizable ? WS_MINIMIZEBOX : 0) |
                        (maximizable ? WS_MAXIMIZEBOX : 0);
    RECT rect{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&rect, style, FALSE);
    window->hwnd = CreateWindowEx(0, WINDOW_CLASS, id, style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
        rect.bottom - rect.top, g_main_ctx.hwnd, nullptr, g_main_ctx.hinst, window.get());
    if (!window->hwnd)
    {
        window.reset();
        return luaL_error(L, "failed to create Lua window");
    }
    if (!closable) EnableMenuItem(GetSystemMenu(window->hwnd, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
    auto *result = window.get();
    realm->windows.push_back(std::move(window));
    result->renderer->initialize(result->hwnd);
    result->reposition();
    push_window(L, result);
    ShowWindow(result->hwnd, SW_SHOWNOACTIVATE);
    return 1;
}
} // namespace LuaCore::Window
