/***************************************************************************
                          main_win.h  -  description
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

#ifndef MAIN_WIN_H
#define MAIN_WIN_H

extern BOOL CALLBACK CfgDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern void ShowMessage(LPSTR lpszMessage) ;
extern void EnableToolbar();
extern void CreateStatusBarWindow( HWND hwnd );
extern void SetStatusMode( int mode );
extern char *getPluginName( char *pluginpath, int plugintype);
extern char* getExtension(char *str);

/********* Global Variables **********/
extern char TempMessage[800];
extern int emu_launched; // emu_emulating
extern int emu_paused;
extern int recording;
extern HWND hTool, mainHWND, hStatus, hRomList, hStatusProgress;
extern HINSTANCE app_hInstance;
extern BOOL manualFPSLimit;
extern char statusmsg[800];

extern char gfx_name[255];
extern char input_name[255];
extern char sound_name[255];
extern char rsp_name[255];

extern HWND hwnd_plug;
extern HANDLE EmuThreadHandle;

extern char AppPath[MAX_PATH];

extern void EnableEmulationMenuItems(BOOL flag);
extern BOOL StartRom(char *fullRomPath);
extern void resetEmu() ;
extern void resumeEmu(BOOL quiet);
extern void pauseEmu(BOOL quiet);
//extern void closeRom();
extern void search_plugins();
extern void rewind_plugin();
extern int get_plugin_type();
extern char *next_plugin();
extern void exec_config(char *name);
extern void exec_test(char *name);
extern void exec_about(char *name);
extern void EnableStatusbar();
extern void OpenMoviePlaybackDialog();
extern void OpenMovieRecordDialog();

typedef struct _HOTKEY {
    int key;
    BOOL shift;
    BOOL ctrl;
    BOOL alt;
    int command;
} HOTKEY;
#define NUM_HOTKEYS (42)

typedef struct _CONFIG {
    unsigned char ConfigVersion ;
    
    //Language
    char DefaultLanguage[100];
    
    // Alert Messages variables
    BOOL showFPS;
    BOOL showVIS;
    BOOL alertBAD;
    BOOL alertHACK;
    BOOL savesERRORS;
    
    // General vars
    BOOL limitFps;
    BOOL compressedIni;
    int guiDynacore;
    BOOL UseFPSmodifier;
    int FPSmodifier;
    
    // Advanced vars
    BOOL StartFullScreen;
    BOOL PauseWhenNotActive;
    BOOL OverwritePluginSettings;
    BOOL GuiToolbar;
    BOOL GuiStatusbar;
    BOOL AutoIncSaveSlot;
    //BOOL RoundToZero;
    
    //Compatibility Options
    //BOOL NoAudioDelay;
    //BOOL NoCompiledJump;
    
    //Rom Browser Columns
    BOOL Column_GoodName;
    BOOL Column_InternalName;
    BOOL Column_Country;
    BOOL Column_Size;
    BOOL Column_Comments;
    BOOL Column_FileName;
    BOOL Column_MD5;
                                             
    // Directories
    BOOL DefaultPluginsDir;
    BOOL DefaultSavesDir;
    BOOL DefaultScreenshotsDir;
    char PluginsDir[MAX_PATH];
    char SavesDir[MAX_PATH];
    char ScreenshotsDir[MAX_PATH];    
    
    // Recent Roms
    char RecentRoms[10][MAX_PATH];
    BOOL RecentRomsFreeze;
    
    //Window
    int WindowWidth;
    int WindowHeight;
    int WindowPosX;
    int WindowPosY;
        
    //Rom Browser
    int RomBrowserSortColumn;
    char RomBrowserSortMethod[10];
    BOOL RomBrowserRecursion;
    
    HOTKEY hotkey [NUM_HOTKEYS];

		//Lua
		char LuaScriptPath[MAX_PATH];
} CONFIG;

extern "C" CONFIG Config;

#endif
