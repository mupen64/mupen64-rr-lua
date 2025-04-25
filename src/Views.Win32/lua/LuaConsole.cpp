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
#include <features/Statusbar.h>
#include <features/FilePicker.h>
#include "presenters/DCompPresenter.h"
#include "presenters/GDIPresenter.h"

constexpr auto LUA_PROP_NAME = L"lua_env";

const auto D2D_OVERLAY_CLASS = L"lua_d2d_overlay";
const auto GDI_OVERLAY_CLASS = L"lua_gdi_overlay";

core_buttons last_controller_data[4];
core_buttons new_controller_data[4];
bool overwrite_controller_data[4];
size_t g_input_count = 0;

std::atomic g_d2d_drawing_section = false;

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

LRESULT CALLBACK d2d_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_PAINT:
        {
            // NOTE: Sometimes, this control receives a WM_PAINT message while execution is already in WM_PAINT, causing us to call begin_present twice in a row...
            // Usually this shouldn't happen, but the shell file dialog API causes this by messing with the parent window's message loop.
            if (g_d2d_drawing_section)
            {
                g_view_logger->warn("Tried to clobber a D2D drawing section!");
                break;
            }

            g_d2d_drawing_section = true;

            auto lua = (t_lua_environment*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            if (!lua)
            {
                g_d2d_drawing_section = false;
                break;
            }

            PAINTSTRUCT ps;
            RECT rect;
            BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);

            bool success;
            if (!lua->presenter)
            {
                // NOTE: We have to invoke the callback because we're waiting for the script to issue a d2d call
                success = LuaCallbacks::invoke_callbacks_with_key(*lua, pcall_no_params, LuaCallbacks::REG_ATDRAWD2D);
            }
            else
            {
                lua->presenter->begin_present();
                success = LuaCallbacks::invoke_callbacks_with_key(*lua, pcall_no_params, LuaCallbacks::REG_ATDRAWD2D);
                lua->presenter->end_present();
            }

            if (!success)
            {
                destroy_lua_environment(lua);
            }

            EndPaint(hwnd, &ps);
            g_d2d_drawing_section = false;
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK gdi_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_PAINT:
        {
            auto lua = (t_lua_environment*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            if (!lua)
            {
                return 0;
            }

            PAINTSTRUCT ps;
            RECT rect;
            HDC dc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);

            const bool success = LuaCallbacks::invoke_callbacks_with_key(*lua, pcall_no_params, LuaCallbacks::REG_ATUPDATESCREEN);

            BitBlt(dc, 0, 0, lua->dc_size.width, lua->dc_size.height, lua->gdi_back_dc, 0, 0, SRCCOPY);

            if (!success)
            {
                destroy_lua_environment(lua);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void lua_init()
{
    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)d2d_overlay_wndproc;
    wndclass.hInstance = g_app_instance;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = D2D_OVERLAY_CLASS;
    RegisterClass(&wndclass);

    wndclass.lpfnWndProc = (WNDPROC)gdi_overlay_wndproc;
    wndclass.lpszClassName = GDI_OVERLAY_CLASS;
    RegisterClass(&wndclass);

    mupen_api_lua_code = get_string_by_textfile_resource_id(IDR_API_LUA_FILE);
    inspect_lua_code = get_string_by_textfile_resource_id(IDR_INSPECT_LUA_FILE);
    shims_lua_code = get_string_by_textfile_resource_id(IDR_SHIMS_LUA_FILE);
}

void create_loadscreen(t_lua_environment* lua)
{
    if (lua->loadscreen_dc)
    {
        return;
    }
    auto gdi_dc = GetDC(g_main_hwnd);
    lua->loadscreen_dc = CreateCompatibleDC(gdi_dc);
    lua->loadscreen_bmp = CreateCompatibleBitmap(gdi_dc, lua->dc_size.width, lua->dc_size.height);
    SelectObject(lua->loadscreen_dc, lua->loadscreen_bmp);
    ReleaseDC(g_main_hwnd, gdi_dc);
}

void destroy_loadscreen(t_lua_environment* lua)
{
    if (!lua->loadscreen_dc)
    {
        return;
    }
    SelectObject(lua->loadscreen_dc, nullptr);
    DeleteDC(lua->loadscreen_dc);
    DeleteObject(lua->loadscreen_bmp);
    lua->loadscreen_dc = nullptr;
}

void ensure_d2d_renderer_created(t_lua_environment* lua)
{
    if (lua->presenter || lua->m_ignore_renderer_creation)
    {
        return;
    }

    g_view_logger->trace("[Lua] Creating D2D renderer...");

    auto hr = CoInitialize(nullptr);
    if (hr != S_OK && hr != S_FALSE && hr != RPC_E_CHANGED_MODE)
    {
        DialogService::show_dialog(L"Failed to initialize COM.\r\nVerify that your system is up-to-date.", L"Lua", fsvc_error);
        return;
    }

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(lua->dw_factory), reinterpret_cast<IUnknown**>(&lua->dw_factory));

    if (g_config.presenter_type != static_cast<int32_t>(PRESENTER_GDI))
    {
        lua->presenter = new DCompPresenter();
    }
    else
    {
        lua->presenter = new GDIPresenter();
    }

    if (!lua->presenter->init(lua->d2d_overlay_hwnd))
    {
        DialogService::show_dialog(L"Failed to initialize presenter.\r\nVerify that your system supports the selected presenter.", L"Lua", fsvc_error);
        return;
    }

    lua->d2d_render_target_stack.push(lua->presenter->dc());
    lua->dw_text_layouts = MicroLRU::Cache<uint64_t, IDWriteTextLayout*>(512, [&](auto value) {
        value->Release();
    });
    lua->dw_text_sizes = MicroLRU::Cache<uint64_t, DWRITE_TEXT_METRICS>(512, [&](auto value) {
    });
}

void create_renderer(t_lua_environment* lua)
{
    if (lua->gdi_back_dc != nullptr || lua->m_ignore_renderer_creation)
    {
        return;
    }

    g_view_logger->info("Creating multi-target renderer for Lua...");

    RECT window_rect;
    GetClientRect(g_main_hwnd, &window_rect);
    if (Statusbar::hwnd())
    {
        // We don't want to paint over statusbar
        RECT rc{};
        GetWindowRect(Statusbar::hwnd(), &rc);
        window_rect.bottom -= (WORD)(rc.bottom - rc.top);
    }

    // NOTE: We don't want negative or zero size on any axis, as that messes up comp surface creation
    lua->dc_size = {(UINT32)std::max(1, (int32_t)window_rect.right), (UINT32)std::max(1, (int32_t)window_rect.bottom)};
    g_view_logger->info("Lua dc size: {} {}", lua->dc_size.width, lua->dc_size.height);

    // Key 0 is reserved for clearing the image pool, too late to change it now...
    lua->image_pool_index = 1;

    auto gdi_dc = GetDC(g_main_hwnd);
    lua->gdi_back_dc = CreateCompatibleDC(gdi_dc);
    lua->gdi_bmp = CreateCompatibleBitmap(gdi_dc, lua->dc_size.width, lua->dc_size.height);
    SelectObject(lua->gdi_back_dc, lua->gdi_bmp);
    ReleaseDC(g_main_hwnd, gdi_dc);

    lua->gdi_overlay_hwnd = CreateWindowEx(WS_EX_LAYERED, GDI_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, lua->dc_size.width, lua->dc_size.height, g_main_hwnd, nullptr, g_app_instance, nullptr);
    SetWindowLongPtr(lua->gdi_overlay_hwnd, GWLP_USERDATA, (LONG_PTR)lua);
    SetLayeredWindowAttributes(lua->gdi_overlay_hwnd, LUA_GDI_COLOR_MASK, 0, LWA_COLORKEY);

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    FillRect(lua->gdi_back_dc, &window_rect, g_alpha_mask_brush);

    lua->d2d_overlay_hwnd = CreateWindowEx(WS_EX_LAYERED, D2D_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, lua->dc_size.width, lua->dc_size.height, g_main_hwnd, nullptr, g_app_instance, nullptr);
    SetWindowLongPtr(lua->d2d_overlay_hwnd, GWLP_USERDATA, (LONG_PTR)lua);

    if (!g_config.lazy_renderer_init)
    {
        ensure_d2d_renderer_created(lua);
    }

    create_loadscreen(lua);
}

void destroy_renderer(t_lua_environment* lua)
{
    g_view_logger->info("Destroying Lua renderer...");

    if (lua->presenter)
    {
        lua->dw_text_layouts.clear();
        lua->dw_text_sizes.clear();

        while (!lua->d2d_render_target_stack.empty())
        {
            lua->d2d_render_target_stack.pop();
        }

        for (auto& [_, val] : lua->image_pool)
        {
            delete val;
        }
        lua->image_pool.clear();

        DestroyWindow(lua->d2d_overlay_hwnd);

        delete lua->presenter;
        lua->presenter = nullptr;
        CoUninitialize();
    }

    if (lua->gdi_back_dc)
    {
        DestroyWindow(lua->gdi_overlay_hwnd);
        SelectObject(lua->gdi_back_dc, nullptr);
        DeleteDC(lua->gdi_back_dc);
        DeleteObject(lua->gdi_bmp);
        lua->gdi_back_dc = nullptr;
        destroy_loadscreen(lua);
    }
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
    lua->m_ignore_renderer_creation = true;
    SetWindowLongPtr(lua->gdi_overlay_hwnd, GWLP_USERDATA, 0);
    SetWindowLongPtr(lua->d2d_overlay_hwnd, GWLP_USERDATA, 0);
    LuaCallbacks::invoke_callbacks_with_key(*lua, pcall_no_params, LuaCallbacks::REG_ATSTOP);

    // NOTE: We must do this *after* calling atstop, as the lua environment still has to exist for that.
    // After this point, it's game over and no callbacks will be called anymore.
    std::erase_if(g_lua_environments, [=](const t_lua_environment* v) {
        return v == lua;
    });
    SetProp(lua->hwnd, LUA_PROP_NAME, nullptr);
    rebuild_lua_env_map();

    SelectObject(lua->gdi_back_dc, nullptr);
    DeleteObject(lua->brush);
    DeleteObject(lua->pen);
    DeleteObject(lua->font);
    lua_close(lua->L);
    lua->L = nullptr;
    set_button_state(lua->hwnd, false);
    destroy_renderer(lua);

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

    lua->hwnd = wnd;
    lua->path = path;

    lua->brush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    lua->pen = static_cast<HPEN>(GetStockObject(BLACK_PEN));
    lua->font = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));
    lua->col = lua->bkcol = 0;
    lua->bkmode = TRANSPARENT;
    lua->L = luaL_newstate();
    lua_atpanic(lua->L, at_panic);
    LuaRegistry::register_functions(lua->L);
    create_renderer(lua);

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

void invalidate_visuals()
{
    assert(is_on_gui_thread());

    for (const auto& lua : g_lua_environments)
    {
        RedrawWindow(lua->d2d_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE);
        RedrawWindow(lua->gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE);
    }
}

void repaint_visuals()
{
    assert(is_on_gui_thread());

    for (const auto& lua : g_lua_environments)
    {
        RedrawWindow(lua->d2d_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        RedrawWindow(lua->gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
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
