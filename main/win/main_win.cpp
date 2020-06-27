/***************************************************************************
                          main_win.cpp  -  description
                             -------------------
    copyright C) 2003    : ShadowPrince (shadow@emulation64.com)
    modifications        : linker (linker@mail.bg)
    mupen64 author       : hacktarux (hacktarux@yahoo.fr)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "LuaConsole.h"
#include "win/DebugInfo.hpp"
	
#if defined(__cplusplus) && !defined(_MSC_VER)
extern "C" {
#endif

#include <windows.h>
#include <Shlwapi.h>
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif
#include <commctrl.h>
#include <stdlib.h>
#include <math.h>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include "../../winproject/resource.h"
#include "../plugin.h"
#include "../rom.h"
#include "../../r4300/r4300.h"
#include "../../memory/memory.h"
#include "translation.h"
#include "rombrowser.h"
#include "main_win.h"
#include "configdialog.h"
#include "../guifuncs.h"
#include "../mupenIniApi.h"
#include "../savestates.h"
#include "dumplist.h"
#include "timers.h"
#include "config.h"
#include "RomSettings.h"
#include "GUI_logwindow.h"
#include "commandline.h"

#include "../vcr.h"
#include "../../r4300/recomph.h"

#define EMULATOR_MAIN_CPP_DEF
#include "kaillera.h"
#undef EMULATOR_MAIN_CPP_DEF

extern void CountryCodeToCountryName(int countrycode,char *countryname);

typedef std::string String;

#if defined(__cplusplus) && !defined(_MSC_VER)
}
#endif



#ifdef _MSC_VER
#define snprintf	_snprintf
#define strcasecmp	stricmp
#define strncasecmp	strnicmp
#endif


static DWORD Id;
static DWORD SOUNDTHREADID;
static HANDLE SoundThreadHandle;
static BOOL FullScreenMode = 0;
static DWORD WINAPI closeRom(LPVOID lpParam);

HANDLE EmuThreadHandle;
HWND hwnd_plug;
//int manualFPSLimit = 1 ;
static void gui_ChangeWindow();


static GFX_INFO dummy_gfx_info;
static GFX_INFO gfx_info;
static CONTROL_INFO dummy_control_info;
static CONTROL_INFO control_info;
static AUDIO_INFO dummy_audio_info;
static AUDIO_INFO audio_info;
static RSP_INFO dummy_rsp_info;
static RSP_INFO rsp_info;
static int currentSaveState = 1 ;
static unsigned char DummyHeader[0x40];


void (__cdecl*moveScreen)(int xpos, int ypos);
void (__cdecl*CaptureScreen) ( char * Directory );
void (__cdecl*old_initiateControllers)(HWND hMainWindow, CONTROL Controls[4]);
void (__cdecl*aiUpdate)(BOOL Wait);


#ifndef _MSC_VER

CONTROL Controls[4];

void (__cdecl*getDllInfo)(PLUGIN_INFO *PluginInfo);
void (__cdecl*dllConfig)(HWND hParent);
void (__cdecl*dllTest)(HWND hParent);
void (__cdecl*dllAbout)(HWND hParent);

void (__cdecl*changeWindow)();
void (__cdecl*closeDLL_gfx)();
BOOL (__cdecl*initiateGFX)(GFX_INFO Gfx_Info);
void (__cdecl*processDList)();
void (__cdecl*processRDPList)();
void (__cdecl*romClosed_gfx)();
void (__cdecl*romOpen_gfx)();
void (__cdecl*showCFB)();
void (__cdecl*updateScreen)();
void (__cdecl*viStatusChanged)();
void (__cdecl*viWidthChanged)();


void (__cdecl*closeDLL_input)();
void (__cdecl*controllerCommand)(int Control, BYTE * Command);
void (__cdecl*getKeys)(int Control, BUTTONS *Keys);
void (__cdecl*initiateControllers)(CONTROL_INFO ControlInfo);
void (__cdecl*readController)(int Control, BYTE *Command);
void (__cdecl*romClosed_input)();
void (__cdecl*romOpen_input)();
void (__cdecl*keyDown)(WPARAM wParam, LPARAM lParam);
void (__cdecl*keyUp)(WPARAM wParam, LPARAM lParam);


void (__cdecl*aiDacrateChanged)(int SystemType);
void (__cdecl*aiLenChanged)();
DWORD (__cdecl*aiReadLength)();
void (__cdecl*closeDLL_audio)();
BOOL (__cdecl*initiateAudio)(AUDIO_INFO Audio_Info);
void (__cdecl*processAList)();
void (__cdecl*romClosed_audio)();
void (__cdecl*romOpen_audio)();

void (__cdecl*closeDLL_RSP)() ;
DWORD (__cdecl*doRspCycles)(DWORD Cycles) ;
void (__cdecl*initiateRSP)(RSP_INFO Rsp_Info, DWORD * CycleCount) ;
void (__cdecl*romClosed_RSP)() ; 

void (__cdecl*fBRead)(DWORD addr);
void (__cdecl*fBWrite)(DWORD addr, DWORD size);
void (__cdecl*fBGetFrameBufferInfo)(void *p);
extern void (__cdecl*readScreen)(void **dest, long *width, long *height);

#endif // !_MSC_VER

/* dummy functions to prevent mupen from crashing if a plugin is missing */
static void __cdecl dummy_void() {}
static BOOL __cdecl dummy_initiateGFX(GFX_INFO Gfx_Info) { return TRUE; }
static BOOL __cdecl dummy_initiateAudio(AUDIO_INFO Audio_Info) { return TRUE; }
static void __cdecl dummy_initiateControllers(CONTROL_INFO Control_Info) {}
static void __cdecl dummy_aiDacrateChanged(int SystemType) {}
static DWORD __cdecl dummy_aiReadLength() { return 0; }
//static void dummy_aiUpdate(BOOL Wait) {}
static void __cdecl dummy_controllerCommand(int Control, BYTE * Command) {}
static void __cdecl dummy_getKeys(int Control, BUTTONS *Keys) {}
static void __cdecl dummy_readController(int Control, BYTE *Command) {}
static void __cdecl dummy_keyDown(WPARAM wParam, LPARAM lParam) {}
static void __cdecl dummy_keyUp(WPARAM wParam, LPARAM lParam) {}
static unsigned long dummy;
static DWORD __cdecl dummy_doRspCycles(DWORD Cycles) { return Cycles; };
static void __cdecl dummy_initiateRSP(RSP_INFO Rsp_Info, DWORD * CycleCount) {};

static DWORD WINAPI ThreadFunc(LPVOID lpParam);

const char g_szClassName[] = "myWindowClass";
char LastSelectedRom[_MAX_PATH];
static BOOL restart_mode = 0;
BOOL really_restart_mode = 0;
BOOL clear_sram_on_restart_mode = 0;
BOOL continue_vcr_on_restart_mode = 0;
BOOL just_restarted_flag = 0;
static int InputPluginVersion;
static BOOL AutoPause = 0;
static BOOL MenuPaused = 0;
//extern int recording;
//static HICON hStatusIcon;                                  //Icon Handle for statusbar
static HWND hStaticHandle;                                 //Handle for static place
int externalReadScreen;

char TempMessage[800];
int emu_launched; // emu_emulating
int emu_paused;
int recording;
HWND hTool, mainHWND, hStatus, hRomList, hStatusProgress;
HINSTANCE app_hInstance;
BOOL manualFPSLimit = TRUE;
char statusmsg[800];

char gfx_name[255];
char input_name[255];
char sound_name[255];
char rsp_name[255];


enum EThreadFuncState {
	TFS_INITMEM,
	TFS_LOADPLUGINS,
	TFS_LOADGFX,
	TFS_LOADINPUT,
	TFS_LOADAUDIO,
	TFS_LOADRSP,
	TFS_OPENGFX,
	TFS_OPENINPUT,
	TFS_OPENAUDIO,
	TFS_DISPLAYMODE,
	TFS_CREATESOUND,
	TFS_EMULATING,
	TFS_CLOSEINPUT,
	TFS_CLOSEAUDIO,
	TFS_CLOSERSP,
	TFS_UNLOADRSP,
	TFS_UNLOADINPUT,
	TFS_UNLOADAUDIO,
	TFS_CLOSEGFX,
	TFS_UNLOADGFX,
	TFS_NONE
};
const char * const ThreadFuncStateDescription [] = {
	"initializing memory",
	"loading plugins",
	"loading graphics plugin",
	"loading input plugin",
	"loading audio plugin",
	"loading RSP plugin",
	"opening graphics",
	"opening input",
	"opening audio",
	"setting display mode",
	"creating sound thread",
	"emulating",
	"closing the input plugin",
	"closing the audio plugin",
	"closing the RSP plugin",
	"unloading the RSP plugin DLL",
	"unloading the input plugin DLL",
	"unloading the audio plugin DLL",
	"closing the graphics plugin",
	"unloading the graphics plugin DLL",
	"(unknown)"
};
static int ThreadFuncState = TFS_NONE;


TCHAR CoreNames[3][30] = {TEXT("Interpreter"), TEXT("Dynamic Recompiler"), TEXT("Pure Interpreter")} ;

char AppPath[MAX_PATH];

void getAppFullPath (char *ret) {
    char drive[_MAX_DRIVE], dirn[_MAX_DIR] ;
	char fname[_MAX_FNAME], ext[_MAX_EXT] ;
	char path_buffer[_MAX_DIR] ;
	
	GetModuleFileName(NULL, path_buffer, sizeof(path_buffer)); 
    _splitpath(path_buffer, drive, dirn, fname, ext);
    strcpy(ret, drive);
    strcat(ret, dirn);
}

static void __cdecl sucre()
{
   //printf("sucre\n");
}

static void gui_ChangeWindow()
{
    if( FullScreenMode )
	{
		EnableWindow( hTool, FALSE);
		ShowWindow( hTool, SW_HIDE);
		ShowWindow( hStatus, SW_HIDE);
		ShowCursor(FALSE);
		changeWindow();
	}
	else
	{
		changeWindow();
        ShowWindow( hTool, SW_SHOW);
        EnableWindow( hTool, TRUE);
		ShowWindow( hStatus, SW_SHOW);
		ShowCursor(TRUE);
	}

}

void SetupDummyInfo()
{
     int i;
   
     /////// GFX ///////////////////////////
     //dummy_gfx_info.hWnd = mainHWND;
     dummy_gfx_info.hWnd = hStatus;
     dummy_gfx_info.hStatusBar = hStatus ;
     dummy_gfx_info.MemoryBswaped = TRUE;
     dummy_gfx_info.HEADER = (BYTE*)DummyHeader;
     dummy_gfx_info.RDRAM = (BYTE*)rdram;
     dummy_gfx_info.DMEM = (BYTE*)SP_DMEM;
     dummy_gfx_info.IMEM = (BYTE*)SP_IMEM;
     dummy_gfx_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
     dummy_gfx_info.DPC_START_REG = &(dpc_register.dpc_start);
     dummy_gfx_info.DPC_END_REG = &(dpc_register.dpc_end);
     dummy_gfx_info.DPC_CURRENT_REG = &(dpc_register.dpc_current);
     dummy_gfx_info.DPC_STATUS_REG = &(dpc_register.dpc_status);
     dummy_gfx_info.DPC_CLOCK_REG = &(dpc_register.dpc_clock);
     dummy_gfx_info.DPC_BUFBUSY_REG = &(dpc_register.dpc_bufbusy);
     dummy_gfx_info.DPC_PIPEBUSY_REG = &(dpc_register.dpc_pipebusy);
     dummy_gfx_info.DPC_TMEM_REG = &(dpc_register.dpc_tmem);
     dummy_gfx_info.VI_STATUS_REG = &(vi_register.vi_status);
     dummy_gfx_info.VI_ORIGIN_REG = &(vi_register.vi_origin);
     dummy_gfx_info.VI_WIDTH_REG = &(vi_register.vi_width);
     dummy_gfx_info.VI_INTR_REG = &(vi_register.vi_v_intr);
     dummy_gfx_info.VI_V_CURRENT_LINE_REG = &(vi_register.vi_current);
     dummy_gfx_info.VI_TIMING_REG = &(vi_register.vi_burst);
     dummy_gfx_info.VI_V_SYNC_REG = &(vi_register.vi_v_sync);
     dummy_gfx_info.VI_H_SYNC_REG = &(vi_register.vi_h_sync);
     dummy_gfx_info.VI_LEAP_REG = &(vi_register.vi_leap);
     dummy_gfx_info.VI_H_START_REG = &(vi_register.vi_h_start);
     dummy_gfx_info.VI_V_START_REG = &(vi_register.vi_v_start);
     dummy_gfx_info.VI_V_BURST_REG = &(vi_register.vi_v_burst);
     dummy_gfx_info.VI_X_SCALE_REG = &(vi_register.vi_x_scale);
     dummy_gfx_info.VI_Y_SCALE_REG = &(vi_register.vi_y_scale);
     dummy_gfx_info.CheckInterrupts = sucre;
     
     /////// AUDIO /////////////////////////
     dummy_audio_info.hwnd = mainHWND;
     dummy_audio_info.hinst = app_hInstance;
     dummy_audio_info.MemoryBswaped = TRUE;
     dummy_audio_info.HEADER = (BYTE*)DummyHeader ;
     dummy_audio_info.RDRAM = (BYTE*)rdram;
     dummy_audio_info.DMEM = (BYTE*)SP_DMEM;
     dummy_audio_info.IMEM = (BYTE*)SP_IMEM;
     dummy_audio_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
     dummy_audio_info.AI_DRAM_ADDR_REG = &(ai_register.ai_dram_addr);
     dummy_audio_info.AI_LEN_REG = &(ai_register.ai_len); 
     dummy_audio_info.AI_CONTROL_REG = &(ai_register.ai_control);
     dummy_audio_info.AI_STATUS_REG = &(ai_register.ai_status);
     dummy_audio_info.AI_DACRATE_REG = &(ai_register.ai_dacrate);
     dummy_audio_info.AI_BITRATE_REG = &(ai_register.ai_bitrate);
     dummy_audio_info.CheckInterrupts = sucre; 
     
     ///// CONTROLS ///////////////////////////
     dummy_control_info.hMainWindow = mainHWND;
     dummy_control_info.hinst = app_hInstance;
     dummy_control_info.MemoryBswaped = TRUE;
     dummy_control_info.HEADER = (BYTE*)DummyHeader;
     dummy_control_info.Controls = Controls;
     for (i=0; i<4; i++)
     {
	    Controls[i].Present = FALSE;
	    Controls[i].RawData = FALSE;
	    Controls[i].Plugin = PLUGIN_NONE;
     } 
     
     //////// RSP /////////////////////////////
    dummy_rsp_info.MemoryBswaped = TRUE;
	dummy_rsp_info.RDRAM = (BYTE*)rdram;
	dummy_rsp_info.DMEM = (BYTE*)SP_DMEM;
	dummy_rsp_info.IMEM = (BYTE*)SP_IMEM;
	dummy_rsp_info.MI_INTR_REG = &MI_register.mi_intr_reg;
	dummy_rsp_info.SP_MEM_ADDR_REG = &sp_register.sp_mem_addr_reg;
	dummy_rsp_info.SP_DRAM_ADDR_REG = &sp_register.sp_dram_addr_reg;
	dummy_rsp_info.SP_RD_LEN_REG = &sp_register.sp_rd_len_reg;
	dummy_rsp_info.SP_WR_LEN_REG = &sp_register.sp_wr_len_reg;
	dummy_rsp_info.SP_STATUS_REG = &sp_register.sp_status_reg;
	dummy_rsp_info.SP_DMA_FULL_REG = &sp_register.sp_dma_full_reg;
	dummy_rsp_info.SP_DMA_BUSY_REG = &sp_register.sp_dma_busy_reg;
	dummy_rsp_info.SP_PC_REG = &rsp_register.rsp_pc;
	dummy_rsp_info.SP_SEMAPHORE_REG = &sp_register.sp_semaphore_reg;
	dummy_rsp_info.DPC_START_REG = &dpc_register.dpc_start;
	dummy_rsp_info.DPC_END_REG = &dpc_register.dpc_end;
	dummy_rsp_info.DPC_CURRENT_REG = &dpc_register.dpc_current;
	dummy_rsp_info.DPC_STATUS_REG = &dpc_register.dpc_status;
	dummy_rsp_info.DPC_CLOCK_REG = &dpc_register.dpc_clock;
	dummy_rsp_info.DPC_BUFBUSY_REG = &dpc_register.dpc_bufbusy;
	dummy_rsp_info.DPC_PIPEBUSY_REG = &dpc_register.dpc_pipebusy;
	dummy_rsp_info.DPC_TMEM_REG = &dpc_register.dpc_tmem;
	dummy_rsp_info.CheckInterrupts = sucre;
	dummy_rsp_info.ProcessDlistList = processDList;
	dummy_rsp_info.ProcessAlistList = processAList;
	dummy_rsp_info.ProcessRdpList = processRDPList;
	dummy_rsp_info.ShowCFB = showCFB;
	
}

