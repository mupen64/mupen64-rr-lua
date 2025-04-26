/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "LuaConsole.h"
#include "Config.h"
#include "DialogService.h"
#include "LuaCallbacks.h"
#include "LuaRegistry.h"
#include "Messenger.h"
#include <Main.h>
#include <features/FilePicker.h>
#include <lua/LuaRenderer.h>

constexpr auto LUA_PROP_NAME = L"lua_env";

core_buttons last_controller_data[4];
core_buttons new_controller_data[4];
bool overwrite_controller_data[4];
size_t g_input_count = 0;

std::vector<t_lua_environment*> g_lua_environments;
std::unordered_map<lua_State*, t_lua_environment*> g_lua_env_map;

std::string mupen_api_lua_code;
std::string inspect_lua_code;
std::string shims_lua_code;

t_lua_environment* get_lua_class(lua_State* lua_state)
{
    if (!g_lua_env_map.contains(lua_state))
    {
        return nullptr;
    }
    return g_lua_env_map[lua_state];
}

int at_panic(lua_State* L)
{
    const auto message = string_to_wstring(lua_tostring(L, -1));

    g_view_logger->info(L"Lua panic: {}", message);
    DialogService::show_dialog(message.c_str(), L"Lua", fsvc_error);

    return 0;
}

void set_button_state(HWND wnd, bool state)
{
    if (!IsWindow(wnd))
        return;
    const HWND state_button = GetDlgItem(wnd, IDC_BUTTON_LUASTATE);
    const HWND stop_button = GetDlgItem(wnd, IDC_BUTTON_LUASTOP);
    if (state)
    {
        SetWindowText(state_button, L"Restart");
        EnableWindow(stop_button, TRUE);
    }
    else
    {
        SetWindowText(state_button, L"Run");
        EnableWindow(stop_button, FALSE);
    }
}

INT_PTR CALLBACK lua_dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto lua = (t_lua_environment*)GetProp(hwnd, LUA_PROP_NAME);

    switch (msg)
    {
    case WM_INITDIALOG:
        SetProp(hwnd, LUA_PROP_NAME, (HANDLE)lparam);
        SetWindowText(GetDlgItem(hwnd, IDC_TEXTBOX_LUASCRIPTPATH), g_config.lua_script_path.c_str());
        return TRUE;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return TRUE;
    case WM_DESTROY:
        if (lua)
        {
            destroy_lua_environment(lua);
        }
        RemoveProp(hwnd, LUA_PROP_NAME);
        return TRUE;
    case WM_SIZE:
        {
            RECT window_rect = {0};
            GetClientRect(hwnd, &window_rect);

            HWND console_hwnd = GetDlgItem(hwnd, IDC_TEXTBOX_LUACONSOLE);
            RECT console_rect = get_window_rect_client_space(hwnd, console_hwnd);
            SetWindowPos(console_hwnd, nullptr, 0, 0, window_rect.right - console_rect.left * 2, window_rect.bottom - console_rect.top, SWP_NOMOVE);

            HWND path_hwnd = GetDlgItem(hwnd, IDC_TEXTBOX_LUASCRIPTPATH);
            RECT path_rect = get_window_rect_client_space(hwnd, path_hwnd);
            SetWindowPos(path_hwnd, nullptr, 0, 0, window_rect.right - console_rect.left * 2, path_rect.bottom - path_rect.top, SWP_NOMOVE);

            if (wparam == SIZE_MINIMIZED)
                SetFocus(g_main_hwnd);

            break;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wparam))
            {
            case IDC_BUTTON_LUASTATE:
                {
                    wchar_t path[MAX_PATH] = {0};
                    GetWindowText(GetDlgItem(hwnd, IDC_TEXTBOX_LUASCRIPTPATH), path, std::size(path));

                    // if already running, delete and erase it (we dont want to overwrite the environment without properly disposing it)
                    if (lua)
                    {
                        destroy_lua_environment(lua);
                    }

                    // now spool up a new one
                    const auto error_msg = create_lua_environment(path, hwnd);
                    Messenger::broadcast(Messenger::Message::ScriptStarted, std::filesystem::path(path));

                    if (!error_msg.empty())
                    {
                        print_con(hwnd, string_to_wstring(error_msg) + L"\r\n");
                    }
                    else
                    {
                        set_button_state(hwnd, true);
                    }

                    return TRUE;
                }
            case IDC_BUTTON_LUASTOP:
                {
                    if (lua)
                    {
                        destroy_lua_environment(lua);
                        set_button_state(hwnd, false);
                    }
                    return TRUE;
                }
            case IDC_BUTTON_LUABROWSE:
                {
                    const auto path = FilePicker::show_open_dialog(L"o_lua", hwnd, L"*.lua");

                    if (path.empty())
                    {
                        break;
                    }

                    SetWindowText(GetDlgItem(hwnd, IDC_TEXTBOX_LUASCRIPTPATH), path.c_str());
                    return TRUE;
                }
            case IDC_BUTTON_LUAEDIT:
                {
                    wchar_t buf[MAX_PATH]{};
                    GetWindowText(GetDlgItem(hwnd, IDC_TEXTBOX_LUASCRIPTPATH), buf, std::size(buf));

                    if (buf == NULL || buf[0] == '\0')
                        return FALSE;

                    ShellExecute(0, 0, buf, 0, 0, SW_SHOW);
                    return TRUE;
                }
            case IDC_BUTTON_LUACLEAR:
                if (GetAsyncKeyState(VK_MENU))
                {
                    // clear path
                    SetWindowText(GetDlgItem(hwnd, IDC_TEXTBOX_LUASCRIPTPATH), L"");
                    return TRUE;
                }

                SetWindowText(GetDlgItem(hwnd, IDC_TEXTBOX_LUACONSOLE), L"");
                return TRUE;
            default:
                break;
            }
            break;
        }
    default:
        break;
    }
    return FALSE;
}

