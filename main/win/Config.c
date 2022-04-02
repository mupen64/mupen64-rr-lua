/***************************************************************************
                          config.c  -  description
                             -------------------
    copyright            : (C) 2003 by ShadowPrince
    email                : shadow@emulation64.com
 ***************************************************************************/

 /***************************************************************************
  *                                                                         *
  *   This program is free software; you can redistribute it and/or modify  *
  *   it under the terms of the GNU General Public License as published by  *
  *   the Free Software Foundation; either version 2 of the License, or     *
  *   (at your option) any later version.                                   *
  *                                                                         *
  ***************************************************************************/

  /*
  #if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
  #undef _WIN32_WINNT
  #define _WIN32_WINNT 0x0500
  #endif
  */
#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include "config.h"
#include "main_win.h"
#include "rombrowser.h"
#include "commandline.h"
#include "../../winproject/resource.h"

#include "../lua/Recent.h"
#include <vcr.h>

#define CfgFileName "mupen64.cfg"

extern int no_audio_delay;
extern int no_compiled_jump;

int input_delay;
int LUA_double_buffered;

////////////////////// Service functions and structures ////////////////////////

CONFIG Config;

// is this the best way to handle this?
int* autoinc_save_slot = &Config.AutoIncSaveSlot;

char* CfgFilePath()
{
    static char* cfgpath = NULL;
    if (cfgpath == NULL)
    {
        cfgpath = (char*)malloc(strlen(AppPath) + 1 + strlen(CfgFileName));
        strcpy(cfgpath, AppPath);
        strcat(cfgpath, CfgFileName);
    }
    return cfgpath;
}


void WriteCfgString(char* Section, char* Key, char* Value)
{
    WritePrivateProfileString(Section, Key, Value, CfgFilePath());
}


void WriteCfgInt(char* Section, char* Key, int Value)
{
    static char TempStr[20];
    sprintf(TempStr, "%d", Value);
    WriteCfgString(Section, Key, TempStr);
}


void ReadCfgString(char* Section, char* Key, char* DefaultValue, char* retValue)
{
    GetPrivateProfileString(Section, Key, DefaultValue, retValue, MAX_PATH, CfgFilePath());
}


int ReadCfgInt(char* Section, char* Key, int DefaultValue)
{
    return GetPrivateProfileInt(Section, Key, DefaultValue, CfgFilePath());
}

//////////////////////////// Load and Save Config //////////////////////////////

void LoadRecentRoms()
{
    int i;
    char tempStr[32];

    Config.RecentRomsFreeze = ReadCfgInt("Recent Roms", "Freeze", 0);
    for (i = 0; i < MAX_RECENT_ROMS; i++)
    {

        sprintf(tempStr, "RecentRom%d", i);
        ReadCfgString("Recent Roms", tempStr, "", Config.RecentRoms[i]);
        //        FILE* f = fopen(tempStr, "wb");
        //        fputs(Config.RecentRoms[i], f);
        //        fclose(f);
    }

}

//Loads recent scripts from .cfg
void LoadRecentScripts()
{
    char tempStr[32];
    Config.RecentScriptsFreeze = ReadCfgInt("Recent Scripts", "Freeze", 0);
    for (unsigned i = 0; i < LUA_MAX_RECENT; i++)
    {

        sprintf(tempStr, "RecentLua%d", i);
        ReadCfgString("Recent Scripts", tempStr, "", Config.RecentScripts[i]);
    }

}

void ReadHotkeyConfig(int n, char* name, int cmd, int def) {
    HOTKEY* h;
    char t[128];
    h = &Config.hotkey[n];
    sprintf(t, "%s Key", name);
    h->key = ReadCfgInt("Hotkeys", t, def & 0xFF);
    sprintf(t, "%s Shift", name);
    h->shift = ReadCfgInt("Hotkeys", t, def & 0x100 ? 1 : 0);
    sprintf(t, "%s Ctrl", name);
    h->ctrl = ReadCfgInt("Hotkeys", t, def & 0x200 ? 1 : 0);
    sprintf(t, "%s Alt", name);
    h->alt = ReadCfgInt("Hotkeys", t, def & 0x400 ? 1 : 0);
    h->command = cmd;
}
void WriteHotkeyConfig(int n, char* name) {
    HOTKEY* h;
    char t[128];
    h = &Config.hotkey[n];
    sprintf(t, "%s Key", name);
    WriteCfgInt("Hotkeys", t, h->key);
    sprintf(t, "%s Shift", name);
    WriteCfgInt("Hotkeys", t, h->shift);
    sprintf(t, "%s Ctrl", name);
    WriteCfgInt("Hotkeys", t, h->ctrl);
    sprintf(t, "%s Alt", name);
    WriteCfgInt("Hotkeys", t, h->alt);
}

