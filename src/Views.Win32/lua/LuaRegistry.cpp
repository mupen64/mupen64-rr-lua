/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <lua/LuaRegistry.h>
#include <lua/modules/AVI.h>
#include <lua/modules/Action.h>
#include <lua/modules/Clipboard.h>
#include <lua/modules/D2D.h>
#include <lua/modules/Emu.h>
#include <lua/modules/Global.h>
#include <lua/modules/Hotkey.h>
#include <lua/modules/IOHelper.h>
#include <lua/modules/Input.h>
#include <lua/modules/Joypad.h>
#include <lua/modules/Memory.h>
#include <lua/modules/Movie.h>
#include <lua/modules/Savestate.h>
#include <lua/modules/WGUI.h>

// these begin and end comments help to generate documentation
// please don't remove them

// begin lua funcs
const luaL_Reg GLOBAL_FUNCS[] = {{"print", LuaCore::Global::print},
                                 {"tostringex", LuaCore::Global::tostringexs},
                                 {"stop", LuaCore::Global::StopScript},
                                 {NULL, NULL}};

const luaL_Reg EMU_FUNCS[] = {{"console", LuaCore::Emu::ConsoleWriteLua},
                              {"statusbar", LuaCore::Emu::StatusbarWrite},

                              {"atvi", LuaCore::Emu::subscribe_atvi},
                              {"atupdatescreen", LuaCore::Emu::subscribe_atupdatescreen},
                              {"atdrawd2d", LuaCore::Emu::subscribe_atdrawd2d},
                              {"atinput", LuaCore::Emu::subscribe_atinput},
                              {"atstop", LuaCore::Emu::subscribe_atstop},
                              {"atwindowmessage", LuaCore::Emu::subscribe_atwindowmessage},
                              {"atinterval", LuaCore::Emu::subscribe_atinterval},
                              {"atplaymovie", LuaCore::Emu::subscribe_atplaymovie},
                              {"atstopmovie", LuaCore::Emu::subscribe_atstopmovie},
                              {"atloadstate", LuaCore::Emu::subscribe_atloadstate},
                              {"atsavestate", LuaCore::Emu::subscribe_atsavestate},
                              {"atreset", LuaCore::Emu::subscribe_atreset},
                              {"atseekcompleted", LuaCore::Emu::subscribe_atseekcompleted},
                              {"atwarpmodifystatuschanged", LuaCore::Emu::subscribe_atwarpmodifystatuschanged},

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

                              {"getaddress", LuaCore::Emu::GetAddress},

                              {"screenshot", LuaCore::Emu::Screenshot},

                              {"play_sound", LuaCore::Emu::LuaPlaySound},
                              {"ismainwindowinforeground", LuaCore::Emu::IsMainWindowInForeground},

                              {NULL, NULL}};

const luaL_Reg MEMORY_FUNCS[] = {
    // memory conversion functions
    {"inttofloat", LuaCore::Memory::int_to_float},
    {"inttodouble", LuaCore::Memory::int_to_double},
    {"floattoint", LuaCore::Memory::float_to_int},
    {"doubletoint", LuaCore::Memory::double_to_int},
    {"qwordtonumber", LuaCore::Memory::qword_to_number},

    {"readbyte", LuaCore::Memory::read_byte},
    {"readbytesigned", LuaCore::Memory::read_byte_signed},
    {"readword", LuaCore::Memory::read_word},
    {"readwordsigned", LuaCore::Memory::read_word_signed},
    {"readdword", LuaCore::Memory::read_dword},
    {"readdwordsigned", LuaCore::Memory::read_dword_signed},
    {"readqword", LuaCore::Memory::read_qword},
    {"readqwordsigned", LuaCore::Memory::read_qword_signed},
    {"readfloat", LuaCore::Memory::read_float},
    {"readdouble", LuaCore::Memory::read_double},
    {"readsize", LuaCore::Memory::read_size},

    {"writebyte", LuaCore::Memory::write_byte},
    {"writeword", LuaCore::Memory::write_word},
    {"writedword", LuaCore::Memory::write_dword},
    {"writeqword", LuaCore::Memory::write_qword},
    {"writefloat", LuaCore::Memory::write_float},
    {"writedouble", LuaCore::Memory::write_double},
    {"writesize", LuaCore::Memory::write_size},

    {"recompile", LuaCore::Memory::recompile},
    {"recompilenextall", LuaCore::Memory::recompile_all},

    {NULL, NULL}};