void SaveGlobalPlugins(BOOL method)
{
    static char gfx_temp[100],input_temp[100],sound_temp[100],rsp_temp[100];
    
    if (method)  //Saving
    {
        sprintf(gfx_temp,gfx_name);
        sprintf(input_temp,input_name);
        sprintf(sound_temp,sound_name);
        sprintf(rsp_temp,rsp_name);
    }
    else         //Loading
    {
        sprintf( gfx_name, gfx_temp);
        sprintf( input_name, input_temp);
        sprintf( sound_name, sound_temp);
        sprintf( rsp_name, rsp_temp);
    }
}

static int LastState = ID_CURRENTSAVE_1;
void SelectState(HWND hWnd, int StateID)
{
    HMENU hMenu = GetMenu(hWnd);
	CheckMenuItem( hMenu, LastState, MF_BYCOMMAND | MFS_UNCHECKED );
	CheckMenuItem( hMenu, StateID, MF_BYCOMMAND | MFS_CHECKED );
	currentSaveState = StateID - (ID_CURRENTSAVE_1-1) ;
	LastState = StateID ;
	savestates_select_slot(currentSaveState) ;
}

void SaveTheState(HWND hWnd, int StateID)
{
	SelectState(hWnd, (StateID - ID_SAVE_1) + ID_CURRENTSAVE_1);

//	SendMessage(hWnd, WM_COMMAND, STATE_SAVE, 0);
    if(!emu_paused)
		savestates_job = SAVESTATE;
    else if(emu_launched)
		savestates_save();
}

void LoadTheState(HWND hWnd, int StateID)
{
	SelectState(hWnd, (StateID - ID_LOAD_1) + ID_CURRENTSAVE_1);
	
//	SendMessage(hWnd, WM_COMMAND, STATE_RESTORE, 0);
    if(!emu_paused)
		savestates_job = LOADSTATE;
    else if(emu_launched)
		savestates_load();
}



//--------------------- plugin storage type ----------------
typedef struct _plugins plugins;
struct _plugins
{
    char *file_name;
    char *plugin_name;
    HMODULE handle;
    int type;
    plugins *next;
};
static plugins *liste_plugins = NULL, *current;

void insert_plugin(plugins *p, char *file_name,
		   char *plugin_name, void *handle, int type,int num)
{
    if (p->next)
        insert_plugin(p->next, file_name, plugin_name, handle, type, 
                               (p->type == type) ? num+1 : num);
    else
    {
        p->next = (plugins*)malloc(sizeof(plugins));
        p->next->type = type;
        p->next->handle = (HMODULE)handle;
        p->next->file_name = (char*)malloc(strlen(file_name)+1);
        strcpy(p->next->file_name, file_name);
        p->next->plugin_name = (char*)malloc(strlen(plugin_name)+7);
        sprintf(p->next->plugin_name, "%s", plugin_name);
        p->next->next=NULL;
    }
}

void rewind_plugin()
{
   current = liste_plugins;
}

char *next_plugin()
{
   if (!current->next) return NULL;
   current = current->next;
   return current->plugin_name;
}

int get_plugin_type()
{
   if (!current->next) return -1;
   return current->next->type;
}

char *getPluginNameInner( plugins *p, char *pluginpath, int plugintype)
{
    if (!p->next) return NULL;
    if ((plugintype==p->next->type) && (strcasecmp(p->next->file_name, pluginpath)==0) )
         return p->next->plugin_name;
    else  
         return getPluginNameInner(p->next, pluginpath, plugintype);
}

char *getPluginName(char *pluginpath,int plugintype)
{
    return getPluginNameInner( liste_plugins, pluginpath, plugintype);
    
}

HMODULE get_handle(plugins *p, char *name)
{
   if (!p->next) return NULL;
   
   while(p->next->plugin_name[strlen(p->next->plugin_name)-1] == ' ') 
     p->next->plugin_name[strlen(p->next->plugin_name)-1] = '\0';
   
   if (!strcmp(p->next->plugin_name, name))
		 return p->next->handle;
   else
		 return (HMODULE)get_handle(p->next, name);
}

char* getExtension(char *str)
{
    if (strlen(str) > 3) return str + strlen(str) - 3;
    else return NULL;
}



struct ProcAddress
{
	FARPROC _fp;
  ProcAddress(HMODULE module, LPCSTR name) : _fp(NULL)
	{
    _fp = ::GetProcAddress(module, name);
  }
  template<class T>
	operator T() const
	{
		return reinterpret_cast<T>(_fp);
	}
};

void search_plugins()
{
	// TODO: この辺りなにしてるかよく見てない
	liste_plugins = (plugins*)malloc(sizeof(plugins));
	liste_plugins->type = -1;
	liste_plugins->next = NULL;
	current = liste_plugins;

	String pluginDir;
	if(Config.DefaultPluginsDir)
	{
		pluginDir.assign(AppPath).append("\\plugin");
	}
	else
	{
		pluginDir.assign(Config.PluginsDir);
	}

	WIN32_FIND_DATA entry;
	HANDLE dirHandle = ::FindFirstFile((pluginDir + "\\*.dll").c_str(), &entry);
	if(dirHandle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{ //pluginディレクトリを読んで、プラグインリストを生成
		if(String(::getExtension(entry.cFileName)) == "dll")
		{
			String pluginPath;
			pluginPath.assign(pluginDir)
				.append("\\").append(entry.cFileName);
			MUPEN64RR_DEBUGINFO(pluginPath);
				
			HMODULE pluginHandle = LoadLibrary(pluginPath.c_str());
			if(pluginHandle != NULL
				&& (getDllInfo = ProcAddress(pluginHandle, "GetDllInfo")) != NULL)
			{
				PLUGIN_INFO pluginInfo;
				getDllInfo(&pluginInfo);
				while(pluginInfo.Name[strlen(pluginInfo.Name)-1] == ' ')
				{
					pluginInfo.Name[strlen(pluginInfo.Name)-1] = '\0';
				}
				insert_plugin(liste_plugins, entry.cFileName, pluginInfo.Name,
					pluginHandle, pluginInfo.Type, 0);
			}
		}
	} while(!!FindNextFile(dirHandle, &entry));

}

void exec_config(char *name)
{
   HMODULE handle;
   PLUGIN_INFO PluginInfo;
   handle = (HMODULE)get_handle(liste_plugins, name);
   int i ;
   BOOL wasPaused = emu_paused;
   if (emu_launched&&!emu_paused) {
                  pauseEmu(FALSE);
     };
     
   if (handle) {
      
      getDllInfo = (void(__cdecl*)(PLUGIN_INFO *PluginInfo))GetProcAddress(handle, "GetDllInfo");
      
      getDllInfo(&PluginInfo);
      switch (PluginInfo.Type)
                {
                    case PLUGIN_TYPE_AUDIO:
                       if (!emu_launched) {
                          initiateAudio = (BOOL (__cdecl *)(AUDIO_INFO))GetProcAddress( handle, "InitiateAudio" );
                          if (!initiateAudio(dummy_audio_info)) {
			                    ShowMessage("Failed to initialize audio plugin.");
		                  }
		               }
		               
		               dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");              
                       if (dllConfig) dllConfig(hwnd_plug); 
                       if (!emu_launched) {
                          closeDLL_audio = (void(__cdecl*)())GetProcAddress( handle, "CloseDLL");
		                  if (closeDLL_audio) closeDLL_audio();
		               }
                    break;
                    case PLUGIN_TYPE_GFX:   
                        if (!emu_launched) {
                             initiateGFX = (BOOL(__cdecl*)(GFX_INFO Gfx_Info))GetProcAddress(handle, "InitiateGFX");
                             if (!initiateGFX(dummy_gfx_info)) {
                                 ShowMessage("Failed to initiate gfx plugin.") ;                   
                             } 
                        }
                        
                        dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");              
                        if (dllConfig) dllConfig(hwnd_plug); 
                        if (!emu_launched) {
                             closeDLL_gfx = (void(__cdecl*)())GetProcAddress( handle, "CloseDLL");
                             if (closeDLL_gfx) closeDLL_gfx();
                        }       
                    break;
                    case PLUGIN_TYPE_CONTROLLER: 
                        if (!emu_launched) {  
                           if (PluginInfo.Version == 0x0101)
                               {
                                 initiateControllers = (void(__cdecl*)(CONTROL_INFO ControlInfo))GetProcAddress(handle, "InitiateControllers");
                                 initiateControllers(dummy_control_info);
                               } 
                               else
                               {
                                  old_initiateControllers = (void(__cdecl*)(HWND hMainWindow, CONTROL Controls[4]))GetProcAddress(handle, "InitiateControllers");
                                  old_initiateControllers(mainHWND, Controls);
                               } 
                             }   
                             
                             dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");              
                             if (dllConfig) dllConfig(hwnd_plug); 
                             
                             if (!emu_launched) {   
                                   closeDLL_input = (void(__cdecl*)())GetProcAddress( handle, "CloseDLL");
                                   if (closeDLL_input) closeDLL_input(); 
                             }                  
                    break;
                    case PLUGIN_TYPE_RSP:
                        if (!emu_launched) {
                             initiateRSP = (void (__cdecl *)( RSP_INFO, DWORD *))GetProcAddress( handle, "InitiateRSP" ); 
                             initiateRSP(dummy_rsp_info,(DWORD*)&i);
                        }
                        
                        dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");              
                        if (dllConfig) dllConfig(hwnd_plug); 
                        if (!emu_launched) {
                             closeDLL_RSP = (void(__cdecl*)())GetProcAddress( handle, "CloseDLL");
                             if (closeDLL_RSP) closeDLL_RSP();
                        }  
                    break;          
                    default:
                             dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");              
                             if (dllConfig) dllConfig(hwnd_plug); 
                    break;        
                } 
                
      
   }
   
    if (emu_launched&&emu_paused&&!wasPaused) {
                  resumeEmu(FALSE);
     }
}

void exec_test(char *name)
{
   HMODULE handle;
   
   handle = (HMODULE)get_handle(liste_plugins, name);
   if (handle)
      {
         dllTest = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllTest");
         if (dllTest) dllTest(hwnd_plug);
      }   
}

void exec_about(char *name)
{
   HMODULE handle;
   
   handle = (HMODULE)get_handle(liste_plugins, name);
   if (handle) 
      {
         dllAbout = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllAbout");
         if (dllAbout) dllAbout(hwnd_plug);
      }
}
int check_plugins()
{
   void *handle_gfx, *handle_input, *handle_sound,*handle_rsp ;

   handle_gfx = get_handle(liste_plugins, gfx_name);
   if (!handle_gfx) {
         ShowMessage("Choose graphics plugin in Configuration Dialog.");
		 return (0);
   }
   
   handle_input = get_handle(liste_plugins, input_name);
   if (!handle_input) {
         ShowMessage("Choose input plugin in Configuration Dialog.");
		 return (0);
   }
   
   handle_sound = get_handle(liste_plugins, sound_name);
   if (!handle_sound) {
         ShowMessage("Choose audio plugin in Configuration Dialog.");
		 return (0);
   }
   
   handle_rsp = get_handle(liste_plugins, rsp_name);
   if (!handle_rsp) {
         ShowMessage("Choose RSP plugin in Configuration Dialog.");
		 return (0);
   }
   return 1;
}

int load_gfx(HMODULE handle_gfx)
{
   if (handle_gfx)
   {
   changeWindow = (void(__cdecl*)())GetProcAddress(handle_gfx, "ChangeWindow");
   closeDLL_gfx = (void(__cdecl*)())GetProcAddress(handle_gfx, "CloseDLL");
   dllAbout = (void(__cdecl*)(HWND hParent))GetProcAddress(handle_gfx, "DllAbout");
   dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle_gfx, "DllConfig");
   dllTest = (void(__cdecl*)(HWND hParent))GetProcAddress(handle_gfx, "DllTest");
   initiateGFX = (BOOL(__cdecl*)(GFX_INFO Gfx_Info))GetProcAddress(handle_gfx, "InitiateGFX");
   processDList = (void(__cdecl*)())GetProcAddress(handle_gfx, "ProcessDList");
   processRDPList = (void(__cdecl*)())GetProcAddress(handle_gfx, "ProcessRDPList");
   romClosed_gfx = (void(__cdecl*)())GetProcAddress(handle_gfx, "RomClosed");
   romOpen_gfx = (void(__cdecl*)())GetProcAddress(handle_gfx, "RomOpen");
   showCFB = (void(__cdecl*)())GetProcAddress(handle_gfx, "ShowCFB");
   updateScreen = (void(__cdecl*)())GetProcAddress(handle_gfx, "UpdateScreen");
   viStatusChanged = (void(__cdecl*)())GetProcAddress(handle_gfx, "ViStatusChanged");
   viWidthChanged = (void(__cdecl*)())GetProcAddress(handle_gfx, "ViWidthChanged");
   moveScreen = (void(__cdecl*)(int, int))GetProcAddress(handle_gfx, "MoveScreen");
   CaptureScreen = (void(__cdecl*)(char *Directory))GetProcAddress(handle_gfx, "CaptureScreen");
   readScreen = (void(__cdecl*)(void **dest, long *width, long *height))GetProcAddress(handle_gfx, "ReadScreen");
   if(readScreen == NULL) externalReadScreen = 0;
   else externalReadScreen = 1;
   
   fBRead = (void(__cdecl*)(DWORD))GetProcAddress(handle_gfx, "FBRead");
   fBWrite = (void(__cdecl*)(DWORD, DWORD))GetProcAddress(handle_gfx, "FBWrite");
   fBGetFrameBufferInfo = (void(__cdecl*)(void*))GetProcAddress(handle_gfx, "FBGetFrameBufferInfo");
   
    if (changeWindow == NULL) changeWindow = dummy_void;
    if (closeDLL_gfx == NULL) closeDLL_gfx = dummy_void;
	if (initiateGFX == NULL) initiateGFX = dummy_initiateGFX;
	if (processDList == NULL) processDList = dummy_void;
	if (processRDPList == NULL) processRDPList = dummy_void;
	if (romClosed_gfx == NULL) romClosed_gfx = dummy_void;
	if (romOpen_gfx == NULL) romOpen_gfx = dummy_void;
	if (showCFB == NULL) showCFB = dummy_void;
	if (updateScreen == NULL) updateScreen = dummy_void;
	if (viStatusChanged == NULL) viStatusChanged = dummy_void;
	if (viWidthChanged == NULL) viWidthChanged = dummy_void;
    if (CaptureScreen == NULL) CaptureScreen = (void(__cdecl*)(char*))dummy_void;
    
   gfx_info.hWnd = mainHWND;
   if (Config.GuiStatusbar) {
      gfx_info.hStatusBar = hStatus ;
   }
   else {
      gfx_info.hStatusBar = NULL ;
   }
   gfx_info.MemoryBswaped = TRUE;
   gfx_info.HEADER = rom;
   gfx_info.RDRAM = (BYTE*)rdram;
   gfx_info.DMEM = (BYTE*)SP_DMEM;
   gfx_info.IMEM = (BYTE*)SP_IMEM;
   gfx_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
   gfx_info.DPC_START_REG = &(dpc_register.dpc_start);
   gfx_info.DPC_END_REG = &(dpc_register.dpc_end);
   gfx_info.DPC_CURRENT_REG = &(dpc_register.dpc_current);
   gfx_info.DPC_STATUS_REG = &(dpc_register.dpc_status);
   gfx_info.DPC_CLOCK_REG = &(dpc_register.dpc_clock);
   gfx_info.DPC_BUFBUSY_REG = &(dpc_register.dpc_bufbusy);
   gfx_info.DPC_PIPEBUSY_REG = &(dpc_register.dpc_pipebusy);
   gfx_info.DPC_TMEM_REG = &(dpc_register.dpc_tmem);
   gfx_info.VI_STATUS_REG = &(vi_register.vi_status);
   gfx_info.VI_ORIGIN_REG = &(vi_register.vi_origin);
   gfx_info.VI_WIDTH_REG = &(vi_register.vi_width);
   gfx_info.VI_INTR_REG = &(vi_register.vi_v_intr);
   gfx_info.VI_V_CURRENT_LINE_REG = &(vi_register.vi_current);
   gfx_info.VI_TIMING_REG = &(vi_register.vi_burst);
   gfx_info.VI_V_SYNC_REG = &(vi_register.vi_v_sync);
   gfx_info.VI_H_SYNC_REG = &(vi_register.vi_h_sync);
   gfx_info.VI_LEAP_REG = &(vi_register.vi_leap);
   gfx_info.VI_H_START_REG = &(vi_register.vi_h_start);
   gfx_info.VI_V_START_REG = &(vi_register.vi_v_start);
   gfx_info.VI_V_BURST_REG = &(vi_register.vi_v_burst);
   gfx_info.VI_X_SCALE_REG = &(vi_register.vi_x_scale);
   gfx_info.VI_Y_SCALE_REG = &(vi_register.vi_y_scale);
   gfx_info.CheckInterrupts = sucre;
   initiateGFX(gfx_info);
   }
   else
   {
    changeWindow = dummy_void;
	closeDLL_gfx = dummy_void;
	initiateGFX = dummy_initiateGFX;
	processDList = dummy_void;
	processRDPList = dummy_void;
	romClosed_gfx = dummy_void;
	romOpen_gfx = dummy_void;
	showCFB = dummy_void;
	updateScreen = dummy_void;
	viStatusChanged = dummy_void;
	viWidthChanged = dummy_void;
   }
   return 0;
}
int load_input(HMODULE handle_input)
{
   int i ;
   PLUGIN_INFO PluginInfo;
   if (handle_input)
   {
   getDllInfo = (void(__cdecl*)(PLUGIN_INFO *PluginInfo))GetProcAddress(handle_input, "GetDllInfo");
   getDllInfo(&PluginInfo);
   
   closeDLL_input = (void(__cdecl*)())GetProcAddress(handle_input, "CloseDLL");
   controllerCommand = (void(__cdecl*)(int Control, BYTE * Command))GetProcAddress(handle_input, "ControllerCommand");
   getKeys = (void(__cdecl*)(int Control, BUTTONS *Keys))GetProcAddress(handle_input, "GetKeys");
   if (PluginInfo.Version == 0x0101)
       initiateControllers = (void(__cdecl*)(CONTROL_INFO ControlInfo))GetProcAddress(handle_input, "InitiateControllers");
   else
       old_initiateControllers = (void(__cdecl*)(HWND hMainWindow, CONTROL Controls[4]))GetProcAddress(handle_input, "InitiateControllers");
   readController = (void(__cdecl*)(int Control, BYTE *Command))GetProcAddress(handle_input, "ReadController");
   romClosed_input = (void(__cdecl*)())GetProcAddress(handle_input, "RomClosed");
   romOpen_input = (void(__cdecl*)())GetProcAddress(handle_input, "RomOpen");
   keyDown = (void(__cdecl*)(WPARAM wParam, LPARAM lParam))GetProcAddress(handle_input, "WM_KeyDown");
   keyUp = (void(__cdecl*)(WPARAM wParam, LPARAM lParam))GetProcAddress(handle_input, "WM_KeyUp");
   
   if (closeDLL_input == NULL) closeDLL_input = dummy_void;
	if (controllerCommand == NULL) controllerCommand = dummy_controllerCommand;
	if (getKeys == NULL) getKeys = dummy_getKeys;
	if (initiateControllers == NULL) initiateControllers = dummy_initiateControllers;
	if (readController == NULL) readController = dummy_readController;
	if (romClosed_input == NULL) romClosed_input = dummy_void;
	if (romOpen_input == NULL) romOpen_input = dummy_void;
	if (keyDown == NULL) keyDown = dummy_keyDown;
	if (keyUp == NULL) keyUp = dummy_keyUp;
   
   control_info.hMainWindow = mainHWND;
   control_info.hinst = app_hInstance;
   control_info.MemoryBswaped = TRUE;
   control_info.HEADER = rom;
   control_info.Controls = Controls;
   for (i=0; i<4; i++)
     {
	Controls[i].Present = FALSE;
	Controls[i].RawData = FALSE;
	Controls[i].Plugin = PLUGIN_NONE;
     }
   if (PluginInfo.Version == 0x0101)
      {
        initiateControllers(control_info);
      } 
   else
      {
        old_initiateControllers(mainHWND, Controls);
      } 
     InputPluginVersion = PluginInfo.Version;
  }
  else
  {
    closeDLL_input = dummy_void;
	controllerCommand = dummy_controllerCommand;
	getKeys = dummy_getKeys;
	initiateControllers = dummy_initiateControllers;
	readController = dummy_readController;
	romClosed_input = dummy_void;
	romOpen_input = dummy_void;
	keyDown = dummy_keyDown;
	keyUp = dummy_keyUp;
  }
  return 0;
}


