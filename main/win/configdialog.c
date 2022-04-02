/***************************************************************************
                          configdialog.c  -  description
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

#include <shlobj.h>
#include <stdio.h>
#include "../lua/LuaConsole.h"
#include "main_win.h"
#include "../../winproject/resource.h"
#include "../plugin.h"
#include "rombrowser.h"
#include "../guifuncs.h"
#include "../md5.h"
#include "timers.h"
#include "translation.h"
#include "Config.h"
#include "../rom.h"
#include "inifunctions.h"

#include "configdialog.h"
#include <vcr.h>
// ughh msvc-only code once again
#pragma comment(lib,"comctl32.lib") 
#pragma comment(lib,"propsys.lib")
#pragma comment(lib,"shlwapi.lib") 

#ifdef _MSC_VER
#define snprintf	_snprintf
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#endif


//HWND romInfoHWND;
static DWORD dwExitCode;
static DWORD Id;
BOOL stopScan = FALSE;
HWND __stdcall CreateTrackbar(HWND hwndDlg, UINT iMin, UINT iMax, UINT iSelMin, UINT iSelMax, UINT x, UINT y, UINT w); // winapi macro was very confusing

HWND hwndTrack ; 
HWND hwndTrackOther;

extern int no_audio_delay;
extern int no_compiled_jump;

BOOL LuaCriticalSettingChangePending; // other options proc

BOOL CALLBACK OtherOptionsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        WriteCheckBoxValue(hwnd, IDC_LUA_SIMPLEDIALOG, Config.LuaSimpleDialog);
        WriteCheckBoxValue(hwnd, IDC_LUA_WARNONCLOSE, Config.LuaWarnOnClose);
        WriteCheckBoxValue(hwnd, IDC_MOVIEBACKUPS, Config.movieBackups);
        WriteCheckBoxValue(hwnd, IDC_ALERTMOVIESERRORS, Config.moviesERRORS);
        WriteCheckBoxValue(hwnd, IDC_FREQUENTVCRREFRESH, Config.FrequentVCRUIRefresh);

        hwndTrackOther = CreateTrackbar(hwnd, 1, 3, Config.movieBackupsLevel, 3, 200, 79, 100);

        switch (Config.SyncMode)
        {
        case VCR_SYNC_AUDIO_DUPL:
            CheckDlgButton(hwnd, IDC_AV_AUDIOSYNC, BST_CHECKED);
            break;
        case VCR_SYNC_VIDEO_SNDROP:
            CheckDlgButton(hwnd, IDC_AV_VIDEOSYNC, BST_CHECKED);
            break;
        case VCR_SYNC_NONE:
            CheckDlgButton(hwnd, IDC_AV_NOSYNC, BST_CHECKED);
            break;
        }


        return TRUE;

    case WM_COMMAND: {
        // dame tu xorita mamacita
        switch (LOWORD(wParam))
        {
        case IDC_AV_AUDIOSYNC:
            if (!VCR_isCapturing())
            Config.SyncMode = VCR_SYNC_AUDIO_DUPL;
            break;
        case IDC_AV_VIDEOSYNC:
            if (!VCR_isCapturing())
            Config.SyncMode = VCR_SYNC_VIDEO_SNDROP;
            break;
        case IDC_AV_NOSYNC:
            if (!VCR_isCapturing())
            Config.SyncMode = VCR_SYNC_NONE;
            break;
        case IDC_LUA_SIMPLEDIALOG: {

            if (MessageBox(0, "Changing this option requires a restart.\nPress Yes to confirm that you want to change this setting. (You won\'t be able to use lua until a restart)\nPress No to revert changes.", "Restart required", MB_TOPMOST | MB_TASKMODAL | MB_ICONWARNING | MB_YESNO) == IDYES) {
                LuaCriticalSettingChangePending = 1;
                CloseAllLuaScript();
            }
            else
                CheckDlgButton(hwnd, IDC_LUA_SIMPLEDIALOG, Config.LuaSimpleDialog);

            break;
        }
        }
        break;
    }

    case WM_NOTIFY:
    {
        if (((NMHDR FAR*) lParam)->code == NM_RELEASEDCAPTURE) {
            Config.movieBackupsLevel = SendMessage(hwndTrackOther, TBM_GETPOS, 0, 0);
        }

        if (((NMHDR FAR*) lParam)->code == PSN_APPLY) {
            Config.LuaSimpleDialog = ReadCheckBoxValue(hwnd, IDC_LUA_SIMPLEDIALOG);
            Config.LuaWarnOnClose = ReadCheckBoxValue(hwnd, IDC_LUA_WARNONCLOSE);
            Config.movieBackups = ReadCheckBoxValue(hwnd, IDC_MOVIEBACKUPS);
            Config.moviesERRORS = ReadCheckBoxValue(hwnd, IDC_ALERTMOVIESERRORS);
            Config.FrequentVCRUIRefresh = ReadCheckBoxValue(hwnd, IDC_FREQUENTVCRREFRESH);
            Config.FPSmodifier = SendMessage(hwndTrackOther, TBM_GETPOS, 0, 0);
            EnableToolbar();
            EnableStatusbar();
            FastRefreshBrowser();
            LoadConfigExternals();

        }

    }
    break;

    default:
        return FALSE;
    }
    return TRUE;
}


void WriteCheckBoxValue( HWND hwnd, int resourceID , int value)
{
    if  (value) {
                  SendMessage(GetDlgItem(hwnd,resourceID),BM_SETCHECK, BST_CHECKED,0);
                }
}

int ReadCheckBoxValue( HWND hwnd, int resourceID)
{
    return SendDlgItemMessage(hwnd,resourceID,BM_GETCHECK , 0,0) == BST_CHECKED?TRUE:FALSE;
}

void WriteComboBoxValue(HWND hwnd,int ResourceID,char *PrimaryVal,char *DefaultVal)
{
     int index;
     index = SendDlgItemMessage(hwnd, ResourceID, CB_FINDSTRINGEXACT, 0, (LPARAM)PrimaryVal);
     if (index!=CB_ERR) {
                SendDlgItemMessage(hwnd, ResourceID, CB_SETCURSEL, index, 0);
                return;
     }
     index = SendDlgItemMessage(hwnd, ResourceID, CB_FINDSTRINGEXACT, 0, (LPARAM)DefaultVal);
     if (index!=CB_ERR) { 
                SendDlgItemMessage(hwnd, ResourceID, CB_SETCURSEL, index, 0);
                return;
     }
     SendDlgItemMessage(hwnd, ResourceID, CB_SETCURSEL, 0, 0);
     
}

void ReadComboBoxValue(HWND hwnd,int ResourceID,char *ret)
{
    int index;
    index = SendDlgItemMessage(hwnd, ResourceID, CB_GETCURSEL, 0, 0);
    SendDlgItemMessage(hwnd, ResourceID, CB_GETLBTEXT, index, (LPARAM)ret);
}

void ChangeSettings(HWND hwndOwner) {
    PROPSHEETPAGE psp[6];
    PROPSHEETHEADER psh;
	char ConfigStr[200],DirectoriesStr[200],titleStr[200],settingsStr[200];
    char AdvSettingsStr[200], HotkeysStr[200], OtherStr[200];
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = app_hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_MAIN);
    psp[0].pfnDlgProc = PluginsCfg;
	TranslateDefault("Plugins","Plugins",ConfigStr);
	psp[0].pszTitle = ConfigStr;
    psp[0].lParam = 0;
    psp[0].pfnCallback = NULL;

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = app_hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_DIRECTORIES);
    psp[1].pfnDlgProc = DirectoriesCfg;
	TranslateDefault("Directories","Directories",DirectoriesStr);
    psp[1].pszTitle = DirectoriesStr;
    psp[1].lParam = 0;
    psp[1].pfnCallback = NULL;

    psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = app_hInstance;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_MESSAGES);
    psp[2].pfnDlgProc = GeneralCfg;
	TranslateDefault("General","General", settingsStr);
    psp[2].pszTitle = settingsStr;
    psp[2].lParam = 0;
    psp[2].pfnCallback = NULL;
    
    psp[3].dwSize = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags = PSP_USETITLE;
    psp[3].hInstance = app_hInstance;
    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_ADVANCED_OPTIONS);
    psp[3].pfnDlgProc = AdvancedSettingsProc;
	TranslateDefault("Advanced","Advanced",AdvSettingsStr);
    psp[3].pszTitle = AdvSettingsStr;
    psp[3].lParam = 0;
    psp[3].pfnCallback = NULL;
     
    psp[4].dwSize = sizeof(PROPSHEETPAGE);
    psp[4].dwFlags = PSP_USETITLE;
    psp[4].hInstance = app_hInstance;
    psp[4].pszTemplate = MAKEINTRESOURCE(IDD_HOTKEY_CONFIG);
    psp[4].pfnDlgProc = HotkeysProc;
	TranslateDefault("Hotkeys","Hotkeys",HotkeysStr);
    psp[4].pszTitle = HotkeysStr;
    psp[4].lParam = 0;
    psp[4].pfnCallback = NULL;
 
    psp[5].dwSize = sizeof(PROPSHEETPAGE);
    psp[5].dwFlags = PSP_USETITLE;
    psp[5].hInstance = app_hInstance;
    psp[5].pszTemplate = MAKEINTRESOURCE(IDD_OTHER_OPTIONS_DIALOG);
    psp[5].pfnDlgProc = OtherOptionsProc;
    TranslateDefault("Other", "Other", OtherStr);
    psp[5].pszTitle = OtherStr;
    psp[5].lParam = 0;
    psp[5].pfnCallback = NULL;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    psh.hwndParent = hwndOwner;
    psh.hInstance = app_hInstance;
    TranslateDefault("Settings","Settings",titleStr);
    psh.pszCaption = (LPSTR) titleStr;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;

    if (PropertySheet(&psh)) SaveConfig();
	return;
}

//Taken from windows docs
HWND CreateToolTip(int toolID, HWND hDlg, PTSTR pszText)
{
    if (!toolID || !hDlg || !pszText)
    {
        return FALSE;
    }
    // Get the window of the tool.
    HWND hwndTool = GetDlgItem(hDlg, toolID);

    // Create the tooltip. g_hInst is the global instance handle.
    HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        hDlg, NULL,
        GetModuleHandle(NULL), NULL);

    if (!hwndTool || !hwndTip)
    {
        return (HWND)NULL;
    }

    // Associate the tooltip with the tool.
    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = hDlg;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwndTool;
    toolInfo.lpszText = pszText;
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

    return hwndTip;
}


BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
             SendDlgItemMessage(hwnd, IDB_LOGO, STM_SETIMAGE, IMAGE_BITMAP, 
                (LPARAM)LoadImage (GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO), 
                            IMAGE_BITMAP, 0, 0, 0));
                      
 //            MessageBox( hwnd, "", "", MB_OK );
        return TRUE;
        
        case WM_CLOSE:
              EndDialog(hwnd, IDOK);  
        break;
        
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwnd, IDOK);
                break;
                
                case IDC_WEBSITE:
                    ShellExecute(0, 0, "http://mupen64.emulation64.com", 0, 0, SW_SHOW); 
                break;
                case IDC_GITREPO:
                    ShellExecute(0, 0, "https://github.com/mkdasher/mupen64-rr-lua-/", 0, 0, SW_SHOW);
                    break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

bool folderDiag(char* out, int max_size, const char* starting_dir)
{
    bool ret = false;
    IFileDialog* pfd;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
    {
        if (starting_dir)
        {
            PIDLIST_ABSOLUTE pidl;
            WCHAR wstarting_dir[MAX_PATH];
            WCHAR* wc = wstarting_dir;
            for (const char* c = starting_dir; *c && wc - wstarting_dir < MAX_PATH - 1; ++c, ++wc)
            {
                *wc = *c == '/' ? '\\' : *c;
            }
            *wc = 0;

            HRESULT hresult = ::SHParseDisplayName(wstarting_dir, 0, &pidl, SFGAO_FOLDER, 0);
            if (SUCCEEDED(hresult))
            {
                IShellItem* psi;
                hresult = ::SHCreateShellItem(NULL, NULL, pidl, &psi);
                if (SUCCEEDED(hresult))
                {
                    pfd->SetFolder(psi);
                }
                ILFree(pidl);
            }
        }

        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
        {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }
        if (SUCCEEDED(pfd->Show(NULL)))
        {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi)))
            {
                WCHAR* tmp;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &tmp)))
                {
                    char* c = out;
                    while (*tmp && c - out < max_size - 1)
                    {
                        *c = (char)*tmp;
                        ++c;
                        ++tmp;
                    }
                    *c = '\0';
                    ret = true;
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return ret;
}


BOOL CALLBACK DirectoriesCfg(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  char Buffer[MAX_PATH], Directory[MAX_PATH];
    LPITEMIDLIST pidl;
	BROWSEINFO bi;
    char RomBrowserDir[_MAX_PATH]; 
    HWND RomBrowserDirListBox;
    int count;
    int selected;
    switch(Message)
    {
        case WM_INITDIALOG:
                FillRomBrowserDirBox(hwnd);
                TranslateDirectoriesConfig(hwnd);
                if  (Config.RomBrowserRecursion) {
                  SendMessage(GetDlgItem(hwnd,IDC_RECURSION),BM_SETCHECK, BST_CHECKED,0);
                }
                
                if (Config.DefaultPluginsDir)
                {
                     SendMessage(GetDlgItem(hwnd,IDC_DEFAULT_PLUGINS_CHECK),BM_SETCHECK, BST_CHECKED,0);
                     EnableWindow( GetDlgItem(hwnd,IDC_PLUGINS_DIR), FALSE );
                     EnableWindow( GetDlgItem(hwnd,IDC_CHOOSE_PLUGINS_DIR), FALSE );           
                }
                if (Config.DefaultSavesDir)
                {
                     SendMessage(GetDlgItem(hwnd,IDC_DEFAULT_SAVES_CHECK),BM_SETCHECK, BST_CHECKED,0);            
                     EnableWindow( GetDlgItem(hwnd,IDC_SAVES_DIR), FALSE );
                     EnableWindow( GetDlgItem(hwnd,IDC_CHOOSE_SAVES_DIR), FALSE );                
                }
                if (Config.DefaultScreenshotsDir)
                {
                     SendMessage(GetDlgItem(hwnd,IDC_DEFAULT_SCREENSHOTS_CHECK),BM_SETCHECK, BST_CHECKED,0);            
                     EnableWindow( GetDlgItem(hwnd,IDC_SCREENSHOTS_DIR), FALSE );
                     EnableWindow( GetDlgItem(hwnd,IDC_CHOOSE_SCREENSHOTS_DIR), FALSE );                
                }                
                
                SetDlgItemText( hwnd, IDC_PLUGINS_DIR, Config.PluginsDir );
                SetDlgItemText( hwnd, IDC_SAVES_DIR, Config.SavesDir );
                SetDlgItemText( hwnd, IDC_SCREENSHOTS_DIR, Config.ScreenshotsDir );
                              
                break;                 
        case WM_NOTIFY:
                if (((NMHDR FAR *) lParam)->code == PSN_APPLY) {
                    SaveRomBrowserDirs();
                    selected = SendDlgItemMessage( hwnd, IDC_DEFAULT_PLUGINS_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED?TRUE:FALSE;    
                    GetDlgItemText( hwnd, IDC_PLUGINS_DIR, TempMessage, 200 );
                    if (strcasecmp(TempMessage,Config.PluginsDir)!=0 || Config.DefaultPluginsDir !=selected)  
                                  //if plugin dir changed,search for plugins in new dir
					           {
					   		        sprintf(Config.PluginsDir,TempMessage);
                                    Config.DefaultPluginsDir =  selected ;       
                                    search_plugins();		                         
                               } 
                    else       {            
                                    sprintf(Config.PluginsDir,TempMessage);
                                    Config.DefaultPluginsDir =  selected ;                                                
                                } 
                                                   
                    selected = SendDlgItemMessage( hwnd, IDC_DEFAULT_SAVES_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED?TRUE:FALSE;    
                    GetDlgItemText( hwnd, IDC_SAVES_DIR, Config.SavesDir, MAX_PATH );
                    Config.DefaultSavesDir =  selected ;
                    
                    selected = SendDlgItemMessage( hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED?TRUE:FALSE;    
                    GetDlgItemText( hwnd, IDC_SCREENSHOTS_DIR, Config.ScreenshotsDir, MAX_PATH );
                    Config.DefaultScreenshotsDir =  selected ;                
                }
                break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDC_RECURSION:
                Config.RomBrowserRecursion = SendDlgItemMessage(hwnd, IDC_RECURSION, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
                break;
            case IDC_ADD_BROWSER_DIR: {

                folderDiag(Directory, sizeof(Directory) / sizeof(char), "");
                int len = strlen(Directory);
                if (addDirectoryToLinkedList(Directory)) {
                    SendDlgItemMessage(hwnd, IDC_ROMBROWSER_DIR_LIST, LB_ADDSTRING, 0, (LPARAM)Directory);
                    AddDirToList(Directory, TRUE);
                }
                break;
            }
            case IDC_REMOVE_BROWSER_DIR:
                RomBrowserDirListBox = GetDlgItem(hwnd, IDC_ROMBROWSER_DIR_LIST);
                count = SendMessage(RomBrowserDirListBox, LB_GETSELCOUNT, 0, 0);
                if (count != 0)
                {
                    selected = SendMessage(RomBrowserDirListBox, LB_GETCURSEL, 0, 0);
                    SendMessage(RomBrowserDirListBox, LB_GETTEXT, selected, (LPARAM)RomBrowserDir);
                    removeDirectoryFromLinkedList(RomBrowserDir);
                    SendMessage(RomBrowserDirListBox, LB_DELETESTRING, selected, 0);
                    RefreshRomBrowser();
                }
                else
                {
                    MessageBox(hwnd, "No items selected.", "Warning", MB_OK);
                }
                break;

            case IDC_REMOVE_BROWSER_ALL:
                SendDlgItemMessage(hwnd, IDC_ROMBROWSER_DIR_LIST, LB_RESETCONTENT, 0, 0);
                freeRomDirList();
                RefreshRomBrowser();
                break;

            case IDC_DEFAULT_PLUGINS_CHECK:
            {
                selected = SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_PLUGINS_CHECK), BM_GETCHECK, 0, 0);
                if (!selected)
                {
                    MessageBox(NULL, "Warning: changing the plugin folder can introduce bugs in many plugins", "Warning", MB_OK);
                    EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_DIR), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_PLUGINS_DIR), TRUE);
                }
                else
                {
                    EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_DIR), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_PLUGINS_DIR), FALSE);
                }
            }
            break;
            case IDC_CHOOSE_PLUGINS_DIR:
            {
                // Lol why is vs indenting this one block down
                folderDiag(Directory, sizeof(Directory)/sizeof(char), "");
                int len = strlen(Directory);
                if (Directory[len - 1] != '\\')
                    strcat(Directory, "\\");
                SetDlgItemText(hwnd, IDC_PLUGINS_DIR, Directory);

            }
            
            
                  break;
				  case IDC_DEFAULT_SAVES_CHECK:
				  {      
                        selected = SendMessage( GetDlgItem(hwnd,IDC_DEFAULT_SAVES_CHECK), BM_GETCHECK, 0, 0 );
				        if (!selected)
				        {
                            EnableWindow( GetDlgItem(hwnd,IDC_SAVES_DIR), TRUE );
                            EnableWindow( GetDlgItem(hwnd,IDC_CHOOSE_SAVES_DIR), TRUE );
                        }else
                        {
                            EnableWindow( GetDlgItem(hwnd,IDC_SAVES_DIR), FALSE );
                            EnableWindow( GetDlgItem(hwnd,IDC_CHOOSE_SAVES_DIR), FALSE );
                        }
                  }
                  break;
                  case IDC_CHOOSE_SAVES_DIR:
                  {
                       folderDiag(Directory, sizeof(Directory) / sizeof(char), "");
                       if (Directory[strlen(Directory) - 1] != '\\')
                       {
                           strcat(Directory, "\\");
                           SetDlgItemText(hwnd, IDC_SAVES_DIR, Directory);
                       }
                  }
                  break;
				  case IDC_DEFAULT_SCREENSHOTS_CHECK:
				  {      
                        selected = SendMessage( GetDlgItem(hwnd,IDC_DEFAULT_SCREENSHOTS_CHECK), BM_GETCHECK, 0, 0 );
				        if (!selected)
				        {
                            EnableWindow( GetDlgItem(hwnd,IDC_SCREENSHOTS_DIR), TRUE );
                            EnableWindow( GetDlgItem(hwnd,IDC_CHOOSE_SCREENSHOTS_DIR), TRUE );
                        }else
                        {
                            EnableWindow( GetDlgItem(hwnd,IDC_SCREENSHOTS_DIR), FALSE );
                            EnableWindow( GetDlgItem(hwnd,IDC_CHOOSE_SCREENSHOTS_DIR), FALSE );
                        }
                  }
                  break;                
                  case IDC_CHOOSE_SCREENSHOTS_DIR:
                  {
                      folderDiag(Directory, sizeof(Directory) / sizeof(char), "");
                      int len = strlen(Directory);
                      if (Directory[len - 1] != '\\')
                          strcat(Directory, "\\");
                      SetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, Directory);
                  }
                  break;
                }
	    break;
    
    }	        
    return FALSE;
}

BOOL CALLBACK PluginsCfg(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    char path_buffer[_MAX_PATH];
    int index;
                          
    switch(Message)
    {
        case WM_CLOSE:
              EndDialog(hwnd, IDOK);  
        break;
        case WM_INITDIALOG:
            rewind_plugin();           
            while(get_plugin_type() != -1) {
                switch (get_plugin_type())
                {
                case PLUGIN_TYPE_GFX:
                    SendDlgItemMessage(hwnd, IDC_COMBO_GFX, CB_ADDSTRING, 0, (LPARAM)next_plugin());
                    break;
                case PLUGIN_TYPE_CONTROLLER:
                    SendDlgItemMessage(hwnd, IDC_COMBO_INPUT, CB_ADDSTRING, 0, (LPARAM)next_plugin());
                    break;
                case PLUGIN_TYPE_AUDIO:
                    SendDlgItemMessage(hwnd, IDC_COMBO_SOUND, CB_ADDSTRING, 0, (LPARAM)next_plugin());
                    break;
                case PLUGIN_TYPE_RSP:
                    SendDlgItemMessage(hwnd, IDC_COMBO_RSP, CB_ADDSTRING, 0, (LPARAM)next_plugin());
                    break;                                
                default:
                    next_plugin();
                }
             }   
            // Set gfx plugin
            index = SendDlgItemMessage(hwnd, IDC_COMBO_GFX, CB_FINDSTRINGEXACT, 0, (LPARAM)gfx_name);
            if (index!=CB_ERR) {
                SendDlgItemMessage(hwnd, IDC_COMBO_GFX, CB_SETCURSEL, index, 0);
            }
            else   {
                SendDlgItemMessage(hwnd, IDC_COMBO_GFX, CB_SETCURSEL, 0, 0);
                SendDlgItemMessage(hwnd, IDC_COMBO_GFX, CB_GETLBTEXT, 0, (LPARAM)gfx_name);
                }
            // Set input plugin
            index = SendDlgItemMessage(hwnd, IDC_COMBO_INPUT, CB_FINDSTRINGEXACT, 0, (LPARAM)input_name);
            if (index!=CB_ERR) {
                 SendDlgItemMessage(hwnd, IDC_COMBO_INPUT, CB_SETCURSEL, index, 0);
            }
            else    {
                 SendDlgItemMessage(hwnd, IDC_COMBO_INPUT, CB_SETCURSEL, 0, 0);
                 SendDlgItemMessage(hwnd, IDC_COMBO_INPUT, CB_GETLBTEXT, 0, (LPARAM)input_name);
                }   
            // Set sound plugin
            index = SendDlgItemMessage(hwnd, IDC_COMBO_SOUND, CB_FINDSTRINGEXACT, 0, (LPARAM)sound_name);
            if (index!=CB_ERR) {
            SendDlgItemMessage(hwnd, IDC_COMBO_SOUND, CB_SETCURSEL, index, 0);
            }
            else    {
                  SendDlgItemMessage(hwnd, IDC_COMBO_SOUND, CB_SETCURSEL, 0, 0);
                  SendDlgItemMessage(hwnd, IDC_COMBO_SOUND, CB_GETLBTEXT, 0, (LPARAM)sound_name);
                }
            // Set RSP plugin
            index = SendDlgItemMessage(hwnd, IDC_COMBO_RSP, CB_FINDSTRINGEXACT, 0, (LPARAM)rsp_name);
            if (index!=CB_ERR) {
            SendDlgItemMessage(hwnd, IDC_COMBO_RSP, CB_SETCURSEL, index, 0);
            }
            else    {
                  SendDlgItemMessage(hwnd, IDC_COMBO_RSP, CB_SETCURSEL, 0, 0);
                  SendDlgItemMessage(hwnd, IDC_COMBO_RSP, CB_GETLBTEXT, 0, (LPARAM)rsp_name);
                }
                
                        
            TranslateConfigDialog(hwnd);
            if(emu_launched) {
                EnableWindow( GetDlgItem(hwnd,IDC_COMBO_GFX), FALSE );
                EnableWindow( GetDlgItem(hwnd,IDC_COMBO_INPUT), FALSE );
                EnableWindow( GetDlgItem(hwnd,IDC_COMBO_SOUND), FALSE );
                EnableWindow( GetDlgItem(hwnd,IDC_COMBO_RSP), FALSE );   
            }
            
            //Show the images
            SendDlgItemMessage(hwnd, IDB_DISPLAY, STM_SETIMAGE, IMAGE_BITMAP, 
                (LPARAM)LoadImage (GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DISPLAY), 
                IMAGE_BITMAP, 0, 0, 0));
            SendDlgItemMessage(hwnd, IDB_CONTROL, STM_SETIMAGE, IMAGE_BITMAP, 
                (LPARAM)LoadImage (GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROL), 
                IMAGE_BITMAP, 0, 0, 0));
             SendDlgItemMessage(hwnd, IDB_SOUND, STM_SETIMAGE, IMAGE_BITMAP, 
                (LPARAM)LoadImage (GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SOUND), 
                IMAGE_BITMAP, 0, 0, 0));
             SendDlgItemMessage(hwnd, IDB_RSP, STM_SETIMAGE, IMAGE_BITMAP, 
                (LPARAM)LoadImage (GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_RSP), 
                IMAGE_BITMAP, 0, 0, 0));
             return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDGFXCONFIG:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_GFX, path_buffer);
                     exec_config(path_buffer);
                     break;
                case IDGFXTEST:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_GFX, path_buffer);
                     exec_test(path_buffer);
                     break;
                case IDGFXABOUT:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_GFX, path_buffer);
                     exec_about(path_buffer);
                     break;
                case IDINPUTCONFIG:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_INPUT, path_buffer);
                     exec_config(path_buffer);
                     break;
                case IDINPUTTEST:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_INPUT, path_buffer);
                     exec_test(path_buffer);
                     break;
                case IDINPUTABOUT:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_INPUT, path_buffer);
                     exec_about(path_buffer);
                     break;
                case IDSOUNDCONFIG:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_SOUND, path_buffer);
                     exec_config(path_buffer);
                     break;
                case IDSOUNDTEST:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_SOUND, path_buffer);
                     exec_test(path_buffer);
                     break;
                case IDSOUNDABOUT:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_SOUND, path_buffer);
                     exec_about(path_buffer);
                     break;
                case IDRSPCONFIG:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_RSP, path_buffer);
                     exec_config(path_buffer);
                     break;
                case IDRSPTEST:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_RSP, path_buffer);
                     exec_test(path_buffer);
                     break;
                case IDRSPABOUT:
                     hwnd_plug = hwnd;
                     ReadComboBoxValue( hwnd, IDC_COMBO_RSP, path_buffer);
                     exec_about(path_buffer);
                     break;    
            }
            break;
        case WM_NOTIFY:
		             if (((NMHDR FAR *) lParam)->code == PSN_APPLY) {
		                 
                         ReadComboBoxValue( hwnd, IDC_COMBO_GFX, gfx_name);
                         WriteCfgString("Plugins","Graphics",gfx_name);
                         
                         ReadComboBoxValue( hwnd, IDC_COMBO_INPUT, input_name);
                         WriteCfgString("Plugins","Input",input_name);
                         
                         ReadComboBoxValue( hwnd, IDC_COMBO_SOUND, sound_name);
                         WriteCfgString("Plugins","Sound",sound_name);
                         
                         ReadComboBoxValue( hwnd, IDC_COMBO_RSP, rsp_name);
                         WriteCfgString("Plugins","RSP",rsp_name);
                     }
                     break;
        default:
            return FALSE;
    }
    return TRUE;
}


void SwitchModifier(HWND hwnd) {
    if ( ReadCheckBoxValue(hwnd,IDC_SPEEDMODIFIER)) {  
                   EnableWindow(hwndTrack,TRUE);             
                }
                else {
                   EnableWindow(hwndTrack,FALSE);
                } 
}

void SwitchLimitFPS(HWND hwnd) {
    if ( ReadCheckBoxValue(hwnd,IDC_LIMITFPS)) {
                  EnableWindow(GetDlgItem(hwnd,IDC_SPEEDMODIFIER), TRUE);
                  SwitchModifier(hwnd) ;               
                }
                else {
                  EnableWindow(GetDlgItem(hwnd,IDC_SPEEDMODIFIER), FALSE); 
                  EnableWindow(hwndTrack,FALSE); 
                }  
}

void FillModifierValue(HWND hwnd,int value ) {
    char temp[10];
    sprintf( temp, "%d%%", value);
    SetDlgItemText( hwnd, IDC_SPEEDMODIFIER_VALUE, temp); 
}

BOOL CALLBACK GeneralCfg(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{

    switch(Message) {
    case WM_INITDIALOG:
         WriteCheckBoxValue( hwnd, IDC_SHOWFPS, Config.showFPS) ;
         WriteCheckBoxValue( hwnd, IDC_SHOWVIS, Config.showVIS) ;       
         WriteCheckBoxValue( hwnd, IDC_ALERTBADROM, Config.alertBAD);
         WriteCheckBoxValue( hwnd, IDC_ALERTHACKEDROM, Config.alertHACK);  
         WriteCheckBoxValue( hwnd, IDC_ALERTSAVESERRORS, Config.savesERRORS);  
         WriteCheckBoxValue( hwnd, IDC_LIMITFPS, Config.limitFps);  
         WriteCheckBoxValue( hwnd, IDC_INI_COMPRESSED, Config.compressedIni);
         WriteCheckBoxValue( hwnd, IDC_SPEEDMODIFIER, Config.UseFPSmodifier  );
         WriteCheckBoxValue(hwnd, IDC_0INDEX, Config.zeroIndex);
         SetDlgItemInt(hwnd, IDC_SKIPFREQ, Config.skipFrequency,0);
               
         CreateToolTip(IDC_SKIPFREQ, hwnd, "0 = Skip all frames, 1 = Show all frames, n = show every nth frame");

         switch (Config.guiDynacore)    
            {        
               case 0:
                     CheckDlgButton(hwnd, IDC_INTERP, BST_CHECKED);
                     break;
               case 1:
                     CheckDlgButton(hwnd, IDC_RECOMP, BST_CHECKED);
                     break;             
               case 2:
                     CheckDlgButton(hwnd, IDC_PURE_INTERP, BST_CHECKED);             
                     break;      
             }       
         
         if (emu_launched) {
                  EnableWindow( GetDlgItem(hwnd,IDC_INTERP), FALSE );
                  EnableWindow( GetDlgItem(hwnd,IDC_RECOMP), FALSE );
                  EnableWindow( GetDlgItem(hwnd,IDC_PURE_INTERP), FALSE );
         }
         
         CreateTrackbar(hwnd,5,200,Config.FPSmodifier,200, 30, 184, 300) ; 
         
         SwitchLimitFPS(hwnd);
         FillModifierValue( hwnd, Config.FPSmodifier);        
         TranslateGeneralDialog(hwnd) ;                           
         return TRUE;
         
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
           case IDC_INTERP:
                if (!emu_launched) {
                   Config.guiDynacore = 0;
                   }
           break;
           case IDC_RECOMP:
                if (!emu_launched) {
                   Config.guiDynacore = 1;
                   }
           break;
           case IDC_PURE_INTERP:
                if (!emu_launched) {
                   Config.guiDynacore = 2;
                   }
           break;
           case IDC_LIMITFPS:
                SwitchLimitFPS(hwnd) ;
           break;
           case  IDC_SPEEDMODIFIER:
                SwitchModifier(hwnd)  ;
           break; 
                    
        }
        break;

    case WM_NOTIFY:
		       if (((NMHDR FAR *) lParam)->code == NM_RELEASEDCAPTURE)  {
                    FillModifierValue( hwnd, SendMessage( hwndTrack , TBM_GETPOS, 0, 0));        
               }
               if (((NMHDR FAR *) lParam)->code == PSN_APPLY)  {
                              Config.showFPS = ReadCheckBoxValue( hwnd, IDC_SHOWFPS);
                              Config.showVIS = ReadCheckBoxValue( hwnd, IDC_SHOWVIS);
                              Config.alertBAD = ReadCheckBoxValue(hwnd,IDC_ALERTBADROM);
                              Config.alertHACK = ReadCheckBoxValue(hwnd,IDC_ALERTHACKEDROM);
                              Config.savesERRORS = ReadCheckBoxValue(hwnd,IDC_ALERTSAVESERRORS);
                              Config.limitFps = ReadCheckBoxValue(hwnd,IDC_LIMITFPS);
                              Config.compressedIni = ReadCheckBoxValue(hwnd,IDC_INI_COMPRESSED);
                              Config.FPSmodifier = SendMessage( hwndTrack , TBM_GETPOS, 0, 0);
                              Config.UseFPSmodifier = ReadCheckBoxValue( hwnd , IDC_SPEEDMODIFIER );
                              Config.skipFrequency = GetDlgItemInt(hwnd, IDC_SKIPFREQ,0,0);
                              Config.zeroIndex = ReadCheckBoxValue(hwnd, IDC_0INDEX);
                              if (emu_launched) SetStatusMode( 2 );
                              else SetStatusMode( 0 );
                              InitTimer();
                              }
            break;                     
     default:
            return FALSE;       
     }
     return TRUE;            
}

DWORD WINAPI ScanThread(LPVOID lpParam) {
    
    int i;
    ROM_INFO *pRomInfo;
    md5_byte_t digest[16];
    char tempname[100];
    EnableWindow(GetDlgItem(romInfoHWND,IDC_STOP),TRUE);
    EnableWindow(GetDlgItem(romInfoHWND,IDC_START),FALSE);
    EnableWindow(GetDlgItem(romInfoHWND,IDC_CLOSE),FALSE);
     for (i=0;i<ItemList.ListCount;i++)
        {
          if (stopScan) break;
          pRomInfo = &ItemList.List[i]; 
          sprintf(TempMessage,"%d",i+1);
          SetDlgItemText(romInfoHWND,IDC_CURRENT_ROM,TempMessage);
          SetDlgItemText(romInfoHWND,IDC_ROM_FULLPATH,pRomInfo->szFullFileName);
          //SendMessage( GetDlgItem(romInfoHWND, IDC_TOTAL_ROMS_PROGRESS), PBM_STEPIT, 0, 0 ); 
          SendMessage( GetDlgItem(romInfoHWND, IDC_TOTAL_ROMS_PROGRESS), PBM_SETPOS, i+1, 0 );
                 
          strcpy(TempMessage,pRomInfo->MD5);
          if (!strcmp(TempMessage,"")) {
                    calculateMD5(pRomInfo->szFullFileName,digest);
                    MD5toString(digest,TempMessage);
          }
          strcpy(pRomInfo->MD5,TempMessage);
          if(getIniGoodNameByMD5(TempMessage,tempname))
	          strcpy(pRomInfo->GoodName,tempname);
	      else
	          sprintf(pRomInfo->GoodName,"%s (not found in INI file)", pRomInfo->InternalName);
        }
     //SendMessage( GetDlgItem(romInfoHWND, IDC_TOTAL_ROMS_PROGRESS), PBM_SETPOS, 0, 0 );
     EnableWindow(GetDlgItem(romInfoHWND,IDC_STOP),FALSE);
     EnableWindow(GetDlgItem(romInfoHWND,IDC_START),TRUE);
     EnableWindow(GetDlgItem(romInfoHWND,IDC_CLOSE),TRUE);
    ExitThread(dwExitCode);
}

BOOL CALLBACK AuditDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    ROM_INFO *pRomInfo;
    HWND hwndPB1, hwndPB2;    //Progress Bar
    
    switch(Message) {
    case WM_INITDIALOG:
         pRomInfo = &ItemList.List[0];
         SetDlgItemText(hwnd,IDC_ROM_FULLPATH,pRomInfo->szFullFileName);
         sprintf(TempMessage,"%d",ItemList.ListCount);
         SetDlgItemText(hwnd,IDC_TOTAL_ROMS,TempMessage);
         SetDlgItemText(hwnd,IDC_CURRENT_ROM,"");
         
         hwndPB1 = GetDlgItem( hwnd, IDC_CURRENT_ROM_PROGRESS );
         hwndPB2 = GetDlgItem( hwnd, IDC_TOTAL_ROMS_PROGRESS );
         SendMessage( hwndPB1, PBM_SETRANGE, 0, MAKELPARAM(0, 100) ); 
         SendMessage( hwndPB1, PBM_SETSTEP, (WPARAM) 1, 0 ); 
         SendMessage( hwndPB2, PBM_SETRANGE, 0, MAKELPARAM(0, ItemList.ListCount) ); 
         SendMessage( hwndPB2, PBM_SETSTEP, (WPARAM) 1, 0 ); 
        
         
         TranslateAuditDialog(hwnd);
         return TRUE;
    case WM_COMMAND:
          switch(LOWORD(wParam))
            {
                case IDC_START:
                    romInfoHWND = hwnd;
                    stopScan = FALSE;
                    EmuThreadHandle = CreateThread(NULL, 0, ScanThread, NULL, 0, &Id);
                break;
                case IDC_STOP:
                     stopScan = TRUE; 
                break;
                case IDCANCEL:
                case IDC_CLOSE:
                     stopScan = TRUE;
                     romInfoHWND = NULL;
                     //FastRefreshBrowser();
                     if (!emu_launched) {
                        ShowWindow( hRomList, FALSE ); 
                        ShowWindow( hRomList, TRUE );
                     }
                     
                     EndDialog(hwnd, IDOK);
                break;
            }             
     default:
            return FALSE;       
     }       
}
BOOL CALLBACK LangInfoProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch(Message) {
    case WM_INITDIALOG:
         TranslateLangInfoDialog(hwnd);
         return TRUE;
    case WM_COMMAND:
          switch(LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                case IDC_CLOSE:
                     EndDialog(hwnd, IDOK);
                break;
            }             
     default:
            return FALSE;       
     }       
}

BOOL CALLBACK AdvancedSettingsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
        
    switch(Message) {
    
      case WM_INITDIALOG:
         WriteCheckBoxValue( hwnd, IDC_STARTFULLSCREEN, Config.StartFullScreen);
         WriteCheckBoxValue( hwnd, IDC_PAUSENOTACTIVE, Config.PauseWhenNotActive);
         WriteCheckBoxValue( hwnd, IDC_PLUGIN_OVERWRITE, Config.OverwritePluginSettings);      
         WriteCheckBoxValue( hwnd, IDC_GUI_TOOLBAR, Config.GuiToolbar);
         WriteCheckBoxValue( hwnd, IDC_GUI_STATUSBAR, Config.GuiStatusbar);
         WriteCheckBoxValue( hwnd, IDC_AUTOINCSAVESLOT, Config.AutoIncSaveSlot);
         WriteCheckBoxValue( hwnd, IDC_ROUNDTOZERO, round_to_zero);
         WriteCheckBoxValue( hwnd, IDC_EMULATEFLOATCRASHES, emulate_float_crashes);
         WriteCheckBoxValue(hwnd, IDC_INPUTDELAY, input_delay);
         EnableWindow(GetDlgItem(hwnd, IDC_INPUTDELAY), false); //disable for now
         WriteCheckBoxValue(hwnd, IDC_CLUADOUBLEBUFFER, LUA_double_buffered);
         WriteCheckBoxValue( hwnd, IDC_NO_AUDIO_DELAY, no_audio_delay);
         WriteCheckBoxValue( hwnd, IDC_NO_COMPILED_JUMP, no_compiled_jump);
		 WriteCheckBoxValue( hwnd, IDC_SUPPRESS_LOAD_ST_PROMPT, Config.IgnoreStWarnings);
         
         WriteCheckBoxValue( hwnd, IDC_COLUMN_GOODNAME, Config.Column_GoodName);
         WriteCheckBoxValue( hwnd, IDC_COLUMN_INTERNALNAME, Config.Column_InternalName);
         WriteCheckBoxValue( hwnd, IDC_COLUMN_COUNTRY, Config.Column_Country);
         WriteCheckBoxValue( hwnd, IDC_COLUMN_SIZE, Config.Column_Size);
         WriteCheckBoxValue( hwnd, IDC_COLUMN_COMMENTS, Config.Column_Comments);
         WriteCheckBoxValue( hwnd, IDC_COLUMN_FILENAME, Config.Column_FileName);
         WriteCheckBoxValue( hwnd, IDC_COLUMN_MD5, Config.Column_MD5);
         
         WriteCheckBoxValue(hwnd, IDC_NORESET, !Config.NoReset);

         WriteCheckBoxValue(hwnd, IDC_FORCEINTERNAL, Config.forceInternalCapture);

         TranslateAdvancedDialog(hwnd) ;                           
         return TRUE;
         
      

       case WM_NOTIFY:
           if (((NMHDR FAR *) lParam)->code == PSN_APPLY)  {
                Config.StartFullScreen = ReadCheckBoxValue( hwnd, IDC_STARTFULLSCREEN);
                Config.PauseWhenNotActive =  ReadCheckBoxValue( hwnd, IDC_PAUSENOTACTIVE); 
                Config.OverwritePluginSettings =  ReadCheckBoxValue( hwnd, IDC_PLUGIN_OVERWRITE);
                Config.GuiToolbar =  ReadCheckBoxValue( hwnd, IDC_GUI_TOOLBAR);
                Config.GuiStatusbar = ReadCheckBoxValue( hwnd, IDC_GUI_STATUSBAR);
	            Config.AutoIncSaveSlot = ReadCheckBoxValue( hwnd, IDC_AUTOINCSAVESLOT);
                round_to_zero = ReadCheckBoxValue( hwnd, IDC_ROUNDTOZERO);
                emulate_float_crashes = ReadCheckBoxValue( hwnd, IDC_EMULATEFLOATCRASHES);
                input_delay = ReadCheckBoxValue(hwnd, IDC_INPUTDELAY);
                
                LUA_double_buffered = ReadCheckBoxValue(hwnd, IDC_CLUADOUBLEBUFFER);
                if (LUA_double_buffered && gfx_name[0] != 0 && strstr(gfx_name, "Jabo") == 0 && strstr(gfx_name, "Rice") == 0) {
                    //CheckDlgButton(hwnd, IDC_CLUADOUBLEBUFFER, 0);
                    //LUA_double_buffered = false;
                    MessageBoxA(mainHWND, "Your current video plugin might produce unexpected results with LUA double buffering.", "Incompatible Plugin", MB_TASKMODAL | MB_TOPMOST | MB_ICONWARNING/*make sure to annoy user a lot*/);
                }
                no_audio_delay = ReadCheckBoxValue( hwnd, IDC_NO_AUDIO_DELAY);
                no_compiled_jump = ReadCheckBoxValue( hwnd, IDC_NO_COMPILED_JUMP);
				Config.IgnoreStWarnings = ReadCheckBoxValue( hwnd, IDC_SUPPRESS_LOAD_ST_PROMPT);
                
                Config.Column_GoodName = ReadCheckBoxValue( hwnd, IDC_COLUMN_GOODNAME);
                Config.Column_InternalName = ReadCheckBoxValue( hwnd, IDC_COLUMN_INTERNALNAME);
                Config.Column_Country = ReadCheckBoxValue( hwnd, IDC_COLUMN_COUNTRY);
                Config.Column_Size = ReadCheckBoxValue( hwnd, IDC_COLUMN_SIZE);
                Config.Column_Comments = ReadCheckBoxValue( hwnd, IDC_COLUMN_COMMENTS);
                Config.Column_FileName = ReadCheckBoxValue( hwnd, IDC_COLUMN_FILENAME);
                Config.Column_MD5 = ReadCheckBoxValue( hwnd, IDC_COLUMN_MD5); 

                Config.NoReset = !ReadCheckBoxValue(hwnd, IDC_NORESET);
                Config.forceInternalCapture = ReadCheckBoxValue(hwnd, IDC_FORCEINTERNAL);
                
                EnableToolbar(); 
                EnableStatusbar();
                FastRefreshBrowser();
				LoadConfigExternals();
           }
       break;
                            
       default:
           return FALSE;       
    }
    return TRUE;            
}