HWND lua_create()
{
    HWND hwnd = CreateDialog(g_app_instance, MAKEINTRESOURCE(IDD_LUAWINDOW), g_main_hwnd, lua_dialog_proc);
    ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}

void lua_create_and_run(const std::wstring& path)
{
    assert(is_on_gui_thread());

    g_view_logger->info("Creating lua window...");
    auto hwnd = lua_create();

    g_view_logger->info("Setting path...");
    // set the textbox content to match the path
    SetWindowText(GetDlgItem(hwnd, IDC_TEXTBOX_LUASCRIPTPATH), path.c_str());

    g_view_logger->info("Simulating run button click...");
    // click run button
    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LUASTATE, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDC_BUTTON_LUASTATE));
}


void lua_close_all_scripts()
{
    assert(is_on_gui_thread());

    // we mutate the map's nodes while iterating, so we have to make a copy
    const auto lua_environments = g_lua_environments;
    for (const auto& lua : lua_environments)
    {
        SendMessage(lua->hwnd, WM_CLOSE, 0, 0);
    }

    assert(g_lua_environments.empty());
}

void lua_stop_all_scripts()
{
    assert(is_on_gui_thread());

    // we mutate the map's nodes while iterating, so we have to make a copy
    const auto lua_environments = g_lua_environments;
    for (const auto& lua : lua_environments)
    {
        SendMessage(lua->hwnd, WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LUASTOP, BN_CLICKED), (LPARAM)GetDlgItem(lua->hwnd, IDC_BUTTON_LUASTOP));
    }

    assert(g_lua_environments.empty());
}

void lua_init()
{
    mupen_api_lua_code = get_string_by_textfile_resource_id(IDR_API_LUA_FILE);
    inspect_lua_code = get_string_by_textfile_resource_id(IDR_INSPECT_LUA_FILE);
    shims_lua_code = get_string_by_textfile_resource_id(IDR_SHIMS_LUA_FILE);
}


static void rebuild_lua_env_map()
{
    g_lua_env_map.clear();
    for (const auto& lua : g_lua_environments)
    {
        g_lua_env_map[lua->L] = lua;
    }
}

void destroy_lua_environment(t_lua_environment* lua)
{
    LuaRenderer::pre_destroy_renderer(&lua->rctx);
    
    LuaCallbacks::invoke_callbacks_with_key(*lua, LuaCallbacks::REG_ATSTOP);

    // NOTE: We must do this *after* calling atstop, as the lua environment still has to exist for that.
    // After this point, it's game over and no callbacks will be called anymore.
    std::erase_if(g_lua_environments, [=](const t_lua_environment* v) {
        return v == lua;
    });
    SetProp(lua->hwnd, LUA_PROP_NAME, nullptr);
    rebuild_lua_env_map();
    
    lua_close(lua->L);
    lua->L = nullptr;
    set_button_state(lua->hwnd, false);
    LuaRenderer::destroy_renderer(&lua->rctx);

    g_view_logger->info("Lua destroyed");
}

void print_con(HWND hwnd, const std::wstring& text)
{
    HWND con_wnd = GetDlgItem(hwnd, IDC_TEXTBOX_LUACONSOLE);

    int length = GetWindowTextLength(con_wnd);
    if (length >= 0x7000)
    {
        SendMessage(con_wnd, EM_SETSEL, 0, length / 2);
        SendMessage(con_wnd, EM_REPLACESEL, false, (LPARAM) "");
        length = GetWindowTextLength(con_wnd);
    }
    SendMessage(con_wnd, EM_SETSEL, length, length);
    SendMessage(con_wnd, EM_REPLACESEL, false, (LPARAM)text.c_str());
}

std::string create_lua_environment(const std::filesystem::path& path, HWND wnd)
{
    assert(is_on_gui_thread());

    auto lua = new t_lua_environment();

    lua->path = path;
    lua->hwnd = wnd;
    lua->rctx = LuaRenderer::default_rendering_context();
    
    lua->L = luaL_newstate();
    lua_atpanic(lua->L, at_panic);
    LuaRegistry::register_functions(lua->L);
    LuaRenderer::create_renderer(&lua->rctx, lua);

    // NOTE: We need to add the lua to the global map already since it may receive callbacks while its executing the global code
    g_lua_environments.push_back(lua);
    SetProp(lua->hwnd, LUA_PROP_NAME, lua);
    rebuild_lua_env_map();

    bool has_error = false;

    {
        ScopeTimer timer("mupenapi.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, mupen_api_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    LuaRegistry::register_functions(lua->L);

    {
        ScopeTimer timer("inspect.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, inspect_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    {
        ScopeTimer timer("shims.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, shims_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    if (luaL_dofile(lua->L, lua->path.string().c_str()))
    {
        has_error = true;
    }

    std::string error_msg;
    if (has_error)
    {
        g_lua_environments.pop_back();
        SetProp(lua->hwnd, LUA_PROP_NAME, nullptr);
        rebuild_lua_env_map();

        error_msg = lua_tostring(lua->L, -1);
        destroy_lua_environment(lua);
        delete lua;
        lua = nullptr;
    }

    return error_msg;
}


void* lua_tocallback(lua_State* L, const int i)
{
    void* key = calloc(1, sizeof(void*));
    lua_pushvalue(L, i);
    lua_pushlightuserdata(L, key);
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    return key;
}

void lua_pushcallback(lua_State* L, void* key)
{
    lua_pushlightuserdata(L, key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    free(key);
    key = nullptr;
}

void lua_freecallback(void* key)
{
    free(key);
}