int load_sound(HMODULE handle_sound )
{
    if (handle_sound)
     {
    closeDLL_audio = (void (__cdecl *)(void))GetProcAddress(handle_sound, "CloseDLL" );
	aiDacrateChanged = (void (__cdecl *)(int))GetProcAddress(handle_sound, "AiDacrateChanged" );
	aiLenChanged = (void (__cdecl *)(void))GetProcAddress(handle_sound, "AiLenChanged" );
	aiReadLength = (DWORD (__cdecl *)(void))GetProcAddress(handle_sound, "AiReadLength" );
	initiateAudio = (BOOL (__cdecl *)(AUDIO_INFO))GetProcAddress(handle_sound, "InitiateAudio" );
	romClosed_audio = (void (__cdecl *)(void))GetProcAddress(handle_sound, "RomClosed" );
	romOpen_audio = (void (__cdecl *)(void))GetProcAddress(handle_sound, "RomOpen" );
	processAList = (void (__cdecl *)(void))GetProcAddress(handle_sound, "ProcessAList" );	
	aiUpdate = (void (__cdecl *)(BOOL))GetProcAddress(handle_sound, "AiUpdate" );
	
	if (aiDacrateChanged == NULL) aiDacrateChanged = dummy_aiDacrateChanged;
	if (aiLenChanged == NULL) aiLenChanged = dummy_void;
	if (aiReadLength == NULL) aiReadLength = dummy_aiReadLength;
	//if (aiUpdate == NULL) aiUpdate = dummy_aiUpdate;
	if (closeDLL_audio == NULL) closeDLL_audio = dummy_void;
	if (initiateAudio == NULL) initiateAudio = dummy_initiateAudio;
	if (processAList == NULL) processAList = dummy_void;
	if (romClosed_audio == NULL) romClosed_audio = dummy_void;
	if (romOpen_audio == NULL) romOpen_audio = dummy_void;
	
    audio_info.hwnd = mainHWND;
    audio_info.hinst = app_hInstance;
    audio_info.MemoryBswaped = TRUE;
    audio_info.HEADER = rom;
        
    audio_info.RDRAM = (BYTE*)rdram;
    audio_info.DMEM = (BYTE*)SP_DMEM;
    audio_info.IMEM = (BYTE*)SP_IMEM;
    
    audio_info.MI_INTR_REG = &dummy;//&(MI_register.mi_intr_reg);
    
    audio_info.AI_DRAM_ADDR_REG = &(ai_register.ai_dram_addr);
    audio_info.AI_LEN_REG = &(ai_register.ai_len); 
    audio_info.AI_CONTROL_REG = &(ai_register.ai_control);
    audio_info.AI_STATUS_REG = &dummy;//&(ai_register.ai_status);
    audio_info.AI_DACRATE_REG = &(ai_register.ai_dacrate);
    audio_info.AI_BITRATE_REG = &(ai_register.ai_bitrate);
    
    audio_info.CheckInterrupts = sucre;
    initiateAudio(audio_info);
    }
    else
     {
	aiDacrateChanged = dummy_aiDacrateChanged;
	aiLenChanged = dummy_void;
	aiReadLength = dummy_aiReadLength;
	//aiUpdate = dummy_aiUpdate;
	closeDLL_audio = dummy_void;
	initiateAudio = dummy_initiateAudio;
	processAList = dummy_void;
	romClosed_audio = dummy_void;
	romOpen_audio = dummy_void;
     }
     return 0;
}

int load_rsp(HMODULE handle_RSP)
{
    int i = 4 ;
    if (handle_RSP)
     {
    closeDLL_RSP = (void (__cdecl *)(void))GetProcAddress(handle_RSP, "CloseDLL" );	 
	doRspCycles = (DWORD (__cdecl *)(DWORD))GetProcAddress(handle_RSP, "DoRspCycles" ); 
	initiateRSP = (void (__cdecl *)( RSP_INFO, DWORD *))GetProcAddress(handle_RSP, "InitiateRSP" ); 
	romClosed_RSP = (void (__cdecl *)(void))GetProcAddress(handle_RSP, "RomClosed" ); 
	
	if (closeDLL_RSP == NULL) closeDLL_RSP = dummy_void;
	if (doRspCycles == NULL) doRspCycles = dummy_doRspCycles;
	if (initiateRSP == NULL) initiateRSP = dummy_initiateRSP;
	if (romClosed_RSP == NULL) romClosed_RSP = dummy_void;
	
	rsp_info.MemoryBswaped = TRUE;
	rsp_info.RDRAM = (BYTE*)rdram;
	rsp_info.DMEM = (BYTE*)SP_DMEM;
	rsp_info.IMEM = (BYTE*)SP_IMEM;
	rsp_info.MI_INTR_REG = &MI_register.mi_intr_reg;
	rsp_info.SP_MEM_ADDR_REG = &sp_register.sp_mem_addr_reg;
	rsp_info.SP_DRAM_ADDR_REG = &sp_register.sp_dram_addr_reg;
	rsp_info.SP_RD_LEN_REG = &sp_register.sp_rd_len_reg;
	rsp_info.SP_WR_LEN_REG = &sp_register.sp_wr_len_reg;
	rsp_info.SP_STATUS_REG = &sp_register.sp_status_reg;
	rsp_info.SP_DMA_FULL_REG = &sp_register.sp_dma_full_reg;
	rsp_info.SP_DMA_BUSY_REG = &sp_register.sp_dma_busy_reg;
	rsp_info.SP_PC_REG = &rsp_register.rsp_pc;
	rsp_info.SP_SEMAPHORE_REG = &sp_register.sp_semaphore_reg;
	rsp_info.DPC_START_REG = &dpc_register.dpc_start;
	rsp_info.DPC_END_REG = &dpc_register.dpc_end;
	rsp_info.DPC_CURRENT_REG = &dpc_register.dpc_current;
	rsp_info.DPC_STATUS_REG = &dpc_register.dpc_status;
	rsp_info.DPC_CLOCK_REG = &dpc_register.dpc_clock;
	rsp_info.DPC_BUFBUSY_REG = &dpc_register.dpc_bufbusy;
	rsp_info.DPC_PIPEBUSY_REG = &dpc_register.dpc_pipebusy;
	rsp_info.DPC_TMEM_REG = &dpc_register.dpc_tmem;
	rsp_info.CheckInterrupts = sucre;
	rsp_info.ProcessDlistList = processDList;
	rsp_info.ProcessAlistList = processAList;
	rsp_info.ProcessRdpList = processRDPList;
	rsp_info.ShowCFB = showCFB;
	initiateRSP(rsp_info,(DWORD*)&i);
     }
   else
     {
	closeDLL_RSP = dummy_void;
	doRspCycles = dummy_doRspCycles;
	initiateRSP = dummy_initiateRSP;
	romClosed_RSP = dummy_void;
     }
     return 0;
}

int load_plugins()
{
   HMODULE handle_gfx, handle_input, handle_sound, handle_rsp ;
      
   DEFAULT_ROM_SETTINGS TempRomSettings;
   
   TempRomSettings = GetDefaultRomSettings((char*)ROM_HEADER->nom) ;
   if (!Config.OverwritePluginSettings) 
   {
       handle_gfx = get_handle(liste_plugins, TempRomSettings.GfxPluginName);
       if (handle_gfx==NULL) {handle_gfx = get_handle(liste_plugins, gfx_name);}
       else {sprintf(gfx_name,TempRomSettings.GfxPluginName);}
   
       handle_input = get_handle(liste_plugins, TempRomSettings.InputPluginName); 
       if (handle_input==NULL) handle_input = get_handle(liste_plugins, input_name);
       else {sprintf(input_name,TempRomSettings.InputPluginName);}
   
       handle_sound = get_handle(liste_plugins, TempRomSettings.SoundPluginName); 
       if (handle_sound==NULL) handle_sound = get_handle(liste_plugins, sound_name);
       else {sprintf(sound_name,TempRomSettings.SoundPluginName);}
   
       handle_rsp = get_handle(liste_plugins, TempRomSettings.RspPluginName); 
       if (handle_rsp==NULL) handle_rsp = get_handle(liste_plugins, rsp_name);
       else {sprintf(rsp_name,TempRomSettings.RspPluginName);}
   }
   else 
   {
       handle_gfx = get_handle(liste_plugins, gfx_name);
       handle_input = get_handle(liste_plugins, input_name);
       handle_sound = get_handle(liste_plugins, sound_name);
       handle_rsp = get_handle(liste_plugins, rsp_name);
   }
   ThreadFuncState = TFS_LOADGFX;
   ShowInfo("Loading gfx -  %s",gfx_name);
   load_gfx(handle_gfx);
   ThreadFuncState = TFS_LOADINPUT;
   ShowInfo("Loading input -  %s",input_name);
   load_input(handle_input);
   ThreadFuncState = TFS_LOADAUDIO;
   ShowInfo("Loading sound - %s",sound_name);
   load_sound(handle_sound );
   ThreadFuncState = TFS_LOADRSP;
   ShowInfo("Loading RSP - %s",rsp_name);
   load_rsp(handle_rsp);
          
   return (1);
}


void WaitEmuThread()
{
    DWORD ExitCode;
    int count;
    
    for (count = 0; count < 400; count ++ )
    {
        SleepEx(50,TRUE);
        GetExitCodeThread( EmuThreadHandle, &ExitCode);
		if (ExitCode != STILL_ACTIVE) {
			EmuThreadHandle = NULL;
//			count = 100;
			break;
		}
    }
    if ( EmuThreadHandle != NULL || ExitCode != 0) {  

			if( EmuThreadHandle != NULL) {
	            ShowError("Abnormal emu thread termination!");
	            TerminateThread( EmuThreadHandle,0); 
	            EmuThreadHandle = NULL; 
			}

			char str [256];
			sprintf(str, "There was a problem with %s.", ThreadFuncStateDescription[ThreadFuncState]);
            ShowError(str);
			sprintf(str, "%s\nYou should quit and re-open the emulator before doing anything else,\nor you may encounter serious errors.", str);
            MessageBox(NULL, str, "Warning", MB_OK);
    }
    emu_launched = 0;
    emu_paused = 1;   
}


void resumeEmu(BOOL quiet)
{
	BOOL wasPaused = emu_paused;
	if (emu_launched) {
		if(!quiet)
			ShowInfo("Resume emulation");
		emu_paused = 0 ;
//		ResumeThread(EmuThreadHandle);
		ResumeThread(SoundThreadHandle);
		if(!quiet)
			SetStatusTranslatedString(hStatus,0,"Emulation started");
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
	}
	else
	{
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
	}

	if(emu_paused != wasPaused && !quiet)
		CheckMenuItem( GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | (emu_paused ? MFS_CHECKED : MFS_UNCHECKED));
}

//void autoPauseEmu(flag)
//{
//    if (flag) {  //Auto Pause emulator
//        AutoPause = 1;
//        if (!emu_paused) {
//                pauseEmu(FALSE);
//        }
//    }
//    else {
//        if (AutoPause&&emu_paused) {
//                resumeEmu(FALSE);
//        }
//        AutoPause = 0;
//    }    
//}


void pauseEmu(BOOL quiet)
{
	BOOL wasPaused = emu_paused;
	if (emu_launched) {
		VCR_updateFrameCounter();
		if(!quiet)
			ShowInfo("Pause emulation");
		emu_paused = 1 ;
//		SuspendThread(EmuThreadHandle);
		if(!quiet) // HACK (not a typo) seems to help avoid a race condition that permanently disables sound when doing frame advance
			SuspendThread(SoundThreadHandle);
		if(!quiet)
			SetStatusTranslatedString(hStatus,0,"Emulation paused");
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 1);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_STOP, 0);
	}
	else
	{
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
	}     

	if(emu_paused != wasPaused && !quiet && !MenuPaused)
		CheckMenuItem( GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | (emu_paused ? MFS_CHECKED : MFS_UNCHECKED));
}

