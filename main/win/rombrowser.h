/***************************************************************************
                          rombrowser.h  -  description
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

#ifndef ROMBROWSER_H
#define ROMBROWSER_H

#include <windows.h>
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif
#include <commctrl.h>

#include "../md5.h"

typedef struct {
	char     szFullFileName[MAX_PATH];
	char     Status[60];
	char     FileName[200];
	char     InternalName[22];
	char     GoodName[200];
	char     CartID[3];
	char     PluginNotes[250];
	char     CoreNotes[250];
	char     UserNotes[250];
	char     Developer[30];
	char     ReleaseDate[30];
	char     Genre[15];
	int      RomSize;
	BYTE     Manufacturer;
	BYTE     Country;
	DWORD    CRC1;
	DWORD    CRC2;
	char     MD5[33];
} ROM_INFO;

typedef struct {
	BYTE     Country;
	DWORD    CRC1;
	DWORD    CRC2;
	long     Fpos;
} ROM_LIST_INFO;

typedef struct {
	int    ListCount;
	int    ListAlloc;
	ROM_LIST_INFO * List;
} ROM_LIST;

typedef struct {
	int    ListCount;
	int    ListAlloc;
	ROM_INFO * List;
} ITEM_LIST;


void AddRomToList (char * RomLocation) ;
void RomList_GetDispInfo(LPNMHDR pnmh) ;
void OpenRomProperties();
int RomList_OpenRom() ;
void RomListNotify(LPNMHDR pnmh) ;
void ResetRomBrowserColomuns (void);
void LoadRomList();
BOOL LoadRomBrowserCache();
void SaveRomBrowserCache();
void saveMD5toCache(char md5str[33]);
void CreateRomListControl (HWND hParent) ;
void ShowRomBrowser(BOOL flag, BOOL showRom);
void RefreshRomBrowser (void);
void AddDirToList(char RomBrowserDir[MAX_PATH],BOOL sortflag);
void RomList_ColoumnSortList(LPNMLISTVIEW pnmv);
int CALLBACK RomList_CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
void ResizeRomListControl ();
void SaveRomBrowserDirs();
void MD5toString(md5_byte_t digest[16],char *mes);
void FastRefreshBrowser();
void ListViewSort();

BOOL addDirectoryToLinkedList(char Dir[MAX_PATH]); 
void removeDirectoryFromLinkedList(char Dir[MAX_PATH]) ;

void FillRomBrowserDirBox(HWND hwnd);
void TranslateBrowserHeader(HWND hwnd);

void freeRomDirList();
void freeRomList();
char *getFieldName(int col);
void ShowTotalRoms();
void drawSortArrow(int SubItem);
int isFieldInBrowser(int index);

ROM_INFO *getSelectedRom();

extern HWND romInfoHWND ;
extern ITEM_LIST ItemList;

#define ROM_COLUMN_FIELDS 7
#define MAX_RECENT_ROMS 10
void SetRealColumnArray();
extern int RealColumn[ROM_COLUMN_FIELDS] ;


////////////////////  Recent Roms Functions ///////////////////
char* ParseName();
void SetRecentList(HWND hwnd);
void ClearRecentList (HWND hwnd,BOOL clear_array);
void AddToRecentList(HWND hwnd,char *rompath);
void RunRecentRom(int i);
void DisableRecentRoms(HMENU hMenu,BOOL disable);
void FreezeRecentRoms(HWND hWnd, BOOL ChangeConfigVariable);

#endif

