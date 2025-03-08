/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "LuaConsole.h"

#include "Config.h"
#include "DialogService.h"
#include "Messenger.h"





#include <gdiplus.h>
#include <gui/Main.h>
#include <gui/features/Statusbar.h>
#include <gui/wrapper/PersistentPathDialog.h>
#include <wincodec.h>
#include "modules/AVI.h"
#include "modules/D2D.h"
#include "modules/Emu.h"
#include "modules/Global.h"
#include "modules/IOHelper.h"
#include "modules/Input.h"
#include "modules/Joypad.h"
#include "modules/Memory.h"
#include "modules/Movie.h"
#include "modules/Savestate.h"
#include "modules/WGUI.h"
#include "presenters/DCompPresenter.h"
#include "presenters/GDIPresenter.h"

const auto D2D_OVERLAY_CLASS = L"lua_d2d_overlay";
const auto GDI_OVERLAY_CLASS = L"lua_gdi_overlay";

core_buttons last_controller_data[4];
core_buttons new_controller_data[4];
bool overwrite_controller_data[4];

std::atomic g_d2d_drawing_section = false;

std::map<HWND, LuaEnvironment*> g_hwnd_lua_map;
std::unordered_map<lua_State*, LuaEnvironment*> g_lua_env_map;

uint64_t inputCount = 0;


int at_panic(lua_State* L)
{
    const auto message = string_to_wstring(lua_tostring(L, -1));

    g_view_logger->info(L"Lua panic: {}", message);
    DialogService::show_dialog(message.c_str(), L"Lua", fsvc_error);

    return 0;
}

void invoke_callbacks_with_key_on_all_instances(const std::function<int(lua_State*)>& function, callback_key key)
{
    // OPTIMIZATION: Store destruction-queued scripts in queue and destroy them after iteration to avoid having to clone the queue
    // OPTIMIZATION: Make the destruction queue static to avoid allocating it every entry 
    static std::queue<LuaEnvironment*> destruction_queue;

    assert(destruction_queue.empty());
    
    for (const auto& [_, env] : g_hwnd_lua_map)
    {
        if (env->invoke_callbacks_with_key(function, key))
        {
            destruction_queue.push(env);
        }
    }

    while (!destruction_queue.empty())
    {
        LuaEnvironment::destroy(destruction_queue.front());
        destruction_queue.pop();
    }
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

INT_PTR CALLBACK DialogProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            SetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH), g_config.lua_script_path.c_str());
            return TRUE;
        }
    case WM_CLOSE:
        DestroyWindow(wnd);
        return TRUE;
    case WM_DESTROY:
        {
            if (g_hwnd_lua_map.contains(wnd))
            {
                LuaEnvironment::destroy(g_hwnd_lua_map[wnd]);
            }
            return TRUE;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON_LUASTATE:
                {
                    wchar_t path[MAX_PATH] = {0};
                    GetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH), path, std::size(path));

                    // if already running, delete and erase it (we dont want to overwrite the environment without properly disposing it)
                    if (g_hwnd_lua_map.contains(wnd))
                    {
                        LuaEnvironment::destroy(g_hwnd_lua_map[wnd]);
                    }

                    // now spool up a new one
                    const auto error_msg = LuaEnvironment::create(path, wnd);
                    Messenger::broadcast(Messenger::Message::ScriptStarted, std::filesystem::path(path));

                    if (!error_msg.empty())
                    {
                        LuaEnvironment::print_con(wnd, string_to_wstring(error_msg) + L"\r\n");
                    }
                    else
                    {
                        set_button_state(wnd, true);
                    }

                    return TRUE;
                }
            case IDC_BUTTON_LUASTOP:
                {
                    if (g_hwnd_lua_map.contains(wnd))
                    {
                        LuaEnvironment::destroy(g_hwnd_lua_map[wnd]);
                        set_button_state(wnd, false);
                    }
                    return TRUE;
                }
            case IDC_BUTTON_LUABROWSE:
                {
                    const auto path = show_persistent_open_dialog(L"o_lua", wnd, L"*.lua");

                    if (path.empty())
                    {
                        break;
                    }

                    SetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH), path.c_str());
                    return TRUE;
                }
            case IDC_BUTTON_LUAEDIT:
                {
                    wchar_t buf[MAX_PATH]{};
                    GetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH), buf, std::size(buf));

                    if (buf == NULL || buf[0] == '\0')
                        return FALSE;

                    ShellExecute(0, 0, buf, 0, 0, SW_SHOW);
                    return TRUE;
                }
            case IDC_BUTTON_LUACLEAR:
                if (GetAsyncKeyState(VK_MENU))
                {
                    // clear path
                    SetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH), L"");
                    return TRUE;
                }

                SetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUACONSOLE), L"");
                return TRUE;
            default:
                break;
            }
        }
    case WM_SIZE:
        {
            RECT window_rect = {0};
            GetClientRect(wnd, &window_rect);

            HWND console_hwnd = GetDlgItem(wnd, IDC_TEXTBOX_LUACONSOLE);
            RECT console_rect = get_window_rect_client_space(wnd, console_hwnd);
            SetWindowPos(console_hwnd, nullptr, 0, 0, window_rect.right - console_rect.left * 2, window_rect.bottom - console_rect.top, SWP_NOMOVE);

            HWND path_hwnd = GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH);
            RECT path_rect = get_window_rect_client_space(wnd, path_hwnd);
            SetWindowPos(path_hwnd, nullptr, 0, 0, window_rect.right - console_rect.left * 2, path_rect.bottom - path_rect.top, SWP_NOMOVE);
            if (wParam == SIZE_MINIMIZED)
                SetFocus(g_main_hwnd);
            break;
        }
    }
    return FALSE;
}

