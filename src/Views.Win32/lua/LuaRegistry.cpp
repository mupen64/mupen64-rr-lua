/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "LuaRegistry.h"
#include <lua/modules/AVI.h>
#include <lua/modules/D2D.h>
#include <lua/modules/Emu.h>
#include <lua/modules/Global.h>
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
const luaL_Reg GLOBAL_FUNCS[] = {
{"print", LuaCore::Global::print},
{"tostringex", LuaCore::Global::tostringexs},
{"stop", LuaCore::Global::StopScript},
{NULL, NULL}};

const luaL_Reg EMU_FUNCS[] = {
{"console", LuaCore::Emu::ConsoleWriteLua},
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
// DEPRECATE: This is completely useless
{"setgfx", LuaCore::Emu::SetGFX},

{"getaddress", LuaCore::Emu::GetAddress},

{"screenshot", LuaCore::Emu::Screenshot},

{"play_sound", LuaCore::Emu::LuaPlaySound},
{"ismainwindowinforeground", LuaCore::Emu::IsMainWindowInForeground},

{NULL, NULL}};

const luaL_Reg MEMORY_FUNCS[] = {
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

{"recompilenow", LuaCore::Memory::Recompile},
{"recompile", LuaCore::Memory::Recompile},
{"recompilenext", LuaCore::Memory::Recompile},
{"recompilenextall", LuaCore::Memory::RecompileNextAll},

{NULL, NULL}};

const luaL_Reg WGUI_FUNCS[] = {
{"setbrush", LuaCore::Wgui::set_brush},
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

const luaL_Reg D2D_FUNCS[] = {
{"create_brush", LuaCore::D2D::create_brush},
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

const luaL_Reg INPUT_FUNCS[] = {
{"get", LuaCore::Input::get_keys},
{"diff", LuaCore::Input::GetKeyDifference},
{"prompt", LuaCore::Input::InputPrompt},
{"get_key_name_text", LuaCore::Input::LuaGetKeyNameText},
{NULL, NULL}};

const luaL_Reg JOYPAD_FUNCS[] = {
{"get", LuaCore::Joypad::lua_get_joypad},
{"set", LuaCore::Joypad::lua_set_joypad},
// OBSOLETE: Cross-module reach
{"count", LuaCore::Emu::GetInputCount},
{NULL, NULL}};

const luaL_Reg MOVIE_FUNCS[] = {
{"play", LuaCore::Movie::PlayMovie},
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

const luaL_Reg SAVESTATE_FUNCS[] = {
{"savefile", LuaCore::Savestate::SaveFileSavestate},
{"loadfile", LuaCore::Savestate::LoadFileSavestate},
{"do_file", LuaCore::Savestate::do_file},
{"do_slot", LuaCore::Savestate::do_slot},
{"do_memory", LuaCore::Savestate::do_memory},
{NULL, NULL}};

const luaL_Reg IOHELPER_FUNCS[] = {
{"filediag", LuaCore::IOHelper::LuaFileDialog},
{NULL, NULL}};

const luaL_Reg AVI_FUNCS[] = {
{"startcapture", LuaCore::Avi::StartCapture},
{"stopcapture", LuaCore::Avi::StopCapture},
{NULL, NULL}};

// end lua funcs

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

void LuaRegistry::register_functions(lua_State* L)
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

    // NOTE: The default os.exit implementation calls C++ destructors before closing the main window (WM_CLOSE + WM_DESTROY),
    // thereby ripping the program apart for the remaining section of time until the exit, which causes extremely unpredictable crashes and an impossible program state.
    lua_getglobal(L, "os");
    lua_pushcfunction(L, LuaCore::Global::Exit);
    lua_setfield(L, -2, "exit");
    lua_pop(L, 1);
}