const luaL_Reg WGUI_FUNCS[] = {{"setbrush", LuaCore::Wgui::set_brush},
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
                               {"saveimage", LuaCore::Wgui::save_image},
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

const luaL_Reg D2D_FUNCS[] = {{"create_brush", LuaCore::D2D::create_brush},
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

const luaL_Reg INPUT_FUNCS[] = {{"get", LuaCore::Input::get_keys},
                                {"diff", LuaCore::Input::GetKeyDifference},
                                {"prompt", LuaCore::Input::prompt},
                                {"get_key_name_text", LuaCore::Input::LuaGetKeyNameText},
                                {NULL, NULL}};

const luaL_Reg JOYPAD_FUNCS[] = {{"get", LuaCore::Joypad::lua_get_joypad},
                                 {"set", LuaCore::Joypad::lua_set_joypad},
                                 // OBSOLETE: Cross-module reach
                                 {"count", LuaCore::Emu::GetInputCount},
                                 {NULL, NULL}};

const luaL_Reg MOVIE_FUNCS[] = {{"play", LuaCore::Movie::play},
                                {"stop", LuaCore::Movie::stop},
                                {"get_filename", LuaCore::Movie::GetMovieFilename},
                                {"get_readonly", LuaCore::Movie::GetVCRReadOnly},
                                {"set_readonly", LuaCore::Movie::SetVCRReadOnly},
                                {"begin_seek", LuaCore::Movie::begin_seek},
                                {"stop_seek", LuaCore::Movie::stop_seek},
                                {"is_seeking", LuaCore::Movie::is_seeking},
                                {"get_seek_completion", LuaCore::Movie::get_seek_completion},
                                {"begin_warp_modify", LuaCore::Movie::begin_warp_modify},
                                {NULL, NULL}};

const luaL_Reg SAVESTATE_FUNCS[] = {{"savefile", LuaCore::Savestate::SaveFileSavestate},
                                    {"loadfile", LuaCore::Savestate::LoadFileSavestate},
                                    {"do_file", LuaCore::Savestate::do_file},
                                    {"do_slot", LuaCore::Savestate::do_slot},
                                    {"do_memory", LuaCore::Savestate::do_memory},
                                    {NULL, NULL}};

const luaL_Reg IOHELPER_FUNCS[] = {{"filediag", LuaCore::IOHelper::LuaFileDialog}, {NULL, NULL}};

const luaL_Reg AVI_FUNCS[] = {
    {"startcapture", LuaCore::Avi::StartCapture}, {"stopcapture", LuaCore::Avi::StopCapture}, {NULL, NULL}};

const luaL_Reg HOTKEY_FUNCS[] = {{"prompt", LuaCore::Hotkey::prompt}, {NULL, NULL}};

const luaL_Reg ACTION_FUNCS[] = {{"add", LuaCore::Action::add},
                                 {"remove", LuaCore::Action::remove},
                                 {"associate_hotkey", LuaCore::Action::associate_hotkey},
                                 {"begin_batch_work", LuaCore::Action::begin_batch_work},
                                 {"end_batch_work", LuaCore::Action::end_batch_work},
                                 {"notify_display_name_changed", LuaCore::Action::notify_display_name_changed},
                                 {"notify_enabled_changed", LuaCore::Action::notify_enabled_changed},
                                 {"notify_active_changed", LuaCore::Action::notify_active_changed},
                                 {"get_display_name", LuaCore::Action::get_display_name},
                                 {"get_enabled", LuaCore::Action::get_enabled},
                                 {"get_active", LuaCore::Action::get_active},
                                 {"get_activatability", LuaCore::Action::get_activatability},
                                 {"get_actions_matching_filter", LuaCore::Action::get_actions_matching_filter},
                                 {"invoke", LuaCore::Action::invoke},
                                 {"lock_hotkeys", LuaCore::Action::lock_hotkeys},
                                 {"get_hotkeys_locked", LuaCore::Action::get_hotkeys_locked},
                                 {NULL, NULL}};

const luaL_Reg CLIPBOARD_FUNCS[] = {{"get", LuaCore::Clipboard::get},
                                    {"get_content_type", LuaCore::Clipboard::get_content_type},
                                    {"set", LuaCore::Clipboard::set},
                                    {"clear", LuaCore::Clipboard::clear},
                                    {NULL, NULL}};

// end lua funcs

const std::pair<std::string, lua_CFunction> OVERRIDE_FUNCS[] = {{"os.exit", LuaCore::Global::Exit}};

void register_as_package(lua_State *lua_state, const char *name, const luaL_Reg regs[])
{
    if (name == nullptr)
    {
        const luaL_Reg *p = regs;
        do
        {
            lua_register(lua_state, p->name, p->func);
        } while ((++p)->func);
        return;
    }
    luaL_newlib(lua_state, regs);
    lua_setglobal(lua_state, name);
}

static void register_function(lua_State *L, const std::wstring &name, const lua_CFunction &func)
{
    const auto parts = StrUtils::split_wstring(name, L".") | std::ranges::to<std::vector>();

    RT_ASSERT(parts.size() == 2, L"Accessor invalid");

    const auto namespace_name = IOUtils::to_utf8_string(parts.at(0));
    const auto function_name = IOUtils::to_utf8_string(parts.at(1));

    lua_getglobal(L, namespace_name.c_str());
    lua_pushcfunction(L, func);
    lua_setfield(L, -2, function_name.c_str());
    lua_pop(L, 1);
}

void LuaRegistry::register_functions(lua_State *L)
{
    luaL_openlibs(L);

    register_as_package(L, nullptr, GLOBAL_FUNCS);
    register_as_package(L, "emu", EMU_FUNCS);
    register_as_package(L, "memory", MEMORY_FUNCS);
    register_as_package(L, "wgui", WGUI_FUNCS);
    register_as_package(L, "d2d", D2D_FUNCS);
    register_as_package(L, "input", INPUT_FUNCS);
    register_as_package(L, "joypad", JOYPAD_FUNCS);
    register_as_package(L, "movie", MOVIE_FUNCS);
    register_as_package(L, "savestate", SAVESTATE_FUNCS);
    register_as_package(L, "iohelper", IOHELPER_FUNCS);
    register_as_package(L, "avi", AVI_FUNCS);
    register_as_package(L, "hotkey", HOTKEY_FUNCS);
    register_as_package(L, "action", ACTION_FUNCS);
    register_as_package(L, "clipboard", CLIPBOARD_FUNCS);
    for (const auto &[name, func] : OVERRIDE_FUNCS)
    {
        register_function(L, IOUtils::to_wide_string(name), func);
    }
}