void LoadConfig()
{
    LoadRecentRoms();
    LoadRecentScripts();

    // Language
    ReadCfgString("Language", "Default", "English", Config.DefaultLanguage);

    //Window position and size 
    Config.WindowWidth = ReadCfgInt("Window", "Width", 640);
    Config.WindowHeight = ReadCfgInt("Window", "Height", 480);
    Config.WindowPosX = ReadCfgInt("Window", "X", (GetSystemMetrics(SM_CXSCREEN) - Config.WindowWidth) / 2);
    Config.WindowPosY = ReadCfgInt("Window", "Y", (GetSystemMetrics(SM_CYSCREEN) - Config.WindowHeight) / 2);
    //if mupen was closed by minimising
    if (Config.WindowPosX < MIN_WINDOW_W-1 || Config.WindowWidth < MIN_WINDOW_H-1) {
        printf("window too small. attempting to fix\n");
        Config.WindowWidth = 800;
        Config.WindowHeight = 600;
        Config.WindowPosX = (GetSystemMetrics(SM_CXSCREEN) - Config.WindowWidth) / 2;
        Config.WindowPosY = (GetSystemMetrics(SM_CYSCREEN) - Config.WindowHeight) / 2;
    }

    //General Vars
    Config.showFPS = ReadCfgInt("General", "Show FPS", 1);
    Config.showVIS = ReadCfgInt("General", "Show VIS", 1);
    Config.alertBAD = ReadCfgInt("General", "Alert Bad roms", 1);
    Config.alertHACK = ReadCfgInt("General", "Alert Hacked roms", 1);
    Config.savesERRORS = ReadCfgInt("General", "Alert Saves errors", 1);
    Config.limitFps = ReadCfgInt("General", "Limit Fps", 1);
    Config.compressedIni = ReadCfgInt("General", "Compressed Ini", 1);
    Config.UseFPSmodifier = ReadCfgInt("General", "Use Fps Modifier", 1);
    Config.FPSmodifier = ReadCfgInt("General", "Fps Modifier", 100);
	Config.skipFrequency = ReadCfgInt("General", "Skip Frequency", 8);
	Config.loopMovie = ReadCfgInt("General", "Loop Movie", 0);
    Config.zeroIndex = ReadCfgInt("General", "Zero index", 0);


    Config.guiDynacore = ReadCfgInt("CPU", "Core", 1);



    //Advanced vars
    Config.StartFullScreen = ReadCfgInt("Advanced", "Start Full Screen", 0);
    Config.PauseWhenNotActive = ReadCfgInt("Advanced", "Pause when not active", 1);
    Config.OverwritePluginSettings = ReadCfgInt("Advanced", "Overwrite Plugins Settings", 0);
    Config.GuiToolbar = ReadCfgInt("Advanced", "Use Toolbar", 0);
    Config.GuiStatusbar = ReadCfgInt("Advanced", "Use Statusbar", 1);
    Config.AutoIncSaveSlot = ReadCfgInt("Advanced", "Auto Increment Save Slot", 0);
    round_to_zero = ReadCfgInt("Advanced", "Round To Zero", 0);
    emulate_float_crashes = ReadCfgInt("Advanced", "Emulate Float Crashes", 0);
    input_delay = ReadCfgInt("Advanced", "Old Input Delay", 0);
    LUA_double_buffered = ReadCfgInt("Advanced", "LUA Double Buffer", 1);
	Config.IgnoreStWarnings = ReadCfgInt("Advanced", "Ignore non-movie st warning", 0);

    //Compatibility Settings
    no_audio_delay = ReadCfgInt("Compatibility", "No Audio Delay", 0);
    no_compiled_jump = ReadCfgInt("Compatibility", "No Compiled Jump", 0);

    //RomBrowser Columns
    Config.Column_GoodName = ReadCfgInt("Rom Browser Columns", "Good Name", 1);
    Config.Column_InternalName = ReadCfgInt("Rom Browser Columns", "Internal Name", 0);
    Config.Column_Country = ReadCfgInt("Rom Browser Columns", "Country", 1);
    Config.Column_Size = ReadCfgInt("Rom Browser Columns", "Size", 1);
    Config.Column_Comments = ReadCfgInt("Rom Browser Columns", "Comments", 1);
    Config.Column_FileName = ReadCfgInt("Rom Browser Columns", "File Name", 0);
    Config.Column_MD5 = ReadCfgInt("Rom Browser Columns", "MD5", 0);

    // Directories
    Config.DefaultPluginsDir = ReadCfgInt("Directories", "Default Plugins Directory", 1);
    Config.DefaultSavesDir = ReadCfgInt("Directories", "Default Saves Directory", 1);
    Config.DefaultScreenshotsDir = ReadCfgInt("Directories", "Default Screenshots Directory", 1);

    sprintf(Config.PluginsDir, "%sPlugin\\", AppPath);
    ReadCfgString("Directories", "Plugins Directory", Config.PluginsDir, Config.PluginsDir);

    sprintf(Config.SavesDir, "%sSave\\", AppPath);
    ReadCfgString("Directories", "Saves Directory", Config.SavesDir, Config.SavesDir);

    sprintf(Config.ScreenshotsDir, "%sScreenShots\\", AppPath);
    ReadCfgString("Directories", "Screenshots Directory", Config.ScreenshotsDir, Config.ScreenshotsDir);



    // Rom Browser
    Config.RomBrowserSortColumn = ReadCfgInt("Rom Browser", "Sort Column", 0);
    Config.RomBrowserRecursion = ReadCfgInt("Rom Browser", "Recursion", 0);
    ReadCfgString("Rom Browser", "Sort Method", "ASC", Config.RomBrowserSortMethod);

    // Recording Options
    Config.NoReset = ReadCfgInt("Recording Options", "No reset recording", 1);

    //avi options
    Config.forceInternalCapture = ReadCfgInt("Avi Options", "Force internal capture", 0);
    Config.captureOtherWindows = ReadCfgInt("Avi Options", "Capture other windows", 0);

    // other
    Config.LuaWarnOnClose = ReadCfgInt("Other", "Ask on lua close", 0);
    Config.LuaSimpleDialog = ReadCfgInt("Other", "Simplified lua", 0);
    Config.movieBackups = ReadCfgInt("Other", "Autobackup", 0);
    Config.movieBackupsLevel = ReadCfgInt("Other", "Autobackup level", 0);
    Config.FrequentVCRUIRefresh = ReadCfgInt("Other", "Fast statusbar updates", 0);
    Config.SyncMode = ReadCfgInt("Other", "Synchronisation Mode", VCR_SYNC_AUDIO_DUPL);
    Config.moviesERRORS = ReadCfgInt("General", "Alert Movies errors", 1); //moviesERRORS == 1 -> the errors are shown

    // Load A Whole Whackton Of Hotkeys:

    ReadHotkeyConfig(0, "Fast Forward", 0, VK_TAB);	// handled specially
    ReadHotkeyConfig(1, "Speed Up", IDC_INCREASE_MODIFIER, /*VK_OEM_PLUS*/0xBB);
    ReadHotkeyConfig(2, "Slow Down", IDC_DECREASE_MODIFIER, /*VK_OEM_MINUS*/0xBD);
    ReadHotkeyConfig(3, "Frame Advance", EMU_FRAMEADVANCE, VK_OEM_5);
    ReadHotkeyConfig(4, "Pause Resume", EMU_PAUSE, VK_PAUSE);
    ReadHotkeyConfig(5, "ReadOnly", EMU_VCRTOGGLEREADONLY, '8' | 0x100);
    ReadHotkeyConfig(6, "Play", ID_START_PLAYBACK, 'P' | 0x300);
    ReadHotkeyConfig(7, "PlayStop", ID_STOP_PLAYBACK, 'S' | 0x300);
    ReadHotkeyConfig(8, "Record", ID_START_RECORD, 'R' | 0x300);
    ReadHotkeyConfig(9, "RecordStop", ID_STOP_RECORD, 'S' | 0x300);
    ReadHotkeyConfig(10, "Screenshot", GENERATE_BITMAP, VK_F12);
    ReadHotkeyConfig(11, "Save Current", STATE_SAVE, VK_F5 | 0x200);
    ReadHotkeyConfig(12, "Load Current", STATE_RESTORE, VK_F7 | 0x200);

    // Save/Load Hotkeys
    {
        int i;
        char str[128];

        for (i = 1; i <= 9; i++)
        {
            sprintf(str, "Save %d", i);
            ReadHotkeyConfig(12 + i, str, (ID_SAVE_1 - 1) + i, ((VK_F1 - 1) + i) | 0x100);
        }
        for (i = 1; i <= 9; i++)
        {
            sprintf(str, "Load %d", i);
            ReadHotkeyConfig(21 + i, str, (ID_LOAD_1 - 1) + i, (VK_F1 - 1) + i);
        }
        for (i = 1; i <= 9; i++)
        {
            sprintf(str, "Select %d", i);
            ReadHotkeyConfig(30 + i, str, (ID_CURRENTSAVE_1 - 1) + i, '0' + i);
        }
    }
    //Lua
    //ダイアログに追加するの面倒くさい
    ReadCfgString("Lua", "Script Path", "", Config.LuaScriptPath);
    ReadHotkeyConfig(40, "Lua Script Reload", ID_LUA_RELOAD, VK_F3 | 0x200);
    ReadHotkeyConfig(41, "Lua Script CloseAll", ID_MENU_LUASCRIPT_CLOSEALL, VK_F4 | 0x200);

    // some new misc ones
    ReadHotkeyConfig(42, "Start From Beginning", ID_RESTART_MOVIE, 'H' | 0x300);
    ReadHotkeyConfig(43, "Replay Latest", ID_REPLAY_LATEST, 'G' | 0x300);


}