void hotkeyToString(HOTKEY* hotkey, char* buf)
{
	int k = hotkey->key;
	buf[0] = 0;
	
	if(!hotkey->ctrl && !hotkey->shift && !hotkey->alt && !hotkey->key)
	{
		strcpy(buf, "(nothing)");
		return;
	}
	
	if(hotkey->ctrl)
		strcat(buf, "Ctrl ");
	if(hotkey->shift)
		strcat(buf, "Shift ");
	if(hotkey->alt)
		strcat(buf, "Alt ");
	if(k)
	{
		char buf2 [32];
		if((k >= '0' && k <= '9') || (k >= 'A' && k <= 'Z'))
			sprintf(buf2, "%c", (char)k);
		else if((k >= VK_F1 && k <= VK_F24))
			sprintf(buf2, "F%d", k - (VK_F1-1));
		else if((k >= VK_NUMPAD0 && k <= VK_NUMPAD9))
			sprintf(buf2, "Num%d", k - VK_NUMPAD0);
		else switch(k)
		{
			case VK_SPACE: strcpy(buf2, "Space"); break;
			case VK_BACK: strcpy(buf2, "Backspace"); break;
			case VK_TAB: strcpy(buf2, "Tab"); break;
			case VK_CLEAR: strcpy(buf2, "Clear"); break;
			case VK_RETURN: strcpy(buf2, "Enter"); break;
			case VK_PAUSE: strcpy(buf2, "Pause"); break;
			case VK_CAPITAL: strcpy(buf2, "Caps"); break;
			case VK_PRIOR: strcpy(buf2, "PageUp"); break;
			case VK_NEXT: strcpy(buf2, "PageDn"); break;
			case VK_END: strcpy(buf2, "End"); break;
			case VK_HOME: strcpy(buf2, "Home"); break;
			case VK_LEFT: strcpy(buf2, "Left"); break;
			case VK_UP: strcpy(buf2, "Up"); break;
			case VK_RIGHT: strcpy(buf2, "Right"); break;
			case VK_DOWN: strcpy(buf2, "Down"); break;
			case VK_SELECT: strcpy(buf2, "Select"); break;
			case VK_PRINT: strcpy(buf2, "Print"); break;
			case VK_SNAPSHOT: strcpy(buf2, "PrintScrn"); break;
			case VK_INSERT: strcpy(buf2, "Insert"); break;
			case VK_DELETE: strcpy(buf2, "Delete"); break;
			case VK_HELP: strcpy(buf2, "Help"); break;
			case VK_MULTIPLY: strcpy(buf2, "Num*"); break;
			case VK_ADD: strcpy(buf2, "Num+"); break;
			case VK_SUBTRACT: strcpy(buf2, "Num-"); break;
			case VK_DECIMAL: strcpy(buf2, "Num."); break;
			case VK_DIVIDE: strcpy(buf2, "Num/"); break;
			case VK_NUMLOCK: strcpy(buf2, "NumLock"); break;
			case VK_SCROLL: strcpy(buf2, "ScrollLock"); break;
			case /*VK_OEM_PLUS*/0xBB: strcpy(buf2, "=+"); break;
			case /*VK_OEM_MINUS*/0xBD: strcpy(buf2, "-_"); break;
			case /*VK_OEM_COMMA*/0xBC: strcpy(buf2, ","); break;
			case /*VK_OEM_PERIOD*/0xBE: strcpy(buf2, "."); break;
			case VK_OEM_7: strcpy(buf2, "'\""); break;
			case VK_OEM_6: strcpy(buf2, "]}"); break;
			case VK_OEM_5: strcpy(buf2, "\\|"); break;
			case VK_OEM_4: strcpy(buf2, "[{"); break;
			case VK_OEM_3: strcpy(buf2, "`~"); break;
			case VK_OEM_2: strcpy(buf2, "/?"); break;
			case VK_OEM_1: strcpy(buf2, ";:"); break;
			default:
				sprintf(buf2, "(%d)", k);
				break;
		}
		strcat(buf, buf2);
	}
}