BOOL StartRom(char *fullRomPath)
{
     LONG winstyle;
  	
     if (emu_launched) {
			/*closeRom();*/
       really_restart_mode = TRUE;
       strcpy(LastSelectedRom, fullRomPath);
			CreateThread(NULL, 0, closeRom, NULL, 0, &Id);
		}
//     while (emu_launched) {
//		SleepEx(10, TRUE);
//	 }
     else{
                         //Makes window not resizable                         
                         //ShowWindow(mainHWND, FALSE);
                         winstyle = GetWindowLong( mainHWND, GWL_STYLE );
                         winstyle = winstyle & ~WS_THICKFRAME;
                         SetWindowLong(mainHWND, GWL_STYLE, winstyle );
                         SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED );  //Set on top
                         //ShowWindow(mainHWND, TRUE);
                        
                         if (!restart_mode) {
                             if (!check_plugins()) 
                             {
                                return TRUE;                  
                             }
                             
                             SetStatusMode( 1 );
                                                                                     
                             SetStatusTranslatedString(hStatus,0,"Loading ROM...");
                             SendMessage( hStatus, SB_SETTEXT, 2, (LPARAM)fullRomPath );

                             if (rom_read(fullRomPath))
                             {
                                emu_launched = 0;
                                saveMD5toCache(ROM_SETTINGS.MD5);
                                SetStatusMode( 0 );
                                SetStatusTranslatedString(hStatus,0,"Failed to open rom");
                                return TRUE;
                             }
                              
                             sprintf( LastSelectedRom, fullRomPath);
                             saveMD5toCache(ROM_SETTINGS.MD5);
                             AddToRecentList( mainHWND, fullRomPath) ; 
                             InitTimer();

                             EnableEmulationMenuItems(TRUE);
                             ShowRomBrowser(FALSE,FALSE);
                             SaveGlobalPlugins(TRUE);
                          }
                         ShowInfo("");
                         ShowWarning("Starting ROM: %s ",ROM_SETTINGS.goodname);
                         
                         SetStatusMode( 2 );
                         
                         ShowInfo("Creating emulation thread...");                          
                         EmuThreadHandle = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &Id);
                         sprintf(TempMessage, "%s - %s",MUPEN_VERSION, ROM_HEADER->nom);
                         SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
                         SetWindowText(mainHWND,TempMessage);
                         SetStatusTranslatedString(hStatus,0,"Emulation started");
                         //SendMessage( hStatus, SB_SETTEXT, 1, (LPARAM)"" ); 
                         return FALSE;
                     }
                     return 0;
}

void exit_emu2();
static int shut_window = 0;

//void closeRom()
static DWORD WINAPI closeRom(LPVOID lpParam) //lpParam - treated as bool, show romlist? 
{
    //ShowInfo("Close Rom"); this crashes everytime????
    
   LONG winstyle;                                //Used for setting new style to the window
//   int browserwidths[] = {400, -1};              //Rombrowser statusbar

   
   if (emu_launched)  {
     if (emu_paused)  {
           resumeEmu(FALSE);
         }
      
      if (recording/* && !continue_vcr_on_restart_mode*/)
      {
         Sleep(1000); // HACK - seems to help crash on closing ROM during capture...?
         if (VCR_stopCapture() < 0)
             MessageBox(NULL, "Couldn't stop capturing", "VCR", MB_OK);
         else {
             SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
             SetStatusTranslatedString(hStatus,0,"Stopped AVI capture");
             recording = FALSE;
         }
      }
      
      
      ShowInfo("Closing emulation thread...");
      stop_it();
      
      WaitEmuThread();
      
      EndGameKaillera();
      
    /*romClosed_input();
    ShowInfo("Emu thread: romClosed (input plugin)");
    romClosed_gfx();
    ShowInfo("Emu thread: romClosed (gfx plugin)");
    romClosed_audio();
    ShowInfo("Emu thread: romClosed (audio plugin)");
    romClosed_RSP();
    ShowInfo("Emu thread: romClosed (RSP plugin)");
    closeDLL_RSP();
    ShowInfo("Emu thread: RSP plugin closed");
    closeDLL_input();
    ShowInfo("Emu thread: input plugin closed");
    closeDLL_gfx();
    ShowInfo("Emu thread: gfx plugin closed");
    closeDLL_audio();
    ShowInfo("Emu thread: audio plugin closed");*/
      
       if (!restart_mode) {
         ShowInfo("Free rom and memory....");
         free(rom);
         rom = NULL;
         free(ROM_HEADER);
         ROM_HEADER = NULL;
         free_memory();  
         
         ShowInfo("Init emulation menu items....");
         EnableEmulationMenuItems(FALSE);
         SaveGlobalPlugins(FALSE);
         ShowRomBrowser(!really_restart_mode,!!lpParam);
         if (!lpParam)
         {
             SetWindowText(mainHWND,MUPEN_VERSION);
             SendMessage( hStatus, SB_SETTEXT, 1, (LPARAM)" " ); 
         }
      }
      ShowInfo("Rom closed.");
      
      if(shut_window)
      {
        //exit_emu2();
        //SleepEx(100,TRUE);
        SendMessage(mainHWND, WM_CLOSE, 0, 0);
        return 0;
      }
   }   
   
   SetStatusMode( 0 );
   SetStatusTranslatedString(hStatus,0,"Rom Closed");
   
   //Makes window resizable                         
   winstyle = GetWindowLong( mainHWND, GWL_STYLE );
   winstyle |= WS_THICKFRAME;
   SetWindowLong(mainHWND, GWL_STYLE, winstyle );
   SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);  //Set on top

   {
       if(really_restart_mode)
       {
       

        if(clear_sram_on_restart_mode)
        {
			VCR_clearAllSaveData();
        	clear_sram_on_restart_mode = FALSE;
		}

       really_restart_mode = FALSE;
       just_restarted_flag = TRUE;
       StartRom(LastSelectedRom);
      }
   }
   
   continue_vcr_on_restart_mode = FALSE;

   return 0;
}

void resetEmu() 
{
     // why is it so damned difficult to reset the game?
     // right now it's hacked to exit to the GUI then re-load the ROM,
     // but it should be possible to reset the game while it's still running
     // simply by clearing out some memory and maybe notifying the plugins...
     
    if (emu_launched ) {
                         ShowInfo("Restart Rom");
                         restart_mode = 0;
                         really_restart_mode = TRUE;
                         CreateThread(NULL, 0, closeRom, NULL, 0, &Id);
                  }
                       
}

void setDefaultPlugins() 
{
      ReadCfgString("Plugins","Graphics","",gfx_name);
      ReadCfgString("Plugins","Sound","",sound_name);
      ReadCfgString("Plugins","Input","",input_name);
      ReadCfgString("Plugins","RSP","",rsp_name);
      rewind_plugin();
}           

void ShowMessage(char *lpszMessage) 
{ 
   MessageBox(NULL, lpszMessage, "Info", MB_OK); 
} 