HWND lua_create()
{
    HWND hwnd = CreateDialogParam(g_app_instance, MAKEINTRESOURCE(IDD_LUAWINDOW), g_main_hwnd, DialogProc, NULL);
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

LuaEnvironment* get_lua_class(lua_State* lua_state) { return g_lua_env_map[lua_state]; }

int RegisterFunction(lua_State* L, callback_key key)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_rawseti(L, LUA_REGISTRYINDEX, key);
        lua_rawgeti(L, LUA_REGISTRYINDEX, key);
    }
    int i = luaL_len(L, -1) + 1;
    lua_pushinteger(L, i);
    lua_pushvalue(L, -3); //
    lua_settable(L, -3);
    lua_pop(L, 1);
    return i;
}

void UnregisterFunction(lua_State* L, callback_key key)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L); // �Ƃ肠����
    }
    int n = luaL_len(L, -1);
    for (LUA_INTEGER i = 0; i < n; i++)
    {
        lua_pushinteger(L, 1 + i);
        lua_gettable(L, -2);
        if (lua_rawequal(L, -1, -3))
        {
            lua_pop(L, 1);
            lua_getglobal(L, "table");
            lua_getfield(L, -1, "remove");
            lua_pushvalue(L, -3);
            lua_pushinteger(L, 1 + i);
            lua_call(L, 2, 0);
            lua_pop(L, 2);
            return;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_pushfstring(L, "unregisterFunction(%s): not found function", key);
    lua_error(L);
}

// these begin and end comments help to generate documentation
// please don't remove them

// begin lua funcs
const luaL_Reg globalFuncs[] = {{"print", LuaCore::Global::Print}, {"printx", LuaCore::Global::PrintX}, {"tostringex", LuaCore::Global::ToStringExs}, {"stop", LuaCore::Global::StopScript}, {NULL, NULL}};

const luaL_Reg emuFuncs[] = {{"console", LuaCore::Emu::ConsoleWriteLua},
                             {"statusbar", LuaCore::Emu::StatusbarWrite},

                             {"atvi", LuaCore::Emu::RegisterVI},
                             {"atupdatescreen", LuaCore::Emu::RegisterUpdateScreen},
                             {"atdrawd2d", LuaCore::Emu::RegisterAtDrawD2D},
                             {"atinput", LuaCore::Emu::RegisterInput},
                             {"atstop", LuaCore::Emu::RegisterStop},
                             {"atwindowmessage", LuaCore::Emu::RegisterWindowMessage},
                             {"atinterval", LuaCore::Emu::RegisterInterval},
                             {"atplaymovie", LuaCore::Emu::RegisterPlayMovie},
                             {"atstopmovie", LuaCore::Emu::RegisterStopMovie},
                             {"atloadstate", LuaCore::Emu::RegisterLoadState},
                             {"atsavestate", LuaCore::Emu::RegisterSaveState},
                             {"atreset", LuaCore::Emu::RegisterReset},
                             {"atseekcompleted", LuaCore::Emu::RegisterSeekCompleted},
                             {"atwarpmodifystatuschanged", LuaCore::Emu::RegisterWarpModifyStatusChanged},

                             {"framecount", LuaCore::Emu::GetVICount},
                             {"samplecount", LuaCore::Emu::GetSampleCount},
                             {"inputcount", LuaCore::Emu::GetInputCount},

                             // DEPRECATE: This is completely useless
                             {"getversion", LuaCore::Emu::GetMupenVersion},

                             {"pause", LuaCore::Emu::EmuPause},
                             {"getpause", LuaCore::Emu::GetEmuPause},
                             {"getspeed", LuaCore::Emu::GetSpeed},
                             {"get_ff", LuaCore::Emu::GetFastForward},
                             {"set_ff", LuaCore::Emu::SetFastForward},
                             {"speed", LuaCore::Emu::SetSpeed},
                             {"speedmode", LuaCore::Emu::SetSpeedMode},
                             // DEPRECATE: This is completely useless
                             {"setgfx", LuaCore::Emu::SetGFX},

                             {"getaddress", LuaCore::Emu::GetAddress},

                             {"screenshot", LuaCore::Emu::Screenshot},

#pragma region WinAPI
                             {"play_sound", LuaCore::Emu::LuaPlaySound},
#pragma endregion
                             {"ismainwindowinforeground", LuaCore::Emu::IsMainWindowInForeground},

                             {NULL, NULL}};
const luaL_Reg memoryFuncs[] = {
// memory conversion functions
{"inttofloat", LuaCore::Memory::LuaIntToFloat},
{"inttodouble", LuaCore::Memory::LuaIntToDouble},
{"floattoint", LuaCore::Memory::LuaFloatToInt},
{"doubletoint", LuaCore::Memory::LuaDoubleToInt},
{"qwordtonumber", LuaCore::Memory::LuaQWordToNumber},

// word = 2 bytes
// reading functions
{"readbytesigned", LuaCore::Memory::LuaReadByteSigned},
{"readbyte", LuaCore::Memory::LuaReadByteUnsigned},
{"readwordsigned", LuaCore::Memory::LuaReadWordSigned},
{"readword", LuaCore::Memory::LuaReadWordUnsigned},
{"readdwordsigned", LuaCore::Memory::LuaReadDWordSigned},
{"readdword", LuaCore::Memory::LuaReadDWorldUnsigned},
{"readqwordsigned", LuaCore::Memory::LuaReadQWordSigned},
{"readqword", LuaCore::Memory::LuaReadQWordUnsigned},
{"readfloat", LuaCore::Memory::LuaReadFloat},
{"readdouble", LuaCore::Memory::LuaReadDouble},
{"readsize", LuaCore::Memory::LuaReadSize},

// writing functions
// all of these are assumed to be unsigned
{"writebyte", LuaCore::Memory::LuaWriteByteUnsigned},
{"writeword", LuaCore::Memory::LuaWriteWordUnsigned},
{"writedword", LuaCore::Memory::LuaWriteDWordUnsigned},
{"writeqword", LuaCore::Memory::LuaWriteQWordUnsigned},
{"writefloat", LuaCore::Memory::LuaWriteFloatUnsigned},
{"writedouble", LuaCore::Memory::LuaWriteDoubleUnsigned},

{"writesize", LuaCore::Memory::LuaWriteSize},

{NULL, NULL}};

const luaL_Reg wguiFuncs[] = {{"setbrush", LuaCore::Wgui::set_brush},
                              {"setpen", LuaCore::Wgui::set_pen},
                              {"setcolor", LuaCore::Wgui::set_text_color},
                              {"setbk", LuaCore::Wgui::SetBackgroundColor},
                              {"setfont", LuaCore::Wgui::SetFont},
                              {"text", LuaCore::Wgui::LuaTextOut},
                              {"drawtext", LuaCore::Wgui::LuaDrawText},
                              {"drawtextalt", LuaCore::Wgui::LuaDrawTextAlt},
                              {"gettextextent", LuaCore::Wgui::GetTextExtent},
                              {"rect", LuaCore::Wgui::DrawRect},
                              {"fillrect", LuaCore::Wgui::FillRect},
                              /*<GDIPlus>*/
                              // GDIPlus functions marked with "a" suffix
                              {"fillrecta", LuaCore::Wgui::FillRectAlpha},
                              {"fillellipsea", LuaCore::Wgui::FillEllipseAlpha},
                              {"fillpolygona", LuaCore::Wgui::FillPolygonAlpha},
                              {"loadimage", LuaCore::Wgui::LuaLoadImage},
                              {"deleteimage", LuaCore::Wgui::DeleteImage},
                              {"drawimage", LuaCore::Wgui::DrawImage},
                              {"loadscreen", LuaCore::Wgui::LoadScreen},
                              {"loadscreenreset", LuaCore::Wgui::LoadScreenReset},
                              {"getimageinfo", LuaCore::Wgui::GetImageInfo},
                              /*</GDIPlus*/
                              {"ellipse", LuaCore::Wgui::DrawEllipse},
                              {"polygon", LuaCore::Wgui::DrawPolygon},
                              {"line", LuaCore::Wgui::DrawLine},
                              {"info", LuaCore::Wgui::GetGUIInfo},
                              {"resize", LuaCore::Wgui::ResizeWindow},
                              {"setclip", LuaCore::Wgui::SetClip},
                              {"resetclip", LuaCore::Wgui::ResetClip},
                              {NULL, NULL}};

const luaL_Reg d2dFuncs[] = {{"create_brush", LuaCore::D2D::create_brush},
                             {"free_brush", LuaCore::D2D::free_brush},

                             {"clear", LuaCore::D2D::clear},
                             {"fill_rectangle", LuaCore::D2D::fill_rectangle},
                             {"draw_rectangle", LuaCore::D2D::draw_rectangle},
                             {"fill_ellipse", LuaCore::D2D::fill_ellipse},
                             {"draw_ellipse", LuaCore::D2D::draw_ellipse},
                             {"draw_line", LuaCore::D2D::draw_line},
                             {"draw_text", LuaCore::D2D::draw_text},
                             {"get_text_size", LuaCore::D2D::measure_text},
                             {"push_clip", LuaCore::D2D::push_clip},
                             {"pop_clip", LuaCore::D2D::pop_clip},
                             {"fill_rounded_rectangle", LuaCore::D2D::fill_rounded_rectangle},
                             {"draw_rounded_rectangle", LuaCore::D2D::draw_rounded_rectangle},
                             {"load_image", LuaCore::D2D::load_image},
                             {"free_image", LuaCore::D2D::free_image},
                             {"draw_image", LuaCore::D2D::draw_image},
                             {"get_image_info", LuaCore::D2D::get_image_info},
                             {"set_text_antialias_mode", LuaCore::D2D::set_text_antialias_mode},
                             {"set_antialias_mode", LuaCore::D2D::set_antialias_mode},

                             {"draw_to_image", LuaCore::D2D::draw_to_image},
                             {NULL, NULL}};

const luaL_Reg inputFuncs[] = {{"get", LuaCore::Input::get_keys}, {"diff", LuaCore::Input::GetKeyDifference}, {"prompt", LuaCore::Input::InputPrompt}, {"get_key_name_text", LuaCore::Input::LuaGetKeyNameText}, {NULL, NULL}};
const luaL_Reg joypadFuncs[] = {{"get", LuaCore::Joypad::lua_get_joypad},
                                {"set", LuaCore::Joypad::lua_set_joypad},
                                // OBSOLETE: Cross-module reach
                                {"count", LuaCore::Emu::GetInputCount},
                                {NULL, NULL}};

const luaL_Reg movieFuncs[] = {{"play", LuaCore::Movie::PlayMovie},
                               {"stop", LuaCore::Movie::StopMovie},
                               {"get_filename", LuaCore::Movie::GetMovieFilename},
                               {"get_readonly", LuaCore::Movie::GetVCRReadOnly},
                               {"set_readonly", LuaCore::Movie::SetVCRReadOnly},
                               {"begin_seek", LuaCore::Movie::begin_seek},
                               {"stop_seek", LuaCore::Movie::stop_seek},
                               {"is_seeking", LuaCore::Movie::is_seeking},
                               {"get_seek_completion", LuaCore::Movie::get_seek_completion},
                               {"begin_warp_modify", LuaCore::Movie::begin_warp_modify},
                               {NULL, NULL}};

const luaL_Reg savestateFuncs[] = {{"savefile", LuaCore::Savestate::SaveFileSavestate}, {"loadfile", LuaCore::Savestate::LoadFileSavestate}, {NULL, NULL}};
const luaL_Reg iohelperFuncs[] = {{"filediag", LuaCore::IOHelper::LuaFileDialog}, {NULL, NULL}};
const luaL_Reg aviFuncs[] = {{"startcapture", LuaCore::Avi::StartCapture}, {"stopcapture", LuaCore::Avi::StopCapture}, {NULL, NULL}};
// end lua funcs

void close_all_scripts()
{
    assert(is_on_gui_thread());

    // we mutate the map's nodes while iterating, so we have to make a copy
    auto copy = std::map(g_hwnd_lua_map);
    for (const auto [fst, _] : copy)
    {
        SendMessage(fst, WM_CLOSE, 0, 0);
    }
    assert(g_hwnd_lua_map.empty());
}

void stop_all_scripts()
{
    assert(is_on_gui_thread());

    // we mutate the map's nodes while iterating, so we have to make a copy
    auto copy = std::map(g_hwnd_lua_map);
    for (const auto [key, _] : copy)
    {
        SendMessage(key, WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LUASTOP, BN_CLICKED), (LPARAM)GetDlgItem(key, IDC_BUTTON_LUASTOP));
    }
    assert(g_hwnd_lua_map.empty());
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

            auto lua = (LuaEnvironment*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            if (!lua)
            {
                g_d2d_drawing_section = false;
                return 0;
            }
        
            PAINTSTRUCT ps;
            RECT rect;
            BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);

            bool failed;
            if (!lua->presenter)
            {
                failed = lua->invoke_callbacks_with_key(pcall_no_params, REG_ATDRAWD2D);
            }
            else
            {
                lua->presenter->begin_present();
                failed = lua->invoke_callbacks_with_key(pcall_no_params, REG_ATDRAWD2D);
                lua->presenter->end_present();
            }

            if (failed)
            {
                LuaEnvironment::destroy(lua);
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
            auto lua = (LuaEnvironment*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            if (!lua)
            {
                return 0;
            }
        
            PAINTSTRUCT ps;
            RECT rect;
            HDC dc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);

            bool failed = lua->invoke_callbacks_with_key(pcall_no_params, REG_ATUPDATESCREEN);

            BitBlt(dc, 0, 0, lua->dc_size.width, lua->dc_size.height, lua->gdi_back_dc, 0, 0, SRCCOPY);

            if (failed)
            {
                LuaEnvironment::destroy(lua);
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
}

void LuaEnvironment::create_loadscreen()
{
    if (loadscreen_dc)
    {
        return;
    }
    auto gdi_dc = GetDC(g_main_hwnd);
    loadscreen_dc = CreateCompatibleDC(gdi_dc);
    loadscreen_bmp = CreateCompatibleBitmap(gdi_dc, dc_size.width, dc_size.height);
    SelectObject(loadscreen_dc, loadscreen_bmp);
    ReleaseDC(g_main_hwnd, gdi_dc);
}

void LuaEnvironment::destroy_loadscreen()
{
    if (!loadscreen_dc)
    {
        return;
    }
    SelectObject(loadscreen_dc, nullptr);
    DeleteDC(loadscreen_dc);
    DeleteObject(loadscreen_bmp);
    loadscreen_dc = nullptr;
}

void LuaEnvironment::ensure_d2d_renderer_created()
{
    if (presenter || m_ignore_renderer_creation)
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

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(dw_factory), reinterpret_cast<IUnknown**>(&dw_factory));


    if (g_config.presenter_type != static_cast<int32_t>(PRESENTER_GDI))
    {
        presenter = new DCompPresenter();
    }
    else
    {
        presenter = new GDIPresenter();
    }

    if (!presenter->init(d2d_overlay_hwnd))
    {
        DialogService::show_dialog(L"Failed to initialize presenter.\r\nVerify that your system supports the selected presenter.", L"Lua", fsvc_error);
        return;
    }

    d2d_render_target_stack.push(presenter->dc());
    dw_text_layouts = MicroLRU::Cache<uint64_t, IDWriteTextLayout*>(512, [&](auto value) { 
        value->Release(); 
    });
}

void LuaEnvironment::create_renderer()
{
    if (gdi_back_dc != nullptr || m_ignore_renderer_creation)
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
    dc_size = {(UINT32)std::max(1, (int32_t)window_rect.right), (UINT32)std::max(1, (int32_t)window_rect.bottom)};
    g_view_logger->info("Lua dc size: {} {}", dc_size.width, dc_size.height);

    // Key 0 is reserved for clearing the image pool, too late to change it now...
    image_pool_index = 1;

    auto gdi_dc = GetDC(g_main_hwnd);
    gdi_back_dc = CreateCompatibleDC(gdi_dc);
    gdi_bmp = CreateCompatibleBitmap(gdi_dc, dc_size.width, dc_size.height);
    SelectObject(gdi_back_dc, gdi_bmp);
    ReleaseDC(g_main_hwnd, gdi_dc);

    gdi_overlay_hwnd = CreateWindowEx(WS_EX_LAYERED, GDI_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, dc_size.width, dc_size.height, g_main_hwnd, nullptr, g_app_instance, nullptr);
    SetWindowLongPtr(gdi_overlay_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    SetLayeredWindowAttributes(gdi_overlay_hwnd, lua_gdi_color_mask, 0, LWA_COLORKEY);

    // If we don't fill up the DC with the key first, it never becomes "transparent"
    FillRect(gdi_back_dc, &window_rect, alpha_mask_brush);

    d2d_overlay_hwnd = CreateWindowEx(WS_EX_LAYERED, D2D_OVERLAY_CLASS, L"", WS_CHILD | WS_VISIBLE, 0, 0, dc_size.width, dc_size.height, g_main_hwnd, nullptr, g_app_instance, nullptr);
    SetWindowLongPtr(d2d_overlay_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    if (!g_config.lazy_renderer_init)
    {
        ensure_d2d_renderer_created();
    }

    create_loadscreen();
}

void LuaEnvironment::destroy_renderer()
{
    g_view_logger->info("Destroying Lua renderer...");

    if (presenter)
    {
        dw_text_layouts.clear();

        while (!d2d_render_target_stack.empty())
        {
            d2d_render_target_stack.pop();
        }

        for (auto& [_, val] : image_pool)
        {
            delete val;
        }
        image_pool.clear();

        DestroyWindow(d2d_overlay_hwnd);

        delete presenter;
        presenter = nullptr;
        CoUninitialize();
    }

    if (gdi_back_dc)
    {
        DestroyWindow(gdi_overlay_hwnd);
        SelectObject(gdi_back_dc, nullptr);
        DeleteDC(gdi_back_dc);
        DeleteObject(gdi_bmp);
        gdi_back_dc = nullptr;
        destroy_loadscreen();
    }
}

void LuaEnvironment::destroy(LuaEnvironment* lua_environment)
{
    g_hwnd_lua_map.erase(lua_environment->hwnd);
    delete lua_environment;
}

void LuaEnvironment::print_con(HWND hwnd, const std::wstring& text)
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

void rebuild_lua_env_map()
{
    g_lua_env_map.clear();
    for (const auto& [_, val] : g_hwnd_lua_map)
    {
        g_lua_env_map[val->L] = val;
    }
}

std::string LuaEnvironment::create(const std::filesystem::path& path, HWND wnd)
{
    assert(is_on_gui_thread());

    auto lua_environment = new LuaEnvironment();

    lua_environment->hwnd = wnd;
    lua_environment->path = path;

    lua_environment->brush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    lua_environment->pen = static_cast<HPEN>(GetStockObject(BLACK_PEN));
    lua_environment->font = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));
    lua_environment->col = lua_environment->bkcol = 0;
    lua_environment->bkmode = TRANSPARENT;
    lua_environment->L = luaL_newstate();
    lua_atpanic(lua_environment->L, at_panic);
    lua_environment->register_functions();
    lua_environment->create_renderer();

    // NOTE: We need to add the lua to the global map already since it may receive callbacks while its executing the global code
    g_hwnd_lua_map[lua_environment->hwnd] = lua_environment;
    rebuild_lua_env_map();

    bool has_error = luaL_dofile(lua_environment->L, lua_environment->path.string().c_str());

    std::string error_msg;
    if (has_error)
    {
        g_hwnd_lua_map.erase(lua_environment->hwnd);
        rebuild_lua_env_map();
        error_msg = lua_tostring(lua_environment->L, -1);
        delete lua_environment;
        lua_environment = nullptr;
    }

    return error_msg;
}

LuaEnvironment::~LuaEnvironment()
{
    m_ignore_renderer_creation = true;
    SetWindowLongPtr(gdi_overlay_hwnd, GWLP_USERDATA, 0);
    SetWindowLongPtr(d2d_overlay_hwnd, GWLP_USERDATA, 0);
    invoke_callbacks_with_key(pcall_no_params, REG_ATSTOP);
    SelectObject(gdi_back_dc, nullptr);
    DeleteObject(brush);
    DeleteObject(pen);
    DeleteObject(font);
    lua_close(L);
    L = NULL;
    set_button_state(hwnd, false);
    this->destroy_renderer();
    g_view_logger->info("Lua destroyed");
}

// calls all functions that lua script has defined as callbacks, reads them from registry
// returns true at fail
bool LuaEnvironment::invoke_callbacks_with_key(const std::function<int(lua_State*)>& function, callback_key key)
{
    assert(is_on_gui_thread());

    lua_rawgeti(L, LUA_REGISTRYINDEX, key);
    // shouldn't ever happen but could cause kernel panic
    if lua_isnil (L, -1)
    {
        lua_pop(L, 1);
        return false;
    }
    int n = luaL_len(L, -1);
    for (LUA_INTEGER i = 0; i < n; i++)
    {
        lua_pushinteger(L, 1 + i);
        lua_gettable(L, -2);
        if (function(L))
        {
            const char* str = lua_tostring(L, -1);
            this->print(string_to_wstring(str) + L"\r\n");
            g_view_logger->info("Lua error: {}", str);
            return true;
        }
    }
    lua_pop(L, 1);
    return false;
}

void LuaEnvironment::invalidate_visuals()
{
    RECT rect;
    GetClientRect(this->d2d_overlay_hwnd, &rect);

    InvalidateRect(this->d2d_overlay_hwnd, &rect, false);
    InvalidateRect(this->gdi_overlay_hwnd, &rect, false);
}

void LuaEnvironment::repaint_visuals()
{
    RedrawWindow(this->d2d_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    RedrawWindow(this->gdi_overlay_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

void register_as_package(lua_State* lua_state, const char* name, const luaL_Reg regs[])
{
    if (name == nullptr)
    {
        const luaL_Reg* p = regs;
        do
        {
            lua_register(lua_state, p->name, p->func);
        }
        while ((++p)->func);
        return;
    }
    luaL_newlib(lua_state, regs);
    lua_setglobal(lua_state, name);
}

void LuaEnvironment::register_functions()
{
    luaL_openlibs(L);

    register_as_package(L, nullptr, globalFuncs);
    register_as_package(L, "emu", emuFuncs);
    register_as_package(L, "memory", memoryFuncs);
    register_as_package(L, "wgui", wguiFuncs);
    register_as_package(L, "d2d", d2dFuncs);
    register_as_package(L, "input", inputFuncs);
    register_as_package(L, "joypad", joypadFuncs);
    register_as_package(L, "movie", movieFuncs);
    register_as_package(L, "savestate", savestateFuncs);
    register_as_package(L, "iohelper", iohelperFuncs);
    register_as_package(L, "avi", aviFuncs);

    // NOTE: The default os.exit implementation calls C++ destructors before closing the main window (WM_CLOSE + WM_DESTROY),
    // thereby ripping the program apart for the remaining section of time until the exit, which causes extremely unpredictable crashes and an impossible program state.
    lua_getglobal(L, "os");
    lua_pushcfunction(L, LuaCore::Global::Exit);
    lua_setfield(L, -2, "exit");
    lua_pop(L, 1);
    
    // COMPAT: table.getn deprecated, replaced by # prefix
    luaL_dostring(L, "table.getn = function(t) return #t end");

    // COMPAT: emu.debugview deprecated, forwarded to print
    luaL_dostring(L, "emu.debugview = print");

    // COMPAT: movie.playmovie deprecated, forwarded to movie.play
    luaL_dostring(L, "movie.playmovie = movie.play");

    // COMPAT: movie.stopmovie deprecated, forwarded to movie.stop
    luaL_dostring(L, "movie.stopmovie = movie.stop");

    // COMPAT: movie.getmoviefilename deprecated, forwarded to movie.get_filename
    luaL_dostring(L, "movie.getmoviefilename = movie.get_filename");

    // COMPAT: movie.isreadonly deprecated, forwarded to movie.get_readonly
    luaL_dostring(L, "movie.isreadonly = movie.get_readonly");

    // COMPAT: emu.isreadonly deprecated, forwarded to movie.get_readonly
    luaL_dostring(L, "emu.isreadonly = movie.get_readonly");

    // DEPRECATED: input.map_virtual_key_ex couples to WinAPI
    luaL_dostring(L, "input.map_virtual_key_ex = function() print('input.map_virtual_key_ex has been deprecated') end");

    // DEPRECATED: emu.getsystemmetrics couples to WinAPI
    luaL_dostring(L, "emu.getsystemmetrics = function() print('emu.getsystemmetrics has been deprecated') end");

    // DEPRECATED: movie.begin_seek_to doesn't exist anymore
    luaL_dostring(L, "movie.begin_seek_to = function() print('movie.begin_seek_to has been deprecated, use movie.begin_seek instead') end");

    // DEPRECATED: movie.get_seek_info doesn't exist anymore
    luaL_dostring(L, "movie.get_seek_info = function() print('movie.get_seek_info has been deprecated, use movie.begin_seek instead') end");

    // os.execute poses security risks
    luaL_dostring(L, "os.execute = function() print('os.execute is disabled') end");
}