static void SetDlgItemHotkey(HWND hwnd, int idc, HOTKEY* hotkey)
{
    char buf [64];
	hotkeyToString(hotkey, buf);
    SetDlgItemText(hwnd, idc, buf);
}

static void SetDlgItemHotkeyAndMenu(HWND hwnd, int idc, HOTKEY* hotkey, HMENU hmenu, int menuItemID)
{
    char buf [64];
	hotkeyToString(hotkey, buf);
    SetDlgItemText(hwnd, idc, buf);

	if(hmenu && menuItemID >= 0)
	{
		if(strcmp(buf, "(nothing)"))
			SetMenuAccelerator(hmenu,menuItemID,buf);
		else
			SetMenuAccelerator(hmenu,menuItemID,"");
	}
}

static void KillMessages()
{
	MSG Msg;
	int i = 0;
    while(GetMessage(&Msg, NULL, 0, 0) > 0 && i < 20)
    {
//		TranslateMessage(&Msg);
//		DispatchMessage(&Msg);
		i++;
	}
}   

static void GetUserHotkey(HOTKEY * hotkey)
{
	int i, j;
	int lc=0, ls=0, la=0;
	for(i = 0 ; i < 500 ; i++)
	{
		SleepEx(10,TRUE);
		for(j = 8 ; j < 254 ; j++)
		{
			if(j == VK_LCONTROL || j == VK_RCONTROL || j == VK_LMENU || j == VK_RMENU || j == VK_LSHIFT || j == VK_RSHIFT)
				continue;
			
			if(GetAsyncKeyState(j) & 0x8000)
			{
				// HACK to avoid exiting all the way out of the dialog on pressing escape to clear a hotkey
				//               or continually re-activating the button on trying to assign space as a hotkey
				if(j == VK_ESCAPE || j == VK_SPACE)
					KillMessages();

				if(j == VK_CONTROL)
				{
					lc=1;
					continue;
				}
				else if(j == VK_SHIFT)
				{
					ls=1;
					continue;
				}
				else if(j == VK_MENU)
				{
					la=1;
					continue;
				}
				else if(j != VK_ESCAPE)
				{
					hotkey->key = j;
					hotkey->shift = GetAsyncKeyState(VK_SHIFT) ? 1 : 0;
					hotkey->ctrl = GetAsyncKeyState(VK_CONTROL) ? 1 : 0;
					hotkey->alt = GetAsyncKeyState(VK_MENU) ? 1 : 0;
					return;
				}
				memset(hotkey, 0, sizeof(HOTKEY)); // clear key on escape
				return;
			}
			else
			{
				if(j == VK_CONTROL && lc)
				{
					hotkey->key = 0;
					hotkey->shift = 0;
					hotkey->ctrl = 1;
					hotkey->alt = 0;
					return;
				}
				else if(j == VK_SHIFT && ls)
				{
					hotkey->key = 0;
					hotkey->shift = 1;
					hotkey->ctrl = 0;
					hotkey->alt = 0;
					return;
				}
				else if(j == VK_MENU && la)
				{
					hotkey->key = 0;
					hotkey->shift = 0;
					hotkey->ctrl = 0;
					hotkey->alt = 1;
					return;
				}
			}
		}
	}
}