void CreateToolBarWindow(HWND hwnd)
{
    TBBUTTON tbButtons[ ] =
    {
        {0, IDLOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
        {0,  0,           TBSTATE_ENABLED, TBSTYLE_SEP,    {0,0}, 0, 0},
        {1, EMU_PLAY, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
		{2, EMU_PAUSE, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
        {3, EMU_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON  | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
        {0,  0,           TBSTATE_ENABLED, TBSTYLE_SEP,    {0,0}, 0, 0},
        {4, FULL_SCREEN, TBSTATE_ENABLED, TBSTYLE_BUTTON  | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
        {0,  0,           TBSTATE_ENABLED, TBSTYLE_SEP,    {0,0}, 0, 0},
        {5, IDGFXCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
        {6, IDSOUNDCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
        {7, IDINPUTCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
        {8, IDRSPCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
        {9, ID_LOAD_CONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0,0}, 0, 0},
    };

            hTool = CreateToolbarEx (hwnd, 
                WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT |
                /*WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NORESIZE |*/ CCS_ADJUSTABLE, 
                IDC_TOOLBAR, 10, app_hInstance, IDB_TOOLBAR, 
                (LPCTBBUTTON)&tbButtons, 13, 16, 16, 200, 25,  sizeof (TBBUTTON)); 
                                
			if(hTool == NULL)
				MessageBox(hwnd, "Could not create tool bar.", "Error", MB_OK | MB_ICONERROR);


            if (emu_launched) {
		   	      if (emu_paused) {
                      SendMessage( hTool, TB_CHECKBUTTON, EMU_PAUSE, 1);        
                  }
                  else {
                      SendMessage( hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
                  }     
            }
			else
            {
                  getSelectedRom();     //Used for enabling/disabling the play button
                  //SendMessage( hTool, TB_ENABLEBUTTON, EMU_PLAY, FALSE );
                  SendMessage( hTool, TB_ENABLEBUTTON, EMU_STOP, FALSE );
                  SendMessage( hTool, TB_ENABLEBUTTON, EMU_PAUSE, FALSE );
                  SendMessage( hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE );  
            }
		   	
}

void CreateStatusBarWindow(HWND hwnd)
{
   //Create Status bar
   hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
			WS_CHILD | WS_VISIBLE /*| SBARS_SIZEGRIP*/, 0, 0, 0, 0,
			hwnd, (HMENU)IDC_MAIN_STATUS, GetModuleHandle(NULL), NULL);

   if (emu_launched) SetStatusMode( 2 );
   else SetStatusMode( 0 );
}

int pauseAtFrame = -1;


LRESULT CALLBACK PlayMovieProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    char tempbuf [MAX_PATH];
    char tempbuf2 [MAX_PATH];
	
//    md5_byte_t digest[16];
//    ROM_INFO *pRomInfo;
//    char tempname[100];
//    HWND hwndPB;    
//    pRomInfo = getSelectedRom();
//    DEFAULT_ROM_SETTINGS TempRomSettings;

	static char path_buffer [_MAX_PATH];
    
//    if (pRomInfo==NULL) { 
//       EndDialog(hwnd, IDOK);
//       return FALSE; 
//      }
    switch(Message)
    {
        case WM_INITDIALOG:
			
			CountryCodeToCountryName(ROM_HEADER->Country_code,tempbuf2);
			sprintf(tempbuf, "%s (%s)", ROM_HEADER->nom, tempbuf2);
			strcat(tempbuf, ".m64");
			SetDlgItemText(hwnd,IDC_INI_MOVIEFILE,tempbuf);

			SetDlgItemText(hwnd,IDC_ROM_INTERNAL_NAME2, (CHAR*)ROM_HEADER->nom);
			
			CountryCodeToCountryName(ROM_HEADER->Country_code,tempbuf);
			SetDlgItemText(hwnd,IDC_ROM_COUNTRY2,tempbuf);

			sprintf(tempbuf, "%X", (unsigned int)ROM_HEADER->CRC1);
			SetDlgItemText(hwnd,IDC_ROM_CRC3,tempbuf);

			SetDlgItemText(hwnd,IDC_MOVIE_VIDEO_TEXT2,gfx_name);
			SetDlgItemText(hwnd,IDC_MOVIE_INPUT_TEXT2,input_name);
			SetDlgItemText(hwnd,IDC_MOVIE_SOUND_TEXT2,sound_name);
			SetDlgItemText(hwnd,IDC_MOVIE_RSP_TEXT2,rsp_name);

			strcpy(tempbuf, Controls[0].Present ? "Present" : "Disconnected");
			if(Controls[0].Present && Controls[0].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if(Controls[0].Present && Controls[0].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble");
			SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER1_TEXT2,tempbuf);

			strcpy(tempbuf, Controls[1].Present ? "Present" : "Disconnected");
			if(Controls[1].Present && Controls[1].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if(Controls[1].Present && Controls[1].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER2_TEXT2,tempbuf);

			strcpy(tempbuf, Controls[2].Present ? "Present" : "Disconnected");
			if(Controls[2].Present && Controls[2].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if(Controls[2].Present && Controls[2].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER3_TEXT2,tempbuf);

			strcpy(tempbuf, Controls[3].Present ? "Present" : "Disconnected");
			if(Controls[3].Present && Controls[3].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if(Controls[3].Present && Controls[3].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER4_TEXT2,tempbuf);
			
	       	CheckDlgButton(hwnd,IDC_MOVIE_READONLY,VCR_getReadOnly());
	       
	       
			SetFocus(GetDlgItem(hwnd,IDC_INI_MOVIEFILE));
	       

			goto refresh; // better than making it a macro or zillion-argument function
	       
	       
			
//             romInfoHWND = hwnd;
/*             
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
             
             TempRomSettings = GetDefaultRomSettings( pRomInfo->InternalName) ;
             
             WriteComboBoxValue( hwnd, IDC_COMBO_GFX, TempRomSettings.GfxPluginName, gfx_name);
             WriteComboBoxValue( hwnd, IDC_COMBO_INPUT, TempRomSettings.InputPluginName, input_name);
             WriteComboBoxValue( hwnd, IDC_COMBO_SOUND, TempRomSettings.SoundPluginName, sound_name);
             WriteComboBoxValue( hwnd, IDC_COMBO_RSP, TempRomSettings.RspPluginName, rsp_name);
             
             if (Config.OverwritePluginSettings) {
                 EnableWindow( GetDlgItem(hwnd,IDC_COMBO_GFX), FALSE );
                 EnableWindow( GetDlgItem(hwnd,IDC_COMBO_INPUT), FALSE );
                 EnableWindow( GetDlgItem(hwnd,IDC_COMBO_SOUND), FALSE );
                 EnableWindow( GetDlgItem(hwnd,IDC_COMBO_RSP), FALSE );         
             }
             
             //Disables the button because of a bug in the emulator:
             //Sound gets distorted if you push the button while ingame
             //Hack: you should check this
             if (emu_launched) EnableWindow( GetDlgItem(hwnd,IDC_MD5_CALCULATE), FALSE );             
             
             SetDlgItemText(hwnd,IDC_ROM_FULLPATH,pRomInfo->szFullFileName);
             SetDlgItemText(hwnd, IDC_ROM_GOODNAME, pRomInfo->GoodName);
             SetDlgItemText(hwnd,IDC_ROM_INTERNAL_NAME,pRomInfo->InternalName);
             sprintf(TempMessage,"%.1f MBit",(float)pRomInfo->RomSize/0x20000); 
             SetDlgItemText(hwnd, IDC_ROM_SIZE, TempMessage); 
             CountryCodeToCountryName(pRomInfo->Country,country);
             SetDlgItemText(hwnd, IDC_ROM_COUNTRY, country);
             sprintf(TempMessage,"%08X-%08X-C%02X",(int)pRomInfo->CRC1,(int)pRomInfo->CRC2,pRomInfo->Country);
             SetDlgItemText(hwnd, IDC_ROM_INICODE, TempMessage);
             SetDlgItemText(hwnd, IDC_ROM_MD5, pRomInfo->MD5);
             getIniComments(pRomInfo,TempMessage);
             SetDlgItemText(hwnd, IDC_INI_COMMENTS, TempMessage);
             TranslateRomInfoDialog(hwnd);*/
             return FALSE;
        case WM_CLOSE:
            if (!emu_launched) {
                ShowWindow( hRomList, FALSE ); 
                ShowWindow( hRomList, TRUE );
            }
              EndDialog(hwnd, IDOK);
//              romInfoHWND = NULL;   
        break; 
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_OK:
                case IDOK:

					{
						BOOL success;
						unsigned int num = GetDlgItemInt(hwnd, IDC_PAUSEAT_FIELD, &success, TRUE);
						if(((signed int)num) >= 0 && success)
							pauseAtFrame = num;
						else
							pauseAtFrame = -1;
					}
					GetDlgItemText(hwnd,IDC_INI_MOVIEFILE,tempbuf,MAX_PATH);

					// turn WCHAR into UTF8
					WCHAR authorWC [MOVIE_AUTHOR_DATA_SIZE];
					char authorUTF8 [MOVIE_AUTHOR_DATA_SIZE * 4];
					if(GetDlgItemTextW(hwnd,IDC_INI_AUTHOR,authorWC,MOVIE_AUTHOR_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, authorWC, -1, authorUTF8, sizeof(authorUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd,IDC_INI_AUTHOR,authorUTF8,MOVIE_AUTHOR_DATA_SIZE);

					WCHAR descriptionWC [MOVIE_DESCRIPTION_DATA_SIZE];
					char descriptionUTF8 [MOVIE_DESCRIPTION_DATA_SIZE * 4];
					if(GetDlgItemTextW(hwnd,IDC_INI_DESCRIPTION,descriptionWC,MOVIE_DESCRIPTION_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, descriptionWC, -1, descriptionUTF8, sizeof(descriptionUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd,IDC_INI_DESCRIPTION,descriptionUTF8,MOVIE_DESCRIPTION_DATA_SIZE);

					VCR_setReadOnly(IsDlgButtonChecked(hwnd,IDC_MOVIE_READONLY));
                    if (strlen(tempbuf) == 0 || VCR_startPlayback( tempbuf, authorUTF8, descriptionUTF8 ) < 0)
                    {
						sprintf(tempbuf2, "Couldn't start playback\nof \"%s\".", tempbuf);
                       MessageBox(hwnd, tempbuf2, "VCR", MB_OK);
                       break;
					}
                    else {
						HMENU hMenu = GetMenu(mainHWND);
						EnableMenuItem(hMenu,ID_STOP_RECORD,MF_GRAYED);
						EnableMenuItem(hMenu,ID_STOP_PLAYBACK,MF_ENABLED);
						if(!emu_paused || !emu_launched)
							SetStatusTranslatedString(hStatus,0,"Playback started...");
						else
							SetStatusTranslatedString(hStatus,0,"Playback started. (Paused)");
                    }
//                    GetDlgItemText(hwnd, IDC_INI_COMMENTS, (LPSTR) TempMessage, 128 );
//                    setIniComments(pRomInfo,TempMessage);
//                    strncpy(pRomInfo->UserNotes,TempMessage,sizeof(pRomInfo->UserNotes));
                    if (!emu_launched) {                    //Refreshes the ROM Browser
                        ShowWindow( hRomList, FALSE ); 
                        ShowWindow( hRomList, TRUE );
                    }
                    EndDialog(hwnd, IDOK);
//                    romInfoHWND = NULL; 
	                break;
                case IDC_CANCEL:
                case IDCANCEL:
                    if (!emu_launched) {
                        ShowWindow( hRomList, FALSE ); 
                        ShowWindow( hRomList, TRUE );
                    }
                    EndDialog(hwnd, IDOK);
//                    romInfoHWND = NULL;
                	break;
                case IDC_MOVIE_BROWSE:
					{
				     OPENFILENAME oifn;
                     ZeroMemory(&oifn, sizeof(OPENFILENAME));
                     oifn.lStructSize = sizeof(OPENFILENAME);
                     oifn.hwndOwner = hwnd;
                     strcpy(path_buffer,"");
                     oifn.lpstrFile = path_buffer,
                     oifn.nMaxFile = sizeof(path_buffer);
                     oifn.lpstrFilter = "Mupen 64 movies(*.m64)\0*.m64\0All Files\0*.*\0";
                     oifn.lpstrFileTitle = "";
                     oifn.nMaxFileTitle = 0;
                     oifn.lpstrInitialDir = "";
                     oifn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                     if (GetOpenFileName(&oifn)) {
						SetDlgItemText(hwnd,IDC_INI_MOVIEFILE,path_buffer);
                     }
					}
					goto refresh;
					break;
				  case IDC_MOVIE_REFRESH:
					goto refresh;
					break;
            }
        break;
    }
    return FALSE;

refresh:


ShowInfo("[VCR]:refreshing movie info...");


    GetDlgItemText(hwnd,IDC_INI_MOVIEFILE,tempbuf,MAX_PATH);
	SMovieHeader m_header = VCR_getHeaderInfo(tempbuf);

	SetDlgItemText(hwnd,IDC_ROM_INTERNAL_NAME,m_header.romNom);
	
	CountryCodeToCountryName(m_header.romCountry,tempbuf);
	SetDlgItemText(hwnd,IDC_ROM_COUNTRY,tempbuf);

	sprintf(tempbuf, "%X", (unsigned int)m_header.romCRC);
	SetDlgItemText(hwnd,IDC_ROM_CRC,tempbuf);

//ShowInfo("refreshing movie plugins...\n");

	SetDlgItemText(hwnd,IDC_MOVIE_VIDEO_TEXT,m_header.videoPluginName);
	SetDlgItemText(hwnd,IDC_MOVIE_INPUT_TEXT,m_header.inputPluginName);
	SetDlgItemText(hwnd,IDC_MOVIE_SOUND_TEXT,m_header.soundPluginName);
	SetDlgItemText(hwnd,IDC_MOVIE_RSP_TEXT,m_header.rspPluginName);

//ShowInfo("refreshing movie controllers...\n");

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_1_PRESENT) ? "Present" : "Disconnected");
	if(m_header.controllerFlags & CONTROLLER_1_MEMPAK)
		strcat(tempbuf, " with mempak");
	if(m_header.controllerFlags & CONTROLLER_1_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER1_TEXT,tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_2_PRESENT) ? "Present" : "Disconnected");
	if(m_header.controllerFlags & CONTROLLER_2_MEMPAK)
		strcat(tempbuf, " with mempak");
	if(m_header.controllerFlags & CONTROLLER_2_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER2_TEXT,tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_3_PRESENT) ? "Present" : "Disconnected");
	if(m_header.controllerFlags & CONTROLLER_3_MEMPAK)
		strcat(tempbuf, " with mempak");
	if(m_header.controllerFlags & CONTROLLER_3_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER3_TEXT,tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_4_PRESENT) ? "Present" : "Disconnected");
	if(m_header.controllerFlags & CONTROLLER_4_MEMPAK)
		strcat(tempbuf, " with mempak");
	if(m_header.controllerFlags & CONTROLLER_4_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER4_TEXT,tempbuf);

//ShowInfo("refreshing movie start/frames...\n");

	SetDlgItemText(hwnd,IDC_FROMSNAPSHOT_TEXT,(m_header.startFlags & MOVIE_START_FROM_SNAPSHOT) ? "Snapshot" : "Start");

	sprintf(tempbuf, "%u  (%u input)", (int)m_header.length_vis, (int)m_header.length_samples);
	SetDlgItemText(hwnd,IDC_MOVIE_FRAMES,tempbuf);

//ShowInfo("calculating movie length...\n");

	if(m_header.vis_per_second == 0)
		m_header.vis_per_second = 60;

	double seconds = (double)m_header.length_vis / (double)m_header.vis_per_second;
	double minutes = seconds / 60.0;
	if(seconds)
		seconds = fmod(seconds, 60.0);
	double hours = minutes / 60.0;
	if(minutes)
		minutes = fmod(minutes, 60.0);

//ShowInfo("refreshing movie length...\n");

	if(hours >= 1.0)
		sprintf(tempbuf, "%d hours and %.1f minutes", (unsigned int)hours, (float)minutes);
	else if(minutes >= 1.0)
		sprintf(tempbuf, "%d minutes and %.0f seconds", (unsigned int)minutes, (float)seconds);
	else if(m_header.length_vis != 0)
		sprintf(tempbuf, "%.1f seconds", (float)seconds);
	else
		strcpy(tempbuf, "0 seconds");
	SetDlgItemText(hwnd,IDC_MOVIE_LENGTH,tempbuf);

//ShowInfo("refreshing movie rerecords...\n");

	sprintf(tempbuf, "%u", m_header.rerecord_count);
	SetDlgItemText(hwnd,IDC_MOVIE_RERECORDS,tempbuf);

ShowInfo("[VCR]:refreshing movie author and description...");

//	SetDlgItemText(hwnd,IDC_INI_AUTHOR,m_header.authorInfo);
//	SetDlgItemText(hwnd,IDC_INI_DESCRIPTION,m_header.description);

	{

		// convert utf8 metadata to windows widechar
		WCHAR wszMeta [MOVIE_MAX_METADATA_SIZE];
		if(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_header.authorInfo, -1, wszMeta, MOVIE_AUTHOR_DATA_SIZE))
		{
			SetLastError(0);
			SetWindowTextW(GetDlgItem(hwnd,IDC_INI_AUTHOR), wszMeta);
			if(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
			{
				// not implemented on this system - convert as best we can to 1-byte characters and set with that
				// TODO: load unicows.dll instead so SetWindowTextW won't fail even on Win98/ME
				char ansiStr [MOVIE_AUTHOR_DATA_SIZE];
				WideCharToMultiByte(CP_ACP,0,wszMeta,-1,ansiStr,MOVIE_AUTHOR_DATA_SIZE,NULL,NULL);
				SetWindowTextA(GetDlgItem(hwnd,IDC_INI_AUTHOR), ansiStr);

				if(strlen(ansiStr) == 0)
					SetWindowTextA(GetDlgItem(hwnd,IDC_INI_AUTHOR), "(too lazy to type name)");

				SetLastError(0);
			}
			else
			{
				if(wcslen(wszMeta) == 0)
					SetWindowTextW(GetDlgItem(hwnd,IDC_INI_AUTHOR), L"(too lazy to type name)");
			}
		}
		if(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_header.description, -1, wszMeta, MOVIE_DESCRIPTION_DATA_SIZE))
		{
			SetWindowTextW(GetDlgItem(hwnd,IDC_INI_DESCRIPTION), wszMeta);
			if(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
			{
				char ansiStr [MOVIE_DESCRIPTION_DATA_SIZE];
				WideCharToMultiByte(CP_ACP,0,wszMeta,-1,ansiStr,MOVIE_DESCRIPTION_DATA_SIZE,NULL,NULL);
				SetWindowTextA(GetDlgItem(hwnd,IDC_INI_DESCRIPTION), ansiStr);

				if(strlen(ansiStr) == 0)
					SetWindowTextA(GetDlgItem(hwnd,IDC_INI_DESCRIPTION), "(no description entered)");

				SetLastError(0);
			}
			else
			{
				if(wcslen(wszMeta) == 0)
					SetWindowTextW(GetDlgItem(hwnd,IDC_INI_DESCRIPTION), L"(no description entered)");
			}
		}
	}

ShowInfo("[VCR]:done refreshing");

    return FALSE;
}

LRESULT CALLBACK RecordMovieProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{    
    char tempbuf [MAX_PATH];
    char tempbuf2 [MAX_PATH];
    switch(Message)
    {
        case WM_INITDIALOG:

			SetDlgItemText(hwnd,IDC_INI_AUTHOR,"");
			SetDlgItemText(hwnd,IDC_INI_DESCRIPTION,"");

			CheckRadioButton(hwnd,IDC_FROMSNAPSHOT_RADIO,IDC_FROMSTART_RADIO,IDC_FROMSTART_RADIO); // check "From Start"
			
			CountryCodeToCountryName(ROM_HEADER->Country_code,tempbuf2);
			sprintf(tempbuf, "%s (%s)", ROM_HEADER->nom, tempbuf2);
			strcat(tempbuf, ".m64");
			SetDlgItemText(hwnd,IDC_INI_MOVIEFILE,tempbuf);

			SetDlgItemText(hwnd,IDC_ROM_INTERNAL_NAME2, (CHAR*)ROM_HEADER->nom);
			
			CountryCodeToCountryName(ROM_HEADER->Country_code,tempbuf);
			SetDlgItemText(hwnd,IDC_ROM_COUNTRY2,tempbuf);

			sprintf(tempbuf, "%X", (unsigned int)ROM_HEADER->CRC1);
			SetDlgItemText(hwnd,IDC_ROM_CRC3,tempbuf);

			SetDlgItemText(hwnd,IDC_MOVIE_VIDEO_TEXT2,gfx_name);
			SetDlgItemText(hwnd,IDC_MOVIE_INPUT_TEXT2,input_name);
			SetDlgItemText(hwnd,IDC_MOVIE_SOUND_TEXT2,sound_name);
			SetDlgItemText(hwnd,IDC_MOVIE_RSP_TEXT2,rsp_name);

			strcpy(tempbuf, Controls[0].Present ? "Present" : "Disconnected");
			if(Controls[0].Present && Controls[0].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak"); 
			if(Controls[0].Present && Controls[0].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble");
			SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER1_TEXT2,tempbuf);

			strcpy(tempbuf, Controls[1].Present ? "Present" : "Disconnected");
			if(Controls[1].Present && Controls[1].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if(Controls[1].Present && Controls[1].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER2_TEXT2,tempbuf);

			strcpy(tempbuf, Controls[2].Present ? "Present" : "Disconnected");
			if(Controls[2].Present && Controls[2].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if(Controls[2].Present && Controls[2].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER3_TEXT2,tempbuf);

			strcpy(tempbuf, Controls[3].Present ? "Present" : "Disconnected");
			if(Controls[3].Present && Controls[3].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if(Controls[3].Present && Controls[3].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd,IDC_MOVIE_CONTROLLER4_TEXT2,tempbuf);
			
			SetFocus(GetDlgItem(hwnd,IDC_INI_AUTHOR));

             return FALSE;
        case WM_CLOSE:
            if (!emu_launched) {
                ShowWindow( hRomList, FALSE ); 
                ShowWindow( hRomList, TRUE );
            }
              EndDialog(hwnd, IDOK);
        break; 
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_OK:
                case IDOK:
				{
	
					// turn WCHAR into UTF8
					WCHAR authorWC [MOVIE_AUTHOR_DATA_SIZE];
					char authorUTF8 [MOVIE_AUTHOR_DATA_SIZE * 4];
					if(GetDlgItemTextW(hwnd,IDC_INI_AUTHOR,authorWC,MOVIE_AUTHOR_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, authorWC, -1, authorUTF8, sizeof(authorUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd,IDC_INI_AUTHOR,authorUTF8,MOVIE_AUTHOR_DATA_SIZE);

					WCHAR descriptionWC [MOVIE_DESCRIPTION_DATA_SIZE];
					char descriptionUTF8 [MOVIE_DESCRIPTION_DATA_SIZE * 4];
					if(GetDlgItemTextW(hwnd,IDC_INI_DESCRIPTION,descriptionWC,MOVIE_DESCRIPTION_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, descriptionWC, -1, descriptionUTF8, sizeof(descriptionUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd,IDC_INI_DESCRIPTION,descriptionUTF8,MOVIE_DESCRIPTION_DATA_SIZE);

					
					GetDlgItemText(hwnd,IDC_INI_MOVIEFILE,tempbuf,MAX_PATH);
                    if (strlen(tempbuf) == 0 || VCR_startRecord( tempbuf, IsDlgButtonChecked(hwnd,IDC_FROMSNAPSHOT_RADIO), authorUTF8, descriptionUTF8 ) < 0)
                    {
					   sprintf(tempbuf2, "Couldn't start recording\nof \"%s\".", tempbuf);
                       MessageBox(hwnd, tempbuf2, "VCR", MB_OK);
                       break;
					}
                    else {
					   HMENU hMenu = GetMenu(mainHWND);
                       EnableMenuItem(hMenu,ID_STOP_RECORD,MF_ENABLED);
                       EnableMenuItem(hMenu,ID_STOP_PLAYBACK,MF_GRAYED);
                       SetStatusTranslatedString(hStatus,0,"Recording replay...");
                    }
                    if (!emu_launched) {                    //Refreshes the ROM Browser
                        ShowWindow( hRomList, FALSE ); 
                        ShowWindow( hRomList, TRUE );
                    }
                    EndDialog(hwnd, IDOK);
                }	break;
                case IDC_CANCEL:
                case IDCANCEL:
                    if (!emu_launched) {
                        ShowWindow( hRomList, FALSE ); 
                        ShowWindow( hRomList, TRUE );
                    }
                    EndDialog(hwnd, IDOK);
                break;
                case IDC_MOVIE_BROWSE:
				{
				   OPENFILENAME oifn;
				   char path_buffer [MAX_PATH];
/*                    if((Controls[0].Present && Controls[0].RawData) ||
                       (Controls[1].Present && Controls[1].RawData) ||
                       (Controls[2].Present && Controls[2].RawData) ||
                       (Controls[3].Present && Controls[3].RawData))
                       {
                          MessageBox(NULL, "Warning: One of the active controllers of your input plugin is set to accept \"Raw Data\".", "VCR", MB_OK);
                       }*/
//                    if(!emu_paused) {
                     ZeroMemory(&oifn, sizeof(OPENFILENAME));
                     oifn.lStructSize = sizeof(OPENFILENAME);
                     oifn.hwndOwner = hwnd;
                     strcpy(path_buffer,"");
                     oifn.lpstrFile = path_buffer,
                     oifn.nMaxFile = sizeof(path_buffer);
                     oifn.lpstrFilter = "Mupen 64 movies(*.m64)\0*.m64\0All Files\0*.*\0";
                     oifn.lpstrFileTitle = "";
                     oifn.nMaxFileTitle = 0;
                     oifn.lpstrInitialDir = "";
                     oifn.Flags = OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
                     if (GetSaveFileName(&oifn)) {
						if(strlen(path_buffer) > 0 && (strlen(path_buffer) < 4 || stricmp(path_buffer + strlen(path_buffer) - 4, ".m64") != 0))
							strcat(path_buffer, ".m64");
						SetDlgItemText(hwnd,IDC_INI_MOVIEFILE,path_buffer);
                     }
				}	break;
            }
        break;
    }
    return FALSE;
}

void OpenMoviePlaybackDialog()
{
	BOOL wasPaused = emu_paused;
	if (emu_launched&&!emu_paused)
		pauseEmu(FALSE) ; 
	
	DialogBox(GetModuleHandle(NULL), 
	    MAKEINTRESOURCE(IDD_MOVIE_PLAYBACK_DIALOG), mainHWND, (DLGPROC)PlayMovieProc);
	
	if (emu_launched&&emu_paused&&!wasPaused)
		resumeEmu(FALSE);
}
void OpenMovieRecordDialog()
{
	BOOL wasPaused = emu_paused;
	if (emu_launched&&!emu_paused)
		pauseEmu(FALSE) ; 

    DialogBox(GetModuleHandle(NULL), 
        MAKEINTRESOURCE(IDD_MOVIE_RECORD_DIALOG), mainHWND, (DLGPROC)RecordMovieProc);
	
	if (emu_launched&&emu_paused&&!wasPaused)
		resumeEmu(FALSE);
}


void SetStatusMode( int mode )
{
    RECT rcClient;                                    //Client area of parent window 
    const int loadingwidths[] = {200, 300, -1};       //Initial statusbar
    const int emulatewidthsFPSVIS[] = {230, 300, 370, 440, -1};//Emulating statusbar with FPS and VIS
    const int emulatewidthsFPS[] = {230, 300, 370, -1};    //Emulating statusbar with FPS
    const int emulatewidths[] = {230, 300, -1};            //Emulating statusbar
    const int browserwidths[] = {400, -1};	          //Initial statusbar
    int parts;


    if (hStatusProgress) DestroyWindow( hStatusProgress );
    //if (hStatusIcon)     DeleteObject( (HGDIOBJ) hStatusIcon );
    if (hStaticHandle)   DestroyWindow( hStaticHandle );
    
    //Setting status widths
    if (Config.GuiStatusbar)
    {
      switch(mode)
      {
        case 0:                 //Rombrowser Statusbar
/*             //Adds sizing grid
             statusstyle = GetWindowLong( hStatus, GWL_STYLE );
             statusstyle = statusstyle | SBARS_SIZEGRIP;
             SetWindowLong( hStatus, GWL_STYLE, statusstyle );
             SetWindowPos( hStatus, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED );  //Set on top
*/
             SendMessage( hStatus, SB_SETPARTS, sizeof(browserwidths)/sizeof(int), (LPARAM)browserwidths);    
             SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg );
             SendMessage( hStatus, SB_SETTEXT, 1, (LPARAM)"" );
             //ShowTotalRoms();
        break;
        
        case 1:                 //Loading Statusbar
/*             //Removes the sizing grid
             statusstyle = GetWindowLong( hStatus, GWL_STYLE );
             statusstyle = statusstyle & ~SBARS_SIZEGRIP;
             SetWindowLong( hStatus, GWL_STYLE, statusstyle );
             SetWindowPos( hStatus, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED );  //Set on top
*/
             SendMessage( hStatus, SB_SETPARTS, sizeof(loadingwidths)/sizeof(int), (LPARAM)loadingwidths);
             SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg );
             SendMessage( hStatus, SB_SETTEXT, 1, (LPARAM)"" );
             SendMessage( hStatus, SB_SETTEXT, 2, (LPARAM)"" );
             
             GetClientRect(hStatus, &rcClient); 
                             
             hStatusProgress = CreateWindowEx(0, PROGRESS_CLASS,
	                            (LPSTR) NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
	                            204, 4, 89, rcClient.bottom-6, 
	                            hStatus, (HMENU) 0, app_hInstance, NULL); 
                                /*hStatusProgress = CreateWindowEx(0, PROGRESS_CLASS,
	                                      (LPSTR) NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
	                                      201, 2, 99, rcClient.bottom-2, 
	                                      hStatus, (HMENU) 0, app_hInstance, NULL); 
                                */
            SendMessage( hStatusProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100) );
            SendMessage( hStatusProgress, PBM_SETBARCOLOR, 0, RGB(90,0,100) );
        break;
        
        case 2:                    //Emulating Statusbar
             if (Config.showFPS && Config.showVIS)
             {
                 SendMessage( hStatus, SB_SETPARTS, sizeof(emulatewidthsFPSVIS)/sizeof(int), (LPARAM)emulatewidthsFPSVIS);
                 SendMessage( hStatus, SB_SETTEXT, 1, (LPARAM)""); //rr
                 SendMessage( hStatus, SB_SETTEXT, 2, (LPARAM)""); //FPS
                 SendMessage( hStatus, SB_SETTEXT, 3, (LPARAM)""); //VIS
                 parts = 5;
             }else if (Config.showFPS)
             {
                 SendMessage( hStatus, SB_SETPARTS, sizeof(emulatewidthsFPS)/sizeof(int), (LPARAM)emulatewidthsFPS);
                 SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
                 SendMessage( hStatus, SB_SETTEXT, 2, (LPARAM)"");
                 parts = 4;
             }else if (Config.showVIS)
             {
                 SendMessage( hStatus, SB_SETPARTS, sizeof(emulatewidthsFPS)/sizeof(int), (LPARAM)emulatewidthsFPS);
                 SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
                 SendMessage( hStatus, SB_SETTEXT, 2, (LPARAM)"");
                 parts = 4;
             }else
             {                          
                 SendMessage( hStatus, SB_SETPARTS, sizeof(emulatewidths)/sizeof(int), (LPARAM)emulatewidths);
                 SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
                 parts = 3;             
             }
               
             SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg );
             sprintf( TempMessage, "%s", ROM_SETTINGS.goodname );
             SendMessage( hStatus, SB_SETTEXT, parts-1, (LPARAM)TempMessage );
/*
             switch( ROM_HEADER->Country_code&0xFF )             //Choosing icon
             {
                case 0:                          
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_DEMO),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
		        break;                         //IDI_DEMO
			    case '7':
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_BETA),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
		        break;                         //IDI_BETA
                case 0x44:
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_GERMANY),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
                break;                         //IDI_GERMANY
                case 0x45:
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_USA),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
                break;                         //IDI_USA
                case 0x4A:
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_JAPAN),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
                break;                         //IDI_JAPAN 
                case 0x20:
                case 0x21:
                case 0x38:
                case 0x70:
                case 0x50:
                case 0x58:
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_EUROPE),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
                break;                         //IDI_EUROPE
                case 0x55:
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_AUSTRALIA),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
                break;                        //IDI_AUSTRALIA
                case 'I':
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_ITALIA),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
               break;                         //IDI_ITALIA 
                case 0x46:                       
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_FRANCE),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
                break;                        //IDI_FRANCE
                case 'S':                        
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_SPAIN),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
                break;                         //IDI_SPAIN
                default: 
                   hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_USA),
                                      IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
                break;
             }  

             GetClientRect(hStatus, &rcClient);
             hStaticHandle = CreateWindowEx(0, "Static", (LPCTSTR) "ROMIcon", 
                                WS_CHILD | WS_VISIBLE | SS_ICON,
	                            347, ((rcClient.bottom - rcClient.top) - 16)/2 + 1, 
                                0, 0, hStatus, (HMENU) 0, app_hInstance, NULL);
	         SendMessage( hStaticHandle, STM_SETICON, (WPARAM) hStatusIcon, 0 );
*/	         
        break;
       }        //Switch
     }        //If
}

void EnableEmulationMenuItems(BOOL flag)
{
   extern BOOL continue_vcr_on_restart_mode;
     
   HMENU hMenu, hSubMenu;
   hMenu = GetMenu(mainHWND);
   if (flag) {
      EnableMenuItem(hMenu,EMU_STOP,MF_ENABLED);
      //EnableMenuItem(hMenu,IDLOAD,MF_GRAYED);
      EnableMenuItem(hMenu,EMU_PAUSE,MF_ENABLED);
      EnableMenuItem(hMenu,EMU_FRAMEADVANCE,MF_ENABLED);
      EnableMenuItem(hMenu,EMU_PLAY,MF_ENABLED);
      EnableMenuItem(hMenu,FULL_SCREEN,MF_ENABLED);
      EnableMenuItem(hMenu,STATE_SAVE,MF_ENABLED);
      EnableMenuItem(hMenu,STATE_SAVEAS,MF_ENABLED);
      EnableMenuItem(hMenu,STATE_RESTORE,MF_ENABLED);
      EnableMenuItem(hMenu,STATE_LOAD,MF_ENABLED);
      EnableMenuItem(hMenu,GENERATE_BITMAP,MF_ENABLED);
      DisableRecentRoms( hMenu, FALSE);
      EnableMenuItem(hMenu,EMU_RESET,MF_ENABLED);
      EnableMenuItem(hMenu,REFRESH_ROM_BROWSER,MF_GRAYED);

      hSubMenu = GetSubMenu( hMenu, 3 );                        //Utilities menu
      EnableMenuItem(hSubMenu,6,MF_BYPOSITION | MF_ENABLED);    //Record Menu
if(!continue_vcr_on_restart_mode)
{
      EnableMenuItem(hMenu,ID_START_RECORD,MF_ENABLED);
      EnableMenuItem(hMenu,ID_STOP_RECORD, VCR_isRecording() ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem(hMenu,ID_START_PLAYBACK,MF_ENABLED);
      EnableMenuItem(hMenu,ID_STOP_PLAYBACK, VCR_isPlaying() ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem(hMenu,ID_START_CAPTURE,MF_ENABLED);
      EnableMenuItem(hMenu,ID_END_CAPTURE, VCR_isCapturing() ? MF_ENABLED : MF_GRAYED);
}
      
      if (Config.GuiToolbar)
      {
         SendMessage( hTool, TB_ENABLEBUTTON, EMU_PLAY, TRUE );
         SendMessage( hTool, TB_ENABLEBUTTON, EMU_STOP, TRUE );
         SendMessage( hTool, TB_ENABLEBUTTON, EMU_PAUSE, TRUE );
         if (recording && !externalReadScreen)
            SendMessage( hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE );
         else
            SendMessage( hTool, TB_ENABLEBUTTON, FULL_SCREEN, TRUE );
      }   
   }
   else {
      EnableMenuItem(hMenu,EMU_STOP,MF_GRAYED);
      EnableMenuItem(hMenu,IDLOAD,MF_ENABLED);
      EnableMenuItem(hMenu,EMU_PAUSE,MF_GRAYED);
      EnableMenuItem(hMenu,EMU_FRAMEADVANCE,MF_GRAYED);
      EnableMenuItem(hMenu,EMU_PLAY,MF_GRAYED);
      EnableMenuItem(hMenu,FULL_SCREEN,MF_GRAYED);
      EnableMenuItem(hMenu,STATE_SAVE,MF_GRAYED);
      EnableMenuItem(hMenu,STATE_SAVEAS,MF_GRAYED);
      EnableMenuItem(hMenu,STATE_RESTORE,MF_GRAYED);
      EnableMenuItem(hMenu,STATE_LOAD,MF_GRAYED);
      EnableMenuItem(hMenu,GENERATE_BITMAP,MF_GRAYED);
      DisableRecentRoms( hMenu, FALSE);
      EnableMenuItem(hMenu,EMU_RESET,MF_GRAYED);
      EnableMenuItem(hMenu,REFRESH_ROM_BROWSER,MF_ENABLED);
     
      hSubMenu = GetSubMenu( hMenu, 3 );                        //Utilities menu
//      EnableMenuItem(hSubMenu,6,MF_BYPOSITION | MF_GRAYED);    //Record Menu
      EnableMenuItem(hSubMenu,6,MF_BYPOSITION | MF_ENABLED);    //Record Menu
if(!continue_vcr_on_restart_mode)
{
      EnableMenuItem(hMenu,ID_START_RECORD,MF_GRAYED);
      EnableMenuItem(hMenu,ID_STOP_RECORD,MF_GRAYED);
      EnableMenuItem(hMenu,ID_STOP_PLAYBACK,MF_GRAYED);
      EnableMenuItem(hMenu,ID_START_CAPTURE,MF_GRAYED);
      EnableMenuItem(hMenu,ID_END_CAPTURE,MF_GRAYED);
}
      if (Config.GuiToolbar)
      {
         getSelectedRom(); //Used to check if the play button should be enabled or not
         //SendMessage( hTool, TB_ENABLEBUTTON, EMU_PLAY, FALSE );
         SendMessage( hTool, TB_ENABLEBUTTON, EMU_STOP, FALSE );
         SendMessage( hTool, TB_ENABLEBUTTON, EMU_PAUSE, FALSE );
         SendMessage( hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE );  
      }                    
   }
   
   if (Config.GuiToolbar) CheckMenuItem( hMenu, IDC_GUI_TOOLBAR,  MF_BYCOMMAND | MF_CHECKED );
   else CheckMenuItem( hMenu, IDC_GUI_TOOLBAR,  MF_BYCOMMAND | MF_UNCHECKED );
   if (Config.GuiStatusbar) CheckMenuItem( hMenu, IDC_GUI_STATUSBAR,  MF_BYCOMMAND | MF_CHECKED );
   else CheckMenuItem( hMenu, IDC_GUI_STATUSBAR,  MF_BYCOMMAND | MF_UNCHECKED );
}

static DWORD WINAPI SoundThread(LPVOID lpParam)
{
    while (emu_launched) aiUpdate(1);
    ExitThread(0);
}


static DWORD WINAPI ThreadFunc(LPVOID lpParam)
{
	ThreadFuncState = TFS_NONE;
    ShowInfo("Emu thread: Start");
    
	ThreadFuncState = TFS_INITMEM;
    ShowInfo("Init memory....");
    init_memory() ;
	ThreadFuncState = TFS_LOADPLUGINS;
    ShowInfo("Loading plugins....");
    load_plugins() ;
	ThreadFuncState = TFS_OPENGFX;
    ShowInfo("Rom open gfx....");
    romOpen_gfx();
	ThreadFuncState = TFS_OPENINPUT;
    ShowInfo("Rom open input....");
    romOpen_input();
	ThreadFuncState = TFS_OPENAUDIO;
    ShowInfo("Rom open audio....");
    romOpen_audio();
    
	ThreadFuncState = TFS_DISPLAYMODE;
    dynacore = Config.guiDynacore ;
    ShowInfo("Core = %s" , CoreNames[dynacore]);
          
    emu_paused = 0;
    emu_launched = 1;
    restart_mode = 0;
    
    if (Config.StartFullScreen) {
        FullScreenMode=1;
        gui_ChangeWindow(); 
    } 
	ThreadFuncState = TFS_CREATESOUND;
    ShowInfo("Emu thread: Creating sound thread...");   
    SoundThreadHandle = CreateThread(NULL, 0, SoundThread, NULL, 0, &SOUNDTHREADID);
	ThreadFuncState = TFS_EMULATING;
    ShowInfo("Emu thread: Emulation started....");
    go();
    ShowInfo("Emu thread: Core stopped...");
	ThreadFuncState = TFS_CLOSEINPUT;
    romClosed_input();
    ShowInfo("Emu thread: romClosed (input plugin)");
	ThreadFuncState = TFS_CLOSEAUDIO;
    romClosed_audio();
    ShowInfo("Emu thread: romClosed (audio plugin)");
	ThreadFuncState = TFS_CLOSERSP;
    romClosed_RSP();
    ShowInfo("Emu thread: romClosed (RSP plugin)");
	ThreadFuncState = TFS_UNLOADRSP;
    closeDLL_RSP();
    ShowInfo("Emu thread: RSP plugin closed");
	ThreadFuncState = TFS_UNLOADINPUT;
    closeDLL_input();
    ShowInfo("Emu thread: input plugin closed");
	ThreadFuncState = TFS_UNLOADAUDIO;
    closeDLL_audio();
    ShowInfo("Emu thread: audio plugin closed");
	ThreadFuncState = TFS_CLOSEGFX;
    romClosed_gfx();
    ShowInfo("Emu thread: romClosed (gfx plugin)");
	ThreadFuncState = TFS_UNLOADGFX;
    closeDLL_gfx();
    ShowInfo("Emu thread: gfx plugin closed");
	ThreadFuncState = TFS_NONE;
    ExitThread(0);
}

void exit_emu(int postquit)
{
   //SleepEx(100,TRUE);
   if(postquit){
	   if ((!cmdlineMode)||(cmdlineSave)) {
	      ini_updateFile(Config.compressedIni);
	      SaveConfig();
	      if (!cmdlineNoGui) {
	          SaveRomBrowserCache();
	      }    
	   } 
	   ini_closeFile();
	}

   //closeRom();
   CreateThread(NULL, 0, closeRom, NULL, 0, &Id); //
   
   if(postquit){
	   freeRomDirList(); 
	   freeRomList();
	   freeLanguages();
	   PostQuitMessage (0);
	}
}

void exit_emu2()
{
     if ((!cmdlineMode)||(cmdlineSave)) {
      ini_updateFile(Config.compressedIni);
      SaveConfig();
      if (!cmdlineNoGui) {
          SaveRomBrowserCache();
      }    
   } 
   ini_closeFile();
   freeRomDirList(); 
   freeRomList();
   freeLanguages();
   PostQuitMessage (0);
}

void ProcessToolTips(LPARAM lParam)
{
    LPTOOLTIPTEXT lpttt; 

    lpttt = (LPTOOLTIPTEXT) lParam; 
    lpttt->hinst = app_hInstance; 

    // Specify the resource identifier of the descriptive 
    // text for the given button. 
    switch (lpttt->hdr.idFrom) 
	{ 
		case IDLOAD:
			 TranslateDefault("Load ROM...","Load ROM...",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break;
		case EMU_PLAY:
		     TranslateDefault("Start/Resume Emulation","Start/Resume Emulation",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break; 
  		case EMU_PAUSE:
             TranslateDefault("Pause Emulation","Pause Emulation",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break; 	
		case EMU_STOP:
  	         TranslateDefault("Stop Emulation","Stop Emulation",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break; 
       case FULL_SCREEN:
             TranslateDefault("Full Screen","Full Screen",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break;
        case IDGFXCONFIG:
             TranslateDefault("Video Settings...","Video Settings...",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break;
        case IDSOUNDCONFIG:
             TranslateDefault("Audio Settings...","Audio Settings...",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break;
        case IDINPUTCONFIG:
             TranslateDefault("Input Settings...","Input Settings...",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break;
        case IDRSPCONFIG:
             TranslateDefault("RSP Settings...","RSP Settings...",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break;
		case ID_LOAD_CONFIG:
             TranslateDefault("Settings...","Settings...",TempMessage) ;
             lpttt->lpszText = TempMessage; 
			break;
                 	
   }
}

void EnableStatusbar()
{
	if (Config.GuiStatusbar)
	{
		if (!IsWindow( hStatus ))
		{
			CreateStatusBarWindow( mainHWND );
			ResizeRomListControl();
		}
	}
	else
	{
		DestroyWindow( hStatus );
		hStatus = NULL;
		ResizeRomListControl(); 
	}    
}

void EnableToolbar()
{
	if (Config.GuiToolbar && !VCR_isCapturing())
	{
		if(!hTool || !IsWindow(hTool))
		{
			CreateToolBarWindow(mainHWND);
			ResizeRomListControl();
		}
	}
	else
	{
		if(hTool && IsWindow(hTool))
		{
			DestroyWindow( hTool);
			hTool = NULL ;
			ResizeRomListControl(); 
		}
	}
}


LRESULT CALLBACK NoGuiWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
     switch(Message)
     {
       case WM_KEYDOWN:
                switch (wParam)
                {
                    case VK_TAB:
                      manualFPSLimit = 0 ; 
                    break;            
                }  
                if (emu_launched) keyDown(wParam, lParam);
               break;
        case WM_KEYUP:
                switch (wParam)
                {
                    case VK_TAB:
                      manualFPSLimit = 1 ;
                    break;
                    case VK_ESCAPE:
                      exit_emu(1) ;
                    break;            
                } 
                if (emu_launched) keyUp(wParam, lParam);
                break; 
        case WM_MOVE:
            if (emu_launched&&!FullScreenMode) {
                     moveScreen(wParam, lParam);
                    }
            break;
        case WM_USER + 17:  SetFocus(mainHWND); 
            break;      
        case WM_CREATE:
            // searching the plugins...........
            search_plugins();
            setDefaultPlugins(); 
            ////////////////////////////
            return TRUE;
        case WM_CLOSE:
             exit_emu(1);
            break; 
            
        default:
            return DefWindowProc(hwnd, Message, wParam, lParam);
    }
    return TRUE;                
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	char path_buffer[_MAX_PATH];
	OPENFILENAME oifn;
	int ret;
	int i;
	static PAINTSTRUCT	ps;
	BOOL	minimize ;
	HMENU hMenu;
	hMenu = GetMenu(hwnd);
	
#ifdef LUA_WINDOWMESSAGE
			LuaWindowMessage(hwnd, Message, wParam, lParam);
#endif
	switch(Message)
	{
    case WM_DROPFILES:
        //HDROP hFile = (HDROP) wParam;
		char fname[MAX_PATH];
		LPSTR fext;
		DragQueryFile((HDROP)wParam, 0, fname, sizeof(fname));
		fext = CharUpper(PathFindExtension(fname));

		if (lstrcmp(fext, ".N64") == 0 || lstrcmp(fext, ".V64") == 0 || lstrcmp(fext, ".Z64") == 0 || lstrcmp(fext, ".ROM") == 0) {
			StartRom(fname);
		}	
		else if (lstrcmp(fext, ".M64") == 0) {
			if (rom) {
				if (!VCR_getReadOnly())	VCR_toggleReadOnly();
				VCR_startPlayback(fname, 0, 0);
				EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
				EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_ENABLED);
			}
		}
		else if (strcmp(fext, ".ST") == 0) {
			if (rom) {
				savestates_select_filename(fname);
				savestates_job = LOADSTATE;
			}
		}
		
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		{
			BOOL hit = FALSE;
			if(manualFPSLimit)
			{
				if((int)wParam == Config.hotkey[0].key) // fast-forward on
				{
					if(((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.hotkey[0].shift
					&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.hotkey[0].ctrl
					&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.hotkey[0].alt)
					{
						manualFPSLimit = 0;
						hit = TRUE;
					}
				}
			}
			for(i = 1 ; i < NUM_HOTKEYS ; i++)
			{
				if((int)wParam == Config.hotkey[i].key)
				{
					if(((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.hotkey[i].shift
					&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.hotkey[i].ctrl
					&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.hotkey[i].alt)
					{
						SendMessage(hwnd, WM_COMMAND, Config.hotkey[i].command, 0);
						hit = TRUE;
					}
				}
			}
			if (emu_launched)
				keyDown(wParam, lParam);
			if(!hit)
				return DefWindowProc(hwnd, Message, wParam, lParam);
        }	break;
	case WM_KEYUP:
			if((int)wParam == Config.hotkey[0].key) // fast-forward off
			{
            	manualFPSLimit = 1 ; 
			}
            if (emu_launched)
				keyUp(wParam, lParam);
           return DefWindowProc(hwnd, Message, wParam, lParam);
           break;
	case WM_NOTIFY:
            if (wParam == IDC_ROMLIST) { 
             RomListNotify((LPNMHDR)lParam); 
            }
            switch (((LPNMHDR) lParam)->code) 
            	{ 
		            case TTN_NEEDTEXT : 
                          ProcessToolTips(lParam);
				    break;
		        }
		    return 0;
	case WM_MOVE:
            if (emu_launched&&!FullScreenMode) {
                     moveScreen(wParam, lParam);
                    }
            break;
	case WM_SIZE:
             if (!FullScreenMode) {
              SendMessage(hTool, TB_AUTOSIZE, 0, 0);
              SendMessage(hStatus, WM_SIZE, 0, 0);  
              ResizeRomListControl();      
             } 
            break;  
	case WM_USER + 17:  SetFocus(mainHWND); break;
	case WM_CREATE:
            // searching the plugins...........
            GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
            search_plugins();
            setDefaultPlugins(); 
            CreateToolBarWindow( hwnd);
            CreateStatusBarWindow( hwnd);
            SetupLanguages( hwnd);
            TranslateMenu(GetMenu( hwnd), hwnd);
            CreateRomListControl( hwnd);
            SetRecentList( hwnd);
            EnableToolbar();
            EnableStatusbar();
            ////////////////////////////
            return TRUE;
	case WM_CLOSE:
		if(emu_launched)
		{
			shut_window = 1;
			exit_emu(0);
			return 0;
		}
		else
		{
			exit_emu(1);
			//DestroyWindow(hwnd);
		}
		break;
        
	case WM_PAINT:
/*			if(emu_launched && emu_paused)
			{
				//// doesn't work...
				//extern void (*updateScreen)();
				//updateScreen();
				
				// since I can't convince the graphics plugin to update the window,
				// just clear everything so we don't get partial clearage...
				// (only if capturing - partial is probably better than full otherwise)
				if(VCR_isCapturing())
				{
					InvalidateRect(hwnd,NULL,TRUE);
					VCR_invalidatedCaptureFrame();
				}
			}*/

//			if(VCR_isCapturing())
//				VCR_invalidatedCaptureFrame();

		BeginPaint(hwnd, &ps);

//			if(VCR_isCapturing())
//				VCR_invalidatedCaptureFrame();

		EndPaint(hwnd, &ps);


//			if(VCR_isCapturing())
//				VCR_invalidatedCaptureFrame();

		return 0;

//		case WM_SETCURSOR:
//			SetCursor(FALSE);
//			return 0;
	    
	case WM_ENTERMENULOOP:       
             AutoPause = emu_paused;
             if (!emu_paused)
             {
				MenuPaused = TRUE;
                pauseEmu(FALSE) ;  
             }  
             break;
        
	case WM_EXITMENULOOP:
			 MenuPaused = FALSE;
             if (emu_paused&&!AutoPause)
             {
                resumeEmu(FALSE) ;
             }
             break;
	case WM_ACTIVATE:
			minimize = (BOOL) HIWORD(wParam);
			
			switch(LOWORD(wParam))
			{
			case WA_ACTIVE:
		    case WA_CLICKACTIVE:
			    if (Config.PauseWhenNotActive&&emu_paused&&!AutoPause ) {
                    resumeEmu(FALSE) ;
                    AutoPause = emu_paused; 
                 }  
            break;
			
			case WA_INACTIVE:
				  AutoPause = emu_paused;
                  if ( Config.PauseWhenNotActive && !emu_paused /*&& minimize*/ && !FullScreenMode) { 
                    pauseEmu(FALSE) ;
                  }
			break;
			}
			break;
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case ID_MENU_LUASCRIPT_NEW:
				{
#ifdef LUA_CONSOLE
					MUPEN64RR_DEBUGINFO("LuaScript New");
					::NewLuaScript((void(*)())lParam);
#endif
				} break;
			case ID_MENU_LUASCRIPT_CLOSEALL:
				{
#ifdef LUA_CONSOLE
					MUPEN64RR_DEBUGINFO("LuaScript CloseALL");
					::CloseAllLuaScript();
#endif
				} break;
			case ID_TRACELOG:
#ifdef LUA_TRACELOG
#ifdef LUA_TRACEINTERP
				if(!dynacore) {
					::LuaTraceLogState();
				}else {
					MessageBox(mainHWND,
						"trace logging works only in (pure) interpreter core\n"
						"(Menu->Option->Settings->General->CPU Core->(Pure) Interpreter)",
						NULL, 0);
				}
#else
				if(!dynacore && interpcore == 1) {
					::LuaTraceLogState();
				}else {
					MessageBox(mainHWND,
						"trace logging works only in pure interpreter core\n"
						"(Menu->Option->Settings->General->CPU Core->Pure Interpreter)",
						NULL, 0);
				}
#endif
#endif
				break;
			case IDGFXCONFIG:
				{
				 BOOL wasPaused = emu_paused;
                 if (emu_launched&&!emu_paused)
                    pauseEmu(FALSE) ; 
                 
                 hwnd_plug = hwnd;
                 exec_config(gfx_name);

                 if (emu_launched&&emu_paused&&!wasPaused)
                    resumeEmu(FALSE);
				}	break;
			case IDINPUTCONFIG:
				{
				 BOOL wasPaused = emu_paused;
                 if (emu_launched&&!emu_paused)
                    pauseEmu(FALSE) ; 

                 hwnd_plug = hwnd;
                 exec_config(input_name);

                 if (emu_launched&&emu_paused&&!wasPaused)
                    resumeEmu(FALSE);
				}	break;
			case IDSOUNDCONFIG:
				{
				 BOOL wasPaused = emu_paused;
                 if (emu_launched&&!emu_paused)
                    pauseEmu(FALSE) ; 

                 hwnd_plug = hwnd;
                 exec_config(sound_name);

                 if (emu_launched&&emu_paused&&!wasPaused)
                    resumeEmu(FALSE);
				}	break;
			case IDRSPCONFIG:
				{
				 BOOL wasPaused = emu_paused;
                 if (emu_launched&&!emu_paused)
                    pauseEmu(FALSE) ; 

                 hwnd_plug = hwnd;
                 exec_config(rsp_name);           

                 if (emu_launched&&emu_paused&&!wasPaused)
                    resumeEmu(FALSE);
				}	break;  
			case EMU_STOP:
                 if (emu_launched) {
                       //closeRom();
                       stop_it();
                       SleepEx(1000, TRUE); //todo check if can remove
                       CreateThread(NULL, 0, closeRom, (LPVOID)1, 0, &Id);
                       SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
                       SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
                     }
                 break;
                
			case EMU_PAUSE:
                 if (!emu_paused) {
                   pauseEmu(VCR_isActive()) ; 
                 }
                 else {
                   resumeEmu(VCR_isActive());               
                 }
                break;
               
			case EMU_FRAMEADVANCE:
                {
					extern int frame_advancing;
					frame_advancing = 1; 
					resumeEmu(TRUE); // maybe multithreading unsafe

                }
                break;
               
			case EMU_VCRTOGGLEREADONLY:
				{
					VCR_toggleReadOnly();
				}
				break;
                
			case EMU_PLAY:
                 if (emu_launched) 
                 {
                   if (emu_paused) {
                      resumeEmu(FALSE);
                   }
                   else{
                      SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);  
                      //The button is always checked when started and not on pause
                   }
                 }
				 else
                 {
                    RomList_OpenRom();
                 }
                 break;
                
			case EMU_RESET:
                 resetEmu() ;
                 break;
               
			case ID_LOAD_CONFIG:
				{
				 BOOL wasPaused = emu_paused;
                 if (emu_launched&&!emu_paused) {
                    pauseEmu(FALSE) ; 
                 }
                 ChangeSettings(hwnd);
		         ini_updateFile(Config.compressedIni);
                 if (emu_launched&&emu_paused&&!wasPaused) {
                    resumeEmu(FALSE);
                 }
				}
                 break;
			case ID_AUDIT_ROMS:
                     ret = DialogBox(GetModuleHandle(NULL), 
                     MAKEINTRESOURCE(IDD_AUDIT_ROMS_DIALOG), hwnd, AuditDlgProc);
                     break;
			case ID_HELP_ABOUT:
                     ret = DialogBox(GetModuleHandle(NULL), 
                     MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
                     break;
			case ID_HELP_CONTENS:
                     sprintf(TempMessage,"%sreadme.pdf",AppPath);
                     ShellExecute(hwnd, "open", TempMessage, NULL, NULL, SW_SHOWNORMAL);           
                     break;
			case ID_HELP_WHATSNEW:
                     sprintf(TempMessage,"%swhatsnew.txt",AppPath);
                     ShellExecute(hwnd, "open", TempMessage, NULL, NULL, SW_SHOWNORMAL);           
                     break;
			case IDLOAD:   
                     ZeroMemory(&oifn, sizeof(OPENFILENAME));
                     oifn.lStructSize = sizeof(OPENFILENAME);
                     oifn.hwndOwner = NULL;
                     strcpy(path_buffer,"");
                     oifn.lpstrFile = path_buffer,
                     oifn.nMaxFile = sizeof(path_buffer);
                     oifn.lpstrFilter = ".n64 Files\0*.n64\0.rom Files\0*.rom\0.v64 Files\0*.v64\0.z64 Files\0*.z64\0Nintendo 64 Rom Files\0*.v64;*.rom;*.bin;*.z64;*n64;*.zip;*.usa;*.jap;*.eur\0All files\0*.*\0";
                     oifn.nFilterIndex = 5;
                     oifn.lpstrFileTitle = "";
                     oifn.nMaxFileTitle = 0;
                     oifn.lpstrInitialDir = "";
                     oifn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                     if (GetOpenFileName(&oifn) ) {
                        // before starting the ROM, also add enclosing directory to ROM list directories, because people are lazy
//						if(!emu_launched)
                        {
						    char temp_buffer [_MAX_PATH];
						    strcpy(temp_buffer, path_buffer);
							unsigned int i; for(i = 0 ; i < strlen(temp_buffer) ; i++)
							           if(temp_buffer[i] == '/')
							               temp_buffer[i] = '\\';
	                        char* slash = strrchr(temp_buffer, '\\');
							if(slash) {
								slash[1] = '\0';
	                            if (addDirectoryToLinkedList(temp_buffer)) {
	                            	SendDlgItemMessage(hwnd, IDC_ROMBROWSER_DIR_LIST, LB_ADDSTRING, 0, (LPARAM)temp_buffer);  
			                        AddDirToList(temp_buffer,TRUE);
				                    SaveRomBrowserDirs();
			                        SaveConfig();
		                        }
							}
						}
						
                        StartRom(path_buffer);
                     }
                     break;
                case ID_EMULATOR_EXIT:
                     shut_window = 1;
                     if(emu_launched)
                       exit_emu(0);
                     else
                       exit_emu(1);
                     break; 
                case FULL_SCREEN:
                     if (emu_launched && (!recording || externalReadScreen))
                     {  
                       FullScreenMode=1-FullScreenMode;
                       gui_ChangeWindow();
                     } 
                     break;
                case REFRESH_ROM_BROWSER:
                     if (!emu_launched)
                     {  
                       RefreshRomBrowser();
                     } 
                    break;
                case ID_POPUP_ROM_SETTING:
                      OpenRomProperties();      
                    break;
                case ID_START_ROM:
                      RomList_OpenRom();       
                    break;
                case ID_START_ROM_ENTER:
                      if (!emu_launched) RomList_OpenRom();
                    break;
                case STATE_SAVE:
                    if(!emu_paused) {
                     savestates_job = SAVESTATE;       
                    }
                    else if(emu_launched)
                    {
					 savestates_save();
					}
                    break;                 
                case STATE_SAVEAS:
//                    if(!emu_paused)  
                    {
                     ZeroMemory(&oifn, sizeof(OPENFILENAME));
                     oifn.lStructSize = sizeof(OPENFILENAME);
                     oifn.hwndOwner = NULL;
                     strcpy(path_buffer,"");
                     oifn.lpstrFile = path_buffer,
                     oifn.nMaxFile = sizeof(path_buffer);
                     oifn.lpstrFilter = "Mupen 64 Saves(*.st)\0*.st;*.st?\0All Files\0*.*\0";
                     oifn.lpstrFileTitle = "";
                     oifn.nMaxFileTitle = 0;
                     oifn.lpstrInitialDir = "";
                    if (GetSaveFileName (&oifn)) {
                     savestates_select_filename(path_buffer);
                     savestates_job = SAVESTATE;
                    }                       
                    }
                    break;
                case STATE_RESTORE:
                    if(!emu_paused)  {
                      savestates_job = LOADSTATE;
                    }
                    else if(emu_launched)
                    {
						savestates_load();
					}
                    break;
                case STATE_LOAD:
//                    if(!emu_paused) {
                     ZeroMemory(&oifn, sizeof(OPENFILENAME));
                     oifn.lStructSize = sizeof(OPENFILENAME);
                     oifn.hwndOwner = NULL;
                     strcpy(path_buffer,"");
                     oifn.lpstrFile = path_buffer,
                     oifn.nMaxFile = sizeof(path_buffer);
                     oifn.lpstrFilter = "Mupen 64 Saves(*.st)\0*.st;*.st?\0All Files\0*.*\0";
                     oifn.lpstrFileTitle = "";
                     oifn.nMaxFileTitle = 0;
                     oifn.lpstrInitialDir = "";
                     oifn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                     if (GetOpenFileName(&oifn)) {
                          savestates_select_filename(path_buffer);
                          savestates_job = LOADSTATE;                
                        }
//                     }
                    break;
                case ID_START_RECORD:
                    if(emu_launched)
						OpenMovieRecordDialog();

                   break;
 
                case ID_STOP_RECORD:
                     if (VCR_stopRecord() < 0)
                     	; // fail quietly
//                        MessageBox(NULL, "Couldn't stop recording.", "VCR", MB_OK);
                     else {
                        EnableMenuItem(hMenu,ID_STOP_RECORD,MF_GRAYED);
                        EnableMenuItem(hMenu,ID_START_RECORD,MF_ENABLED);
                        SetStatusTranslatedString(hStatus,0,"Recording stopped");            
                     }
                break;
                case ID_START_PLAYBACK:
					if(emu_launched)
                      OpenMoviePlaybackDialog();
      
                break;
                case ID_STOP_PLAYBACK:
                     if (VCR_stopPlayback() < 0)
                     	; // fail quietly
//                        MessageBox(NULL, "Couldn't stop playback.", "VCR", MB_OK);
                     else {
                        EnableMenuItem(hMenu,ID_STOP_PLAYBACK,MF_GRAYED);
                        EnableMenuItem(hMenu,ID_START_PLAYBACK,MF_ENABLED);
                        SetStatusTranslatedString(hStatus,0,"Playback stopped");
                     }
                break;
                case ID_START_CAPTURE:
                   if(emu_launched) {
						BOOL wasPaused = emu_paused;
						if (emu_launched&&!emu_paused)
							pauseEmu(FALSE) ; 

                        char rec_buffer[MAX_PATH];
                        
                        ZeroMemory(&oifn, sizeof(OPENFILENAME));
                        oifn.lStructSize = sizeof(OPENFILENAME);
                        oifn.hwndOwner = NULL;
                        strcpy(path_buffer,"");
                        oifn.lpstrFile = path_buffer;
                        oifn.nMaxFile = sizeof(path_buffer);
                        oifn.lpstrFilter = "Avi files (*.avi)\0*.avi\0All Files\0*.*\0";
                        oifn.lpstrFileTitle = "";
                        oifn.nMaxFileTitle = 0;
                        oifn.lpstrInitialDir = "";
                        oifn.Flags = OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
                        if (GetSaveFileName(&oifn)) {
                           int len = strlen(path_buffer);
                           if (len < 5 ||
                               (path_buffer[len-1] != 'i' && path_buffer[len-1] != 'I') ||
                               (path_buffer[len-2] != 'v' && path_buffer[len-2] != 'V') ||
                               (path_buffer[len-3] != 'a' && path_buffer[len-3] != 'A') ||
                               path_buffer[len-4] != '.')
                               strcat(path_buffer, ".avi");
                           Sleep(1000);
                           if (VCR_startCapture( rec_buffer, path_buffer ) < 0)
                           {   
                              MessageBox(NULL, "Couldn't start capturing.", "VCR", MB_OK);
                              recording = FALSE;
                           }
                           else {
                              SetWindowPos(mainHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  //Set on top
                              EnableMenuItem(hMenu,ID_START_CAPTURE,MF_GRAYED);
                              EnableMenuItem(hMenu,ID_END_CAPTURE,MF_ENABLED);
                              if(!externalReadScreen)
                              {
                               EnableMenuItem( hMenu, FULL_SCREEN, MF_GRAYED );           //Disables fullscreen menu
                               SendMessage( hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE ); //Disables fullscreen button
                              }
                              SetStatusTranslatedString(hStatus,0,"Recording avi...");
                              recording = TRUE;
                           }
                        }
						if (emu_launched&&emu_paused&&!wasPaused)
							resumeEmu(FALSE);
                    }
                    
                break;
                case ID_END_CAPTURE:
		         Sleep(1000); // HACK - seems to help crash on closing ROM during capture...?
                   if (VCR_stopCapture() < 0)
                        MessageBox(NULL, "Couldn't stop capturing.", "VCR", MB_OK);
                     else {
                        SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        EnableMenuItem(hMenu,ID_END_CAPTURE,MF_GRAYED);
                        EnableMenuItem(hMenu,ID_START_CAPTURE,MF_ENABLED);
                        SetStatusTranslatedString(hStatus,0,"Stopped AVI capture");
                        recording = FALSE;
                     }
                break;    
                case ID_GENERATE_ROM_INFO:
                     generateRomInfo();
                     break; 
                case ID_LANG_INFO_MENU:
                     ret = DialogBox(GetModuleHandle(NULL), 
                           MAKEINTRESOURCE(IDD_LANG_INFO), hwnd, LangInfoProc);
                     break;
                case GENERATE_BITMAP: // take/capture a screenshot
                     
                     if (Config.DefaultScreenshotsDir) { 
                         sprintf(path_buffer,"%sScreenShots\\",AppPath);                           
                         CaptureScreen ( path_buffer);           
                         }
                      else {
                         CaptureScreen ( Config.ScreenshotsDir);                     
                      }   
                     break; 
                case ID_RECENTROMS_RESET:
                     ClearRecentList(hwnd,TRUE);           
                     break;   
                case ID_RECENTROMS_FREEZE: 
                     FreezeRecentRoms( hwnd, TRUE ) ;                                                                      
                     break; 
                case ID_LOG_WINDOW:
                     ShowHideLogWindow();
                     break; 
                case ID_KAILLERA:
                     CreateThread(NULL, 0, KailleraThread, NULL, 0, &SOUNDTHREADID);
                     break;
                case IDC_GUI_TOOLBAR:
                     Config.GuiToolbar = 1 - Config.GuiToolbar;
                     EnableToolbar();           
                     if (Config.GuiToolbar) CheckMenuItem( hMenu, IDC_GUI_TOOLBAR,  MF_BYCOMMAND | MF_CHECKED );
                     else CheckMenuItem( hMenu, IDC_GUI_TOOLBAR,  MF_BYCOMMAND | MF_UNCHECKED );
                     break; 
                 case IDC_GUI_STATUSBAR:
                     Config.GuiStatusbar = 1 - Config.GuiStatusbar;
                     EnableStatusbar();             
                     if (Config.GuiStatusbar) CheckMenuItem( hMenu, IDC_GUI_STATUSBAR,  MF_BYCOMMAND | MF_CHECKED );
                     else CheckMenuItem( hMenu, IDC_GUI_STATUSBAR,  MF_BYCOMMAND | MF_UNCHECKED );
                     break; 
                case IDC_INCREASE_MODIFIER:
                     if (Config.FPSmodifier<50)
                           Config.FPSmodifier = Config.FPSmodifier + 5;
                     else if (Config.FPSmodifier<100)
                           Config.FPSmodifier = Config.FPSmodifier + 10;
                     else if (Config.FPSmodifier<200)
                           Config.FPSmodifier = Config.FPSmodifier + 25;
                     else if (Config.FPSmodifier<1000)
                           Config.FPSmodifier = Config.FPSmodifier + 50;
                     if (Config.FPSmodifier>1000)
                           Config.FPSmodifier = 1000;
                     InitTimer();
                     break;
                case IDC_DECREASE_MODIFIER:
                     if (Config.FPSmodifier>200)
                           Config.FPSmodifier = Config.FPSmodifier - 50;
                     else if (Config.FPSmodifier>100)
                           Config.FPSmodifier = Config.FPSmodifier - 25;
                     else if (Config.FPSmodifier>50)
                           Config.FPSmodifier = Config.FPSmodifier - 10;
                     else if (Config.FPSmodifier>5)
                           Config.FPSmodifier = Config.FPSmodifier - 5;
                     if (Config.FPSmodifier<5)
                           Config.FPSmodifier = 5;
                     InitTimer();
                     break;
                case IDC_RESET_MODIFIER:
                     Config.FPSmodifier = 100;
                     InitTimer();
                     break;                                                 
								case ID_LUA_RELOAD:
#ifdef LUA_CONSOLE
									LuaReload();
#endif
									break;
                default :
                     //Language Support  from ID_LANG_ENGLISH to ID_LANG_ENGLISH+100
                     if (LOWORD(wParam) >= ID_LANG_ENGLISH && LOWORD(wParam) <= (ID_LANG_ENGLISH + 100)) {
		                SelectLang(hwnd,LOWORD(wParam));		     
                        TranslateMenu(GetMenu(hwnd),hwnd);
                        TranslateBrowserHeader(hRomList);
                        ShowTotalRoms();
                     }
                     else if (LOWORD(wParam) >= ID_CURRENTSAVE_1 && LOWORD(wParam) <= ID_CURRENTSAVE_9) {
		                SelectState(hwnd,LOWORD(wParam));		     
                     }
                     else if (LOWORD(wParam) >= ID_SAVE_1 && LOWORD(wParam) <= ID_SAVE_9) {
		                SaveTheState(hwnd,LOWORD(wParam));		     
                     }
                     else if (LOWORD(wParam) >= ID_LOAD_1 && LOWORD(wParam) <= ID_LOAD_9) {
		                LoadTheState(hwnd,LOWORD(wParam));		     
                     }
                     else if (LOWORD(wParam) >= ID_RECENTROMS_FIRST && LOWORD(wParam) < (ID_RECENTROMS_FIRST + MAX_RECENT_ROMS))  {
                          RunRecentRom(LOWORD(wParam));              
                     }
			         break;    
            }
       }    
			 break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	
	return TRUE;	
}

int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	FILE *newfp;
	newfp = freopen("stdout.txt", "w", stdout);
	if ( newfp == NULL )
	{
		newfp = fopen("stdout.txt", "w");
		if ( newfp )
		{
 			*stdout = *newfp;
 		}
	}
	
	newfp = freopen("stderr.txt", "w", stderr);
	if ( newfp == NULL )
	{
		newfp = fopen("stderr.txt", "w");
		if ( newfp )
		{
 			*stderr = *newfp;
 		}
	}
	
	/* Put absolute App path to AppPath variable */  
	getAppFullPath( AppPath );
  app_hInstance = hInstance;
  InitCommonControls(); 
  SaveCmdLineParameter(lpCmdLine);
  ini_openFile();
    
  // ensure folders exist!
  {
		String path = AppPath;
		CreateDirectory((path + "save").c_str(), NULL);
		CreateDirectory((path + "Mempaks").c_str(), NULL);
		CreateDirectory((path + "Lang").c_str(), NULL);
		CreateDirectory((path + "ScreenShots").c_str(), NULL);
		CreateDirectory((path + "plugin").c_str(), NULL);
	}
           
  emu_launched = 0;
  emu_paused = 1;
  /************    Loading Config  *******/
	LoadConfig() ;
  /************************************************************************/

	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;
	HACCEL Accel;

	if (GuiDisabled())
	{
    wc.cbSize		 = sizeof(WNDCLASSEX);
		wc.style		 = 0;
		wc.lpfnWndProc	 = NoGuiWndProc;
		wc.cbClsExtra	 = 0;
		wc.cbWndExtra	 = 0;
		wc.hInstance	 = hInstance;
		wc.hIcon		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONBIG));
		wc.hIconSm		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONSMALL));
		wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = g_szClassName;
 
    if(!RegisterClassEx(&wc))
	  {
			MessageBox(NULL, "Window Registration Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
	  }
		
		hwnd = CreateWindowEx(
			0 ,
			g_szClassName,
			MUPEN_VERSION,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			Config.WindowPosX, Config.WindowPosY, Config.WindowWidth, Config.WindowHeight,
			NULL, NULL, hInstance, NULL);
        
		GUI_CreateLogWindow(hwnd);
		mainHWND = hwnd ;
		ShowWindow(hwnd, nCmdShow);
		UpdateWindow(hwnd);
     
		StartGameByCommandLine();
		
		ShowInfo("Mupen64 - Nintendo 64 emulator - Guiless mode");
        
		while(GetMessage(&Msg, NULL, 0, 0) > 0)
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	else
	{
	//window initialize
#if 1
		wc.cbSize		 = sizeof(WNDCLASSEX);
		wc.style		 = 0;
		wc.lpfnWndProc	 = WndProc;
		wc.cbClsExtra	 = 0;
		wc.cbWndExtra	 = 0;
		wc.hInstance	 = hInstance;
		wc.hIcon		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONBIG));
		wc.hIconSm		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONSMALL));
		wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+11);
		wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU);
		wc.lpszClassName = g_szClassName;
	
		if(!RegisterClassEx(&wc))
		{
			MessageBox(NULL, "Window Registration Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}
	
		Accel = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_ACCEL));
    
		hwnd = CreateWindowEx(
            WS_EX_ACCEPTFILES,
			g_szClassName,
			MUPEN_VERSION,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_EX_TOPMOST,
			Config.WindowPosX, Config.WindowPosY, Config.WindowWidth, Config.WindowHeight,
			NULL, NULL, hInstance, NULL);
	
		if(hwnd == NULL)
		{
			MessageBox(NULL, "Window Creation Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}

		mainHWND = hwnd ;
		ShowWindow(hwnd, nCmdShow);
		UpdateWindow(hwnd);
#endif

		GUI_CreateLogWindow(hwnd);
		if (!extLogger)
		{
			//DeleteMenu( GetMenu(hwnd), ID_LOG_WINDOW, MF_BYCOMMAND);
			EnableMenuItem(GetMenu(hwnd), ID_LOG_WINDOW, MF_GRAYED);
		}
    
		if (!isKailleraExist())
		{
			DeleteMenu( GetMenu(hwnd), ID_KAILLERA, MF_BYCOMMAND);
		}   
    
		SetupDummyInfo(); 
    
		EnableEmulationMenuItems( 0 );
		if (!StartGameByCommandLine())
		{
			cmdlineMode = 0;
		}

		ShowInfo("Mupen64 - Nintendo 64 emulator - re-recording v8 - GUI mode");
		SetStatusTranslatedString( hStatus, 0, "Mupen64 - Nintendo 64 emulator - re-recording v8" );
    
		while(GetMessage(&Msg, NULL, 0, 0) > 0)
		{
			if (!TranslateAccelerator(mainHWND,Accel,&Msg)
#ifdef LUA_CONSOLE
			&& !::IsLuaConsoleMessage(&Msg)
#endif
			)
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
	
			// modifier-only checks, cannot be obtained through windows messaging...
			int i;
			for(i = 0 ; i < NUM_HOTKEYS ; i++)
			{
				if(!Config.hotkey[i].key && (Config.hotkey[i].shift || Config.hotkey[i].ctrl || Config.hotkey[i].alt))
				{
					if(i != 0)
					{
						if(((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.hotkey[i].shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.hotkey[i].ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.hotkey[i].alt)
						{
							SendMessage(hwnd, WM_COMMAND, Config.hotkey[i].command, 0);
						}
					}
					else // fast-forward 
					{
						if(((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.hotkey[i].shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.hotkey[i].ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.hotkey[i].alt)
						{
							manualFPSLimit = 0 ; 
						}
						else
						{
							manualFPSLimit = 1 ; 
						}
					}
				}
			}
		}
	}
        
	fclose(newfp);
	CloseLogWindow();
	CloseKaillera();
	return Msg.wParam;
}