/////////////////////////////////////////////////////////////////////////////////

void saveWindowSettings()
{
    RECT rcMain;
    GetWindowRect(mainHWND, &rcMain);
    Config.WindowPosX = rcMain.left;
    Config.WindowPosY = rcMain.top;
    Config.WindowWidth = rcMain.right - rcMain.left;
    Config.WindowHeight = rcMain.bottom - rcMain.top;
    WriteCfgInt("Window", "Width", Config.WindowWidth);
    WriteCfgInt("Window", "Height", Config.WindowHeight);
    WriteCfgInt("Window", "X", Config.WindowPosX);
    WriteCfgInt("Window", "Y", Config.WindowPosY);
}

void saveBrowserSettings()
{
    int Column, ColWidth, index;
    index = 0;
    for (Column = 0; Column < ROM_COLUMN_FIELDS; Column++) {
        if (isFieldInBrowser(Column)) {
            ColWidth = ListView_GetColumnWidth(hRomList, index);
            WriteCfgInt("Rom Browser", getFieldName(Column), ColWidth);
            index++;
        }
    }

    WriteCfgInt("Rom Browser", "Sort Column", Config.RomBrowserSortColumn);
    WriteCfgInt("Rom Browser", "Recursion", Config.RomBrowserRecursion);
    WriteCfgString("Rom Browser", "Sort Method", Config.RomBrowserSortMethod);

}