HOTKEY tempHotkeys [NUM_HOTKEYS];

BOOL CALLBACK HotkeysProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message) {
      case WM_INITDIALOG:
inithotkeysdialog:
		SetDlgItemHotkey(hwnd, IDC_HOT_FASTFORWARD, &Config.hotkey[0]);
		SetDlgItemHotkey(hwnd, IDC_HOT_SPEEDUP, &Config.hotkey[1]);
		SetDlgItemHotkey(hwnd, IDC_HOT_SPEEDDOWN, &Config.hotkey[2]);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_HOT_FRAMEADVANCE, &Config.hotkey[3], GetSubMenu(GetMenu(mainHWND),1), 1);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_HOT_PAUSE, &Config.hotkey[4], GetSubMenu(GetMenu(mainHWND),1), 0);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_HOT_READONLY, &Config.hotkey[5], GetSubMenu(GetSubMenu(GetMenu(mainHWND),3),6), 5);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_HOT_PLAY, &Config.hotkey[6], GetSubMenu(GetSubMenu(GetMenu(mainHWND),3),6), 3);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_HOT_PLAYSTOP, &Config.hotkey[7], GetSubMenu(GetSubMenu(GetMenu(mainHWND),3),6), 4);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_HOT_RECORD, &Config.hotkey[8], GetSubMenu(GetSubMenu(GetMenu(mainHWND),3),6), 0);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_HOT_RECORDSTOP, &Config.hotkey[9], GetSubMenu(GetSubMenu(GetMenu(mainHWND),3),6), 1);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_HOT_SCREENSHOT, &Config.hotkey[10], GetSubMenu(GetMenu(mainHWND),1), 2);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_CSAVE, &Config.hotkey[11], GetSubMenu(GetMenu(mainHWND),1), 4);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_CLOAD, &Config.hotkey[12], GetSubMenu(GetMenu(mainHWND),1), 6);

		SetDlgItemHotkey(hwnd, IDC_1SAVE, &Config.hotkey[13]);
		SetDlgItemHotkey(hwnd, IDC_2SAVE, &Config.hotkey[14]);
		SetDlgItemHotkey(hwnd, IDC_3SAVE, &Config.hotkey[15]);
		SetDlgItemHotkey(hwnd, IDC_4SAVE, &Config.hotkey[16]);
		SetDlgItemHotkey(hwnd, IDC_5SAVE, &Config.hotkey[17]);
		SetDlgItemHotkey(hwnd, IDC_6SAVE, &Config.hotkey[18]);
		SetDlgItemHotkey(hwnd, IDC_7SAVE, &Config.hotkey[19]);
		SetDlgItemHotkey(hwnd, IDC_8SAVE, &Config.hotkey[20]);
		SetDlgItemHotkey(hwnd, IDC_9SAVE, &Config.hotkey[21]);

		SetDlgItemHotkey(hwnd, IDC_1LOAD, &Config.hotkey[22]);
		SetDlgItemHotkey(hwnd, IDC_2LOAD, &Config.hotkey[23]);
		SetDlgItemHotkey(hwnd, IDC_3LOAD, &Config.hotkey[24]);
		SetDlgItemHotkey(hwnd, IDC_4LOAD, &Config.hotkey[25]);
		SetDlgItemHotkey(hwnd, IDC_5LOAD, &Config.hotkey[26]);
		SetDlgItemHotkey(hwnd, IDC_6LOAD, &Config.hotkey[27]);
		SetDlgItemHotkey(hwnd, IDC_7LOAD, &Config.hotkey[28]);
		SetDlgItemHotkey(hwnd, IDC_8LOAD, &Config.hotkey[29]);
		SetDlgItemHotkey(hwnd, IDC_9LOAD, &Config.hotkey[30]);

		SetDlgItemHotkeyAndMenu(hwnd, IDC_1SEL, &Config.hotkey[31], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 0);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_2SEL, &Config.hotkey[32], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 1);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_3SEL, &Config.hotkey[33], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 2);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_4SEL, &Config.hotkey[34], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 3);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_5SEL, &Config.hotkey[35], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 4);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_6SEL, &Config.hotkey[36], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 5);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_7SEL, &Config.hotkey[37], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 6);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_8SEL, &Config.hotkey[38], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 7);
		SetDlgItemHotkeyAndMenu(hwnd, IDC_9SEL, &Config.hotkey[39], GetSubMenu(GetSubMenu(GetMenu(mainHWND),1),9), 8);

		memcpy(tempHotkeys, Config.hotkey, NUM_HOTKEYS * sizeof(HOTKEY));

         return TRUE;
      case WM_COMMAND:
        switch(LOWORD(wParam))
        {

#define HOTKEY_MACRO(IDC,i) \
	case IDC: \
		SetDlgItemText(hwnd, IDC, "...type hotkey..."); \
		GetUserHotkey(&tempHotkeys[i]); \
		SetDlgItemHotkey(hwnd, IDC, &tempHotkeys[i]); \
		break

			HOTKEY_MACRO(IDC_HOT_FASTFORWARD, 0);
			HOTKEY_MACRO(IDC_HOT_SPEEDUP, 1);
			HOTKEY_MACRO(IDC_HOT_SPEEDDOWN, 2);
			HOTKEY_MACRO(IDC_HOT_FRAMEADVANCE, 3);
			HOTKEY_MACRO(IDC_HOT_PAUSE, 4);
			HOTKEY_MACRO(IDC_HOT_READONLY, 5);
			HOTKEY_MACRO(IDC_HOT_PLAY, 6);
			HOTKEY_MACRO(IDC_HOT_PLAYSTOP, 7);
			HOTKEY_MACRO(IDC_HOT_RECORD, 8);
			HOTKEY_MACRO(IDC_HOT_RECORDSTOP, 9);
			HOTKEY_MACRO(IDC_HOT_SCREENSHOT, 10);
			HOTKEY_MACRO(IDC_CSAVE, 11);
			HOTKEY_MACRO(IDC_CLOAD, 12);
			HOTKEY_MACRO(IDC_1SAVE, 13);
			HOTKEY_MACRO(IDC_2SAVE, 14);
			HOTKEY_MACRO(IDC_3SAVE, 15);
			HOTKEY_MACRO(IDC_4SAVE, 16);
			HOTKEY_MACRO(IDC_5SAVE, 17);
			HOTKEY_MACRO(IDC_6SAVE, 18);
			HOTKEY_MACRO(IDC_7SAVE, 19);
			HOTKEY_MACRO(IDC_8SAVE, 20);
			HOTKEY_MACRO(IDC_9SAVE, 21);
			HOTKEY_MACRO(IDC_1LOAD, 22);
			HOTKEY_MACRO(IDC_2LOAD, 23);
			HOTKEY_MACRO(IDC_3LOAD, 24);
			HOTKEY_MACRO(IDC_4LOAD, 25);
			HOTKEY_MACRO(IDC_5LOAD, 26);
			HOTKEY_MACRO(IDC_6LOAD, 27);
			HOTKEY_MACRO(IDC_7LOAD, 28);
			HOTKEY_MACRO(IDC_8LOAD, 29);
			HOTKEY_MACRO(IDC_9LOAD, 30);
			HOTKEY_MACRO(IDC_1SEL, 31);
			HOTKEY_MACRO(IDC_2SEL, 32);
			HOTKEY_MACRO(IDC_3SEL, 33);
			HOTKEY_MACRO(IDC_4SEL, 34);
			HOTKEY_MACRO(IDC_5SEL, 35);
			HOTKEY_MACRO(IDC_6SEL, 36);
			HOTKEY_MACRO(IDC_7SEL, 37);
			HOTKEY_MACRO(IDC_8SEL, 38);
			HOTKEY_MACRO(IDC_9SEL, 39);
				
#undef HOTKEY_MACRO
        }
        break;

       case WM_NOTIFY:
           if (((NMHDR FAR *) lParam)->code == PSN_APPLY)  {
				memcpy(Config.hotkey, tempHotkeys, NUM_HOTKEYS * sizeof(HOTKEY));
				goto inithotkeysdialog;
           }
       break;
                            
       default:
           return FALSE;       
    }
    return TRUE;            
}



HWND WINAPI CreateTrackbar( 
    HWND hwndDlg,  // handle of dialog box (parent window) 
    UINT iMin,     // minimum value in trackbar range 
    UINT iMax,     // maximum value in trackbar range 
    UINT iSelMin,  // minimum value in trackbar selection 
    UINT iSelMax, // maximum value in trackbar selection 
    UINT x, // x pos
    UINT y, // y pos
    UINT w
    )// width
{  

    
    hwndTrack = CreateWindowEx( 
        0,                             // no extended styles 
        TRACKBAR_CLASS,                // class name 
        "Trackbar Control",            // title (caption) 
        WS_CHILD | WS_VISIBLE | 
         /*TBS_TOOLTIPS |*/ TBS_FIXEDLENGTH ,  // style 
        x, y,                        // position 
        w, 30,                       // size 
        hwndDlg,                       // parent window 
        (HMENU)ID_FPSTRACKBAR,               // control identifier 
        app_hInstance,                 // instance 
        NULL) ;                          // no WM_CREATE parameter 
         

    SendMessage(hwndTrack, TBM_SETRANGE, 
        (WPARAM) TRUE,                   // redraw flag 
        (LPARAM) MAKELONG(iMin, iMax));  // min. & max. positions 
    SendMessage(hwndTrack, TBM_SETPAGESIZE, 
        0, (LPARAM) 10);                  // new page size 
    SendMessage(hwndTrack, TBM_SETLINESIZE, 
        0, (LPARAM) 10);
    SendMessage(hwndTrack, TBM_SETTIC, 
        0, (LPARAM) 100);              
/*
    SendMessage(hwndTrack, TBM_SETSEL, 
        (WPARAM) FALSE,                  // redraw flag 
        (LPARAM) MAKELONG(iSelMin, iSelMax)); 
*/        
    SendMessage(hwndTrack, TBM_SETPOS, 
        (WPARAM) TRUE,                   // redraw flag 
        (LPARAM) iSelMin); 

    SetFocus(hwndTrack); 

    return hwndTrack; 
} 