void SaveRecentRoms()
{
    int i;
    char tempStr[50];

    WriteCfgInt("Recent Roms", "Freeze", Config.RecentRomsFreeze);
    for (i = 0; i < MAX_RECENT_ROMS; i++)
    {
        sprintf(tempStr, "RecentRom%d", i);
        WriteCfgString("Recent Roms", tempStr, Config.RecentRoms[i]);
    }
}

void SaveRecentScripts()
{
    char tempStr[32];
    WriteCfgInt("Recent Scripts", "Freeze", Config.RecentScriptsFreeze);
    for (unsigned i = 0; i < LUA_MAX_RECENT; i++)
    {
        sprintf(tempStr, "RecentLua%d", i);
        WriteCfgString("Recent Scripts", tempStr, Config.RecentScripts[i]);
    }
}

void SaveConfig()
{
    saveWindowSettings();

    if (!cmdlineNoGui) {
        saveBrowserSettings();
        SaveRecentRoms();
        SaveRecentScripts();
    }

    //Language
    WriteCfgString("Language", "Default", Config.DefaultLanguage);

    //General Vars
    WriteCfgInt("General", "Show FPS", Config.showFPS);
    WriteCfgInt("General", "Show VIS", Config.showVIS);
    WriteCfgInt("General", "Alert Bad roms", Config.alertBAD);
    WriteCfgInt("General", "Alert Hacked roms", Config.alertHACK);
    WriteCfgInt("General", "Alert Saves errors", Config.savesERRORS);
    WriteCfgInt("General", "Alert Movies errors", Config.moviesERRORS);
    WriteCfgInt("General", "Limit Fps", Config.limitFps);
    WriteCfgInt("General", "Compressed Ini", Config.compressedIni);
    WriteCfgInt("General", "Fps Modifier", Config.FPSmodifier);
    WriteCfgInt("General", "Use Fps Modifier", Config.UseFPSmodifier);
	WriteCfgInt("General", "Skip Frequency", Config.skipFrequency);
	WriteCfgInt("General", "Loop Movie", Config.loopMovie);
    WriteCfgInt("General", "Zero Index", Config.zeroIndex);

    //Advanced Vars
    WriteCfgInt("Advanced", "Start Full Screen", Config.StartFullScreen);
    WriteCfgInt("Advanced", "Pause when not active", Config.PauseWhenNotActive);
    WriteCfgInt("Advanced", "Overwrite Plugins Settings", Config.OverwritePluginSettings);
    WriteCfgInt("Advanced", "Use Toolbar", Config.GuiToolbar);
    WriteCfgInt("Advanced", "Use Statusbar", Config.GuiStatusbar);
    WriteCfgInt("Advanced", "Auto Increment Save Slot", Config.AutoIncSaveSlot);
    WriteCfgInt("Advanced", "Round To Zero", round_to_zero);
    WriteCfgInt("Advanced", "Emulate Float Crashes", emulate_float_crashes);
    WriteCfgInt("Advanced", "Old Input Delay", input_delay);
	WriteCfgInt("Advanced", "LUA Double Buffer", LUA_double_buffered);
    WriteCfgInt("Advanced", "Ignore non-movie st warning", Config.IgnoreStWarnings);
    
    WriteCfgInt("CPU", "Core", Config.guiDynacore);

    //Compatibility Settings
    WriteCfgInt("Compatibility", "No Audio Delay", no_audio_delay);
    WriteCfgInt("Compatibility", "No Compiled Jump", no_compiled_jump);

    //Rom Browser columns
    WriteCfgInt("Rom Browser Columns", "Good Name", Config.Column_GoodName);
    WriteCfgInt("Rom Browser Columns", "Internal Name", Config.Column_InternalName);
    WriteCfgInt("Rom Browser Columns", "Country", Config.Column_Country);
    WriteCfgInt("Rom Browser Columns", "Size", Config.Column_Size);
    WriteCfgInt("Rom Browser Columns", "Comments", Config.Column_Comments);
    WriteCfgInt("Rom Browser Columns", "File Name", Config.Column_FileName);
    WriteCfgInt("Rom Browser Columns", "MD5", Config.Column_MD5);

    // Directories
    WriteCfgInt("Directories", "Default Plugins Directory", Config.DefaultPluginsDir);
    WriteCfgString("Directories", "Plugins Directory", Config.PluginsDir);
    WriteCfgInt("Directories", "Default Saves Directory", Config.DefaultSavesDir);
    WriteCfgString("Directories", "Saves Directory", Config.SavesDir);
    WriteCfgInt("Directories", "Default Screenshots Directory", Config.DefaultScreenshotsDir);
    WriteCfgString("Directories", "Screenshots Directory", Config.ScreenshotsDir);

    WriteCfgInt("Recording Options", "No reset recording", Config.NoReset);

    WriteCfgInt("Avi Options", "Force internal capture", Config.forceInternalCapture);
    WriteCfgInt("Avi Options", "Capture other windows", Config.captureOtherWindows);

    WriteCfgInt("Other", "Ask on lua close", Config.LuaWarnOnClose);
    WriteCfgInt("Other", "Simplified lua", Config.LuaSimpleDialog);
    WriteCfgInt("Other", "Autobackup", Config.movieBackups);
    WriteCfgInt("Other", "Autobackup level", Config.movieBackupsLevel);
    WriteCfgInt("Other", "Fast statusbar updates", Config.FrequentVCRUIRefresh);
    WriteCfgInt("Other", "Synchronisation Mode", Config.SyncMode);

    // Save A Whole Whackton Of Hotkeys:

    char* settingStrings[13] = { "Fast Forward", "Speed Up", "Slow Down", "Frame Advance", "Pause Resume",
        "ReadOnly", "Play", "PlayStop", "Record",
        "RecordStop", "Screenshot", "Save Current", "Load Current" };

    for (int i = 0; i <= 12; i++)
    {
        WriteHotkeyConfig(i, settingStrings[i]);
    }

    // Save/Load Hotkeys
        int i;
        char str[128];

        for (i = 1; i <= 9; i++)
        {
            sprintf(str, "Save %d", i);
            WriteHotkeyConfig(12 + i, str);
            sprintf(str, "Load %d", i);
            WriteHotkeyConfig(21 + i, str);
            sprintf(str, "Select %d", i);
            WriteHotkeyConfig(30 + i, str);
        }
    //Lua
    WriteCfgString("Lua", "Script Path", Config.LuaScriptPath);
    WriteHotkeyConfig(40, "Lua Script Reload");
    WriteHotkeyConfig(41, "Lua Script CloseAll");

    // some new misc ones
    WriteHotkeyConfig(42, "Start From Beginning");
    WriteHotkeyConfig(43, "Replay Latest");
    
}


