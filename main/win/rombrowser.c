/***************************************************************************
                          rombrowser.c  -  description
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "rombrowser.h"
#include "main_win.h"
#include "../../winproject/resource.h"
#include "../rom.h"
#include "../md5.h"
#include <zlib.h>
#include "inifunctions.h"
#include "../guifuncs.h"
#include "../plugin.h"
#include "configdialog.h"
#include "RomSettings.h"
#include "translation.h"
#include "Config.h"

static int TOTAL_ROMS_NUMBER = 0;
static BOOL SortAscending ;
static int ROM_SIZE = 0;
static int m_CurrentSortItem = 0 ;
int calculateGlobalStop ;
HWND romInfoHWND;
ITEM_LIST ItemList = {0,0,NULL};

TCHAR RomBrowserFields[ROM_COLUMN_FIELDS][30] = {TEXT("Good Name"), 
                                     TEXT("Internal Name"),
                                     TEXT("Country"),
                                     TEXT("Size"),
                                     TEXT("Comments"), 
                                     TEXT("File Name"),
                                     TEXT("MD5")
                                     };
int RomBrowserFieldsWidth[ROM_COLUMN_FIELDS] = {250,150,70,70,200,100,100};  

int RealColumn[ROM_COLUMN_FIELDS] = {0,2,3,4,-1,-1,-1};

char *getFieldName(int col)
{
    return RomBrowserFields[col];
}                                   

int isFieldInBrowser(int index)
{
    switch (index)
    {
        case 0:
               return Config.Column_GoodName ; 
        break;
        case 1:
               return Config.Column_InternalName ;
        break;
        case 2:
               return Config.Column_Country ;
        break;
        case 3:
               return Config.Column_Size;
        break;
        case 4:
               return Config.Column_Comments;
        break;
        case 5:
               return Config.Column_FileName ;
        break;
        case 6:
               return Config.Column_MD5;
        break;
        default:
        return 0;
    }
}

void SetRealColumnArray()
{
    int i, index;
    index = 0;
    
    for (i=0;i<ROM_COLUMN_FIELDS;i++) {
        if (isFieldInBrowser(i)) {
                RealColumn[index] = i ;
                index ++; 
                }
        else {
                RealColumn[i] = -1 ;
        }
    }
}
                                     
ROM_LIST RDBList = {0,0,NULL}; 

typedef struct rom_directory_list{
    char RomDirectory[MAX_PATH];
    struct rom_directory_list *next;
} ROM_DIRECTORY_LIST, *ROM_DIRECTORY_PTR;

ROM_DIRECTORY_PTR ROM_DIRECTORY_HEADER;

char *get_cachepath()
{
   static char *cachepath = NULL;
   if (cachepath == NULL)
     {
       	cachepath = (char*)malloc(strlen(get_currentpath())+1+strlen("mupen64.cache"));
	    strcpy(cachepath, get_currentpath());
    	strcat(cachepath, "mupen64.cache");
     }
   return cachepath;
}

char *get_dirfilepath()
{
    static char *cachepath = NULL;
    if (cachepath == NULL)
     {
       	cachepath = (char*)malloc(strlen(get_currentpath())+1+strlen("mupen64.cch"));
	    strcpy(cachepath, get_currentpath());
    	strcat(cachepath, "mupen64.cch");
     }
   return cachepath;
}

void FillRomBrowserDirBox(HWND hwnd) {
    char RomDirectory[MAX_PATH];
    ROM_DIRECTORY_PTR tmp;
    tmp = ROM_DIRECTORY_HEADER ;
     while (tmp) {
       sprintf(RomDirectory,"%s",tmp->RomDirectory);
       SendDlgItemMessage(hwnd, IDC_ROMBROWSER_DIR_LIST, LB_ADDSTRING, 0, (LPARAM)RomDirectory);
       tmp=tmp->next ;    
     }
}

void freeRomDirList() {
    ROM_DIRECTORY_PTR temp ;
    while (ROM_DIRECTORY_HEADER!=NULL) {
        temp = ROM_DIRECTORY_HEADER->next;
        free(ROM_DIRECTORY_HEADER);
        ROM_DIRECTORY_HEADER = temp;
    }
}

void freeRomList() {
    if (ItemList.ListAlloc != 0) {
		free(ItemList.List);
		ItemList.ListAlloc = 0;
		ItemList.ListCount = 0;
		ItemList.List = NULL;		
	}
}

BOOL addDirectoryToLinkedList(char Dir[MAX_PATH]) {
   ROM_DIRECTORY_PTR temp,newptr ;
   temp = ROM_DIRECTORY_HEADER ;
   if (temp) {
    if (lstrcmpi(temp->RomDirectory,Dir)==0) return FALSE;
   while(temp->next) {
    temp = temp->next;
    if (lstrcmpi(temp->RomDirectory,Dir)==0) return FALSE;
   }
   newptr = (ROM_DIRECTORY_LIST*)malloc(sizeof(ROM_DIRECTORY_LIST));
   temp->next = newptr;
   sprintf(newptr->RomDirectory,"%s",Dir);
   newptr->next=NULL;
   }
   else {
   ROM_DIRECTORY_HEADER = (ROM_DIRECTORY_LIST*)malloc(sizeof(ROM_DIRECTORY_LIST));
   sprintf(ROM_DIRECTORY_HEADER->RomDirectory,"%s",Dir);
   ROM_DIRECTORY_HEADER->next=NULL;
   }
   return TRUE;
}

void removeDirectoryFromLinkedList(char Dir[MAX_PATH]) {
    ROM_DIRECTORY_PTR temp,prev=NULL;
    temp = ROM_DIRECTORY_HEADER ;
    while (temp) {
         if(lstrcmpi(temp->RomDirectory,Dir)==0) {
              if (prev==NULL) {
                 temp = ROM_DIRECTORY_HEADER;
                 ROM_DIRECTORY_HEADER = temp->next;
                 free(temp);        
              }
              else {
                 prev->next=temp->next;
                 free(temp);
              }
              return;
         }
         else
         {
              prev=temp;
              temp=temp->next;  
         }
    }
    
 //   ShowMessage("Directory not in list!");
}

ROM_DIRECTORY_PTR LoadRomBrowserDirs() {
    char RomBrowserDir[MAX_PATH];
    ROM_DIRECTORY_PTR head,tmp,prev ;
    gzFile cacheFile;
    head=NULL;
        
    cacheFile = gzopen(get_dirfilepath(), "rb");
    if (cacheFile) {
    gzgets(cacheFile, RomBrowserDir, MAX_PATH);
    RomBrowserDir[strlen(RomBrowserDir)-1]='\0';
    if (strcmp(RomBrowserDir, "")) {
            head = (ROM_DIRECTORY_LIST*)malloc(sizeof(ROM_DIRECTORY_LIST));
            sprintf(head->RomDirectory,"%s",RomBrowserDir);
            head->next=NULL;
            }
    prev=head;
    gzgets(cacheFile, RomBrowserDir, MAX_PATH);
    while(strcmp(RomBrowserDir, "")) {
        RomBrowserDir[strlen(RomBrowserDir)-1]='\0';
        tmp = (ROM_DIRECTORY_LIST*)malloc(sizeof(ROM_DIRECTORY_LIST));
        if (tmp) {
                   sprintf(tmp->RomDirectory,"%s",RomBrowserDir);
                   tmp->next = NULL;
                   prev->next = tmp;
                   prev=tmp;
                   gzgets(cacheFile, RomBrowserDir, MAX_PATH);
         }
         else {
            ShowMessage("Not enough Memory");
         }     
     }
     gzclose(cacheFile);
    }
   return head;
}

void SaveRomBrowserCache()
{
   gzFile cacheFile;
   cacheFile = gzopen (get_cachepath(), "wb");
   if (cacheFile) {
   gzwrite(cacheFile,&ItemList.ListCount,sizeof(ItemList.ListCount));
   gzwrite(cacheFile,ItemList.List,sizeof(ROM_INFO)*ItemList.ListCount);
   gzclose(cacheFile);
   }
}

BOOL LoadRomBrowserCache()
{
   gzFile cacheFile;
   ROM_INFO * pRomInfo;
   int i;
   LV_ITEM  lvItem;
   cacheFile = gzopen (get_cachepath(), "rb");
   if (cacheFile) {
       freeRomList();
	   gzread(cacheFile,&ItemList.ListCount,sizeof(ItemList.ListCount));  
       ItemList.List = (ROM_INFO *)malloc(sizeof(ROM_INFO) * ItemList.ListCount);
   	   ItemList.ListAlloc = ItemList.ListCount;
   	   gzread(cacheFile,ItemList.List,sizeof(ROM_INFO) * ItemList.ListCount); 
       gzclose(cacheFile);
       ListView_DeleteAllItems(hRomList);
       
       for (i=0;i<ItemList.ListCount;i++)
       {
        pRomInfo = &ItemList.List[i];
        memset(&lvItem, 0, sizeof(lvItem));
        switch( pRomInfo->Country & 0xFF)
	   {
	 	   case 0:
		  lvItem.iImage = 9;   
		   break;

        	case '7':
		 lvItem.iImage = 10;  
		   break;
           case 0x44:
            lvItem.iImage = 0;              // IDI_GERMANY
           break;
           case 0x45:
            lvItem.iImage = 1;              // IDI_USA
           break;
           case 0x4A:
            lvItem.iImage = 2;              // IDI_JAPAN 
           break;
           case 0x20:
           case 0x21:
	       case 0x38:
	 	   case 0x70:
           case 0x50:
           case 0x58:
            lvItem.iImage = 3;              // IDI_EUROPE
           break;
           case 0x55:
            lvItem.iImage = 4;              // IDI_AUSTRALIA
           break;
           case 'I':
	        lvItem.iImage = 5;             // Italy 
           break;
           case 0x46:                       // IDI_FRANCE
	        lvItem.iImage = 6; 
		   break;
	       case 'S':                        //SPAIN
            lvItem.iImage = 7;
           break;   	
           default : 
            lvItem.iImage = 8;              // IDI_N64CART 
           break;
         }  
             lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;		
	         lvItem.iItem = ListView_GetItemCount(hRomList);
	         lvItem.lParam = (LPARAM)i;
	         lvItem.pszText = LPSTR_TEXTCALLBACK;
	         ListView_InsertItem(hRomList, &lvItem);
       	     TOTAL_ROMS_NUMBER = TOTAL_ROMS_NUMBER + 1;
             ShowTotalRoms();    
       }
   return TRUE;
   } 
   return FALSE;
}

void SaveRomBrowserDirs() {
   ROM_DIRECTORY_PTR RomDirsHeader;
   gzFile cacheFile;
   RomDirsHeader = ROM_DIRECTORY_HEADER;
   cacheFile = gzopen (get_dirfilepath(), "wb");
   if (cacheFile) {
   while (RomDirsHeader) {
     gzprintf(cacheFile, "%s\n",RomDirsHeader->RomDirectory);
     RomDirsHeader = RomDirsHeader->next ;
   }
   //gzprintf(cacheFile, "%sDIRS_END\n");
   gzclose(cacheFile);
   }
}

void CountryCodeToCountryName(int countrycode,char *countryname)
{  //Thuis function is from 1964 sources,Thanks to Schibo for allowing me to use it
   //SLightly changed for internal use
	switch(countrycode&0xFF)
	{
	/* Demo */
	case 0:
		strcpy(countryname, "Demo");
		break;

	case '7':
		strcpy(countryname, "Beta");
		break;

	case 0x41:
		strcpy(countryname, "USA/Japan");
		break;

	/* Germany */
	case 0x44:
		strcpy(countryname, "German");
		break;

	/* USA */
	case 0x45:
		strcpy(countryname, "USA");
		break;

	/* France */
	case 0x46:
		strcpy(countryname, "France");
		break;

	/* Italy */
	case 'I':
		strcpy(countryname, "Italy");
		break;

	/* Japan */
	case 0x4A:
		strcpy(countryname, "Japan");
		break;

	case 'S':	/* Spain */
		strcpy(countryname, "Spain");
		break;

	/* Australia */
	case 0x55:
	case 0x59:
		strcpy(countryname, "Australia");
		break;

    case 0x50:
    case 0x58:
	case 0x20:
	case 0x21:
	case 0x38:
	case 0x70:
		sprintf(countryname, "Europe");
		break;

	default:
		sprintf(countryname, "Unknown (%d)", (int)(countrycode&0xFF));
		break;
	}
	TranslateDefault(countryname,countryname,countryname);
//	sprintf(countryname,"%s (0x%X)",countryname,countrycode);
}

BOOL romInList(char *RomLocation)
{
    ROM_INFO *pRomInfo;
	int i;
	if (!Config.RomBrowserRecursion) return FALSE;
    for (i=0;i<ItemList.ListCount;i++)
    {
        pRomInfo = &ItemList.List[i];
        if (lstrcmpi(pRomInfo->szFullFileName,RomLocation)==0) {
                        return TRUE;
                }
    }
    return FALSE;
}

void AddRomToList (char * RomLocation) {
	LV_ITEM  lvItem;
	ROM_INFO * pRomInfo;
	int index;
    if  (romInList(RomLocation)) return;
	if (ItemList.ListAlloc == 0) {
		ItemList.List = (ROM_INFO *)malloc(100 * sizeof(ROM_INFO));
		ItemList.ListAlloc = 100;
	} else if (ItemList.ListAlloc == ItemList.ListCount) {
		ItemList.ListAlloc += 100;
		ItemList.List = (ROM_INFO *)realloc(ItemList.List, ItemList.ListAlloc * sizeof(ROM_INFO));
		if (ItemList.List == NULL) {
			ShowMessage("Failed");
			ExitThread(0);
		}
	}
	pRomInfo = &ItemList.List[ItemList.ListCount];
	if (pRomInfo == NULL) { return; }

	memset(pRomInfo, 0, sizeof(ROM_INFO));	
	memset(&lvItem, 0, sizeof(lvItem));
//Filling rombrowser info
	strncpy(pRomInfo->szFullFileName, RomLocation, MAX_PATH);
    strncpy(pRomInfo->InternalName, (const char*)ROM_HEADER->nom, sizeof(ROM_HEADER->nom));
    pRomInfo->Country = ROM_HEADER->Country_code;
    pRomInfo->RomSize = ROM_SIZE;
    pRomInfo->CRC1 = sl(ROM_HEADER->CRC1);
    pRomInfo->CRC2 = sl(ROM_HEADER->CRC2);
    getIniGoodName(ROM_HEADER,TempMessage) ;
    strncpy(pRomInfo->GoodName,TempMessage,sizeof(TempMessage));
    getIniComments(pRomInfo,TempMessage);
    strncpy(pRomInfo->UserNotes,TempMessage,sizeof(pRomInfo->UserNotes));
   switch( pRomInfo->Country&0xFF )
	{
    case 0:
		lvItem.iImage = 9;   
		break;

	case '7':
		lvItem.iImage = 10;  
		break;
     case 0x44:
      lvItem.iImage = 0;              // IDI_GERMANY
      break;
     case 0x45:
      lvItem.iImage = 1;              // IDI_USA
      break;
     case 0x4A:
      lvItem.iImage = 2;              // IDI_JAPAN 
      break;
     case 0x20:
	 case 0x21:
	 case 0x38:
	 case 0x70:
     case 0x50:
     case 0x58:
       lvItem.iImage = 3;              // IDI_EUROPE
      break;
     case 0x55:
      lvItem.iImage = 4;              // IDI_AUSTRALIA
      break;
     case 'I':
	   lvItem.iImage = 5;             // Italy 
        break;
     case 0x46:                       // IDI_FRANCE
	   lvItem.iImage = 6; 
		break;
	 case 'S':                        //SPAIN
       lvItem.iImage = 7;
       break;   	
     default : 
      lvItem.iImage = 8;              // IDI_N64CART 
      break;
    }  
    lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;		
	lvItem.iItem = ListView_GetItemCount(hRomList);
	lvItem.lParam = (LPARAM)ItemList.ListCount;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	ItemList.ListCount += 1;
	index = ListView_InsertItem(hRomList, &lvItem);
    TOTAL_ROMS_NUMBER = TOTAL_ROMS_NUMBER + 1;
    ShowTotalRoms();	
}

void RomList_GetDispInfo(LPNMHDR pnmh) {
    char country[50];
	LV_DISPINFO * lpdi = (LV_DISPINFO *)pnmh;
	ROM_INFO * pRomInfo = &ItemList.List[lpdi->item.lParam];
	
	switch( RealColumn[lpdi->item.iSubItem]) {
	case 0:
	 if (lstrcmpi(pRomInfo->GoodName,"")!=0)
	  {
	     strncpy(lpdi->item.pszText, pRomInfo->GoodName, lpdi->item.cchTextMax); 
      }
	 else
	  {
	     strncpy(lpdi->item.pszText, pRomInfo->InternalName, lpdi->item.cchTextMax); 
	  }   
    break;
    
    case 1:
       strncpy(lpdi->item.pszText, pRomInfo->InternalName, lpdi->item.cchTextMax); 
    break;
    
    case 2:
      CountryCodeToCountryName(pRomInfo->Country,country);
      strncpy(lpdi->item.pszText,country, lpdi->item.cchTextMax); 
    break;
    
    case 3:
      sprintf(lpdi->item.pszText,"%.1f MBit",(float)pRomInfo->RomSize/0x20000); 
    break; 
    
    case 4:
      strncpy(lpdi->item.pszText, pRomInfo->UserNotes, lpdi->item.cchTextMax); 
    break;    
    
    case 5:
      strncpy(lpdi->item.pszText, pRomInfo->szFullFileName, lpdi->item.cchTextMax); 
    break;
    
    case 6:
      strncpy(lpdi->item.pszText, pRomInfo->MD5, lpdi->item.cchTextMax); 
    break;       
   }
}

void MD5toString(md5_byte_t digest[16],char *mes) {
    char localMessage[3];
    int i;
    sprintf(mes,"%s", "");
    for (i=0; i<16; i++) {
      sprintf(localMessage,"%02X",digest[i]);
      strcat(mes, localMessage);
    } 
}

LRESULT CALLBACK RomPropertiesProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    char country [50];
    md5_byte_t digest[16];
    ROM_INFO *pRomInfo;
    char tempname[100];
    HWND hwndPB;    
    pRomInfo = getSelectedRom();
    DEFAULT_ROM_SETTINGS TempRomSettings;
    
    if (pRomInfo==NULL) { 
       EndDialog(hwnd, IDOK);
       return FALSE; 
      }
    switch(Message)
    {
        case WM_INITDIALOG:
             romInfoHWND = hwnd;
             
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
             TranslateRomInfoDialog(hwnd);
             return FALSE;
        case WM_CLOSE:
              EndDialog(hwnd, IDOK);
              romInfoHWND = NULL;   
        break; 
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_OK:
                    GetDlgItemText(hwnd, IDC_INI_COMMENTS, (LPSTR) TempMessage, 128 );
                    setIniComments(pRomInfo,TempMessage);
                    strncpy(pRomInfo->UserNotes,TempMessage,sizeof(pRomInfo->UserNotes));
                    if (!emu_launched) {                    //Refreshes the ROM Browser
                        ShowWindow( hRomList, FALSE ); 
                        ShowWindow( hRomList, TRUE );
                    }
                    EndDialog(hwnd, IDOK);
                    romInfoHWND = NULL; 
                break;
                case IDC_CANCEL:
                    if (!emu_launched) {
                        ShowWindow( hRomList, FALSE ); 
                        ShowWindow( hRomList, TRUE );
                    }
                    EndDialog(hwnd, IDOK);
                    romInfoHWND = NULL;
                break;
                case IDC_MD5_CALCULATE:
                    hwndPB = GetDlgItem( hwnd, IDC_MD5_PROGRESS_BAR );
                    SendMessage( hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, 100) ); 
//                    ShowWindow( GetDlgItem(hwnd, IDC_MD5_CALCULATE), FALSE );  //Hides "Calculate" Button
                    ShowWindow( hwndPB, TRUE );                                //Shows Progress bar
                                        
                    calculateGlobalStop = 0;
                    calculateMD5(pRomInfo->szFullFileName,digest);
                    MD5toString(digest,TempMessage);
                    SetDlgItemText(hwnd, IDC_ROM_MD5, TempMessage);
                    if(getIniGoodNameByMD5(TempMessage,tempname))
	                    SetDlgItemText(hwnd, IDC_ROM_GOODNAME, tempname);
	                else
	                    SetDlgItemText(hwnd, IDC_ROM_GOODNAME, "(goodname not found in mupen64.ini)");
                    saveMD5toCache(TempMessage);
 
                    ShowWindow( hwndPB, FALSE );                    //Hides Progress bar
                    ShowWindow( GetDlgItem(hwnd, IDC_MD5_CALCULATE), TRUE );  //Shows "Calculate" Button
                break;
                case IDC_SAVE_PROFILE:
                    ReadComboBoxValue( hwnd, IDC_COMBO_GFX,   TempRomSettings.GfxPluginName) ;
                    ReadComboBoxValue( hwnd, IDC_COMBO_INPUT, TempRomSettings.InputPluginName) ;
                    ReadComboBoxValue( hwnd, IDC_COMBO_SOUND, TempRomSettings.SoundPluginName) ;
                    ReadComboBoxValue( hwnd, IDC_COMBO_RSP,   TempRomSettings.RspPluginName) ; 
                    saveDefaultRomSettings( pRomInfo->InternalName, TempRomSettings);         
                break;
            }
        break;
    }
    return FALSE;
}

void OpenRomProperties()
{
    DialogBox(GetModuleHandle(NULL), 
                     MAKEINTRESOURCE(IDD_ROM_SETTING_DIALOG), mainHWND, (DLGPROC)RomPropertiesProc);
}

int RomList_OpenRom()
{
    ROM_INFO * pRomInfo;
    pRomInfo = getSelectedRom();
    if (pRomInfo==NULL) return 0;
    
    StartRom(pRomInfo->szFullFileName);
    return 1;
}

void RomList_RightClick(LPNMHDR pnmh) {
    			RECT pos;
    			ROM_INFO *pRomInfo;
				HMENU popupmainmenu, popupmenu;
				int x = ((LPNMLISTVIEW) pnmh)->ptAction.x;
				int y = ((LPNMLISTVIEW) pnmh)->ptAction.y;
								
				GetWindowRect(hRomList, &pos);
				x+=pos.left;
				y+=pos.top;
				popupmainmenu = LoadMenu(app_hInstance, MAKEINTRESOURCE(ROM_POPUP_MENU));
				popupmenu = GetSubMenu(popupmainmenu,0);
				TranslateRomBrowserMenu(popupmenu);
				pRomInfo = getSelectedRom();
				if (pRomInfo==NULL) {
				   EnableMenuItem(popupmenu,ID_START_ROM,MF_GRAYED);
                   EnableMenuItem(popupmenu,ID_POPUP_ROM_SETTING,MF_GRAYED);  
				}
				
                TrackPopupMenuEx(popupmenu, TPM_VERTICAL, x, y, mainHWND, NULL);
                DestroyMenu(popupmenu);
}

void RomListNotify(LPNMHDR pnmh) {
	
    switch (pnmh->code) {
	case LVN_GETDISPINFO: RomList_GetDispInfo(pnmh); break;
	case NM_DBLCLK:       RomList_OpenRom(); break;
	case LVN_COLUMNCLICK: RomList_ColoumnSortList((LPNMLISTVIEW)pnmh); break;
	case NM_RCLICK:       RomList_RightClick(pnmh); break;
	case NM_CLICK:        getSelectedRom(); break;
	}
}

void ResetRomBrowserColomuns (void) {
	int Column, index;
	LV_COLUMN lvColumn;
	char szString[300];
	BOOL Deleted=TRUE;
    
    for (Column = 0; Column < ROM_COLUMN_FIELDS; Column ++) {
        RomBrowserFieldsWidth[Column] = ReadCfgInt("Rom Browser",RomBrowserFields[Column],RomBrowserFieldsWidth[Column]);
    }        
    memset(&lvColumn,0,sizeof(lvColumn));
	
    
    
    while(Deleted==TRUE) {
        Deleted=ListView_DeleteColumn( hRomList, 0);
    }
    
	//Add Colomuns
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = szString;
	
	index = 0;
	for (Column = 0; Column < ROM_COLUMN_FIELDS; Column ++) {
	    if (isFieldInBrowser(Column)) {
			    lvColumn.iSubItem = Column;
                lvColumn.cx = RomBrowserFieldsWidth[Column];
                TranslateDefault(RomBrowserFields[Column],RomBrowserFields[Column],TempMessage);
	            strncpy(szString, TempMessage, sizeof(szString));
		        ListView_InsertColumn(hRomList, index, &lvColumn);
		        index++;
		}
	}
}

void TranslateBrowserHeader(HWND hwnd)
{
    int Column, index;
	LV_COLUMN lvColumn;
	char szString[300];
       
    memset(&lvColumn,0,sizeof(lvColumn));
    index = 0;
	//Add Colomuns
	lvColumn.mask = LVCF_TEXT ;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = szString;

	for (Column = 0; Column < ROM_COLUMN_FIELDS; Column ++) {
		 if (isFieldInBrowser(Column)) {
              lvColumn.iSubItem = Column;
              Translate(RomBrowserFields[Column],TempMessage);
		      strncpy(szString, TempMessage, sizeof(szString));
		      ListView_SetColumn(hRomList, index, &lvColumn);
		      index++;
		      }
	}
}

int getSortColumn()
{
   return  Config.RomBrowserSortColumn ; 
}

void ListViewSort()
{
    SortAscending = strcmp(Config.RomBrowserSortMethod,"ASC")?FALSE:TRUE;
    drawSortArrow(getSortColumn());
    ListView_SortItems(hRomList, RomList_CompareItems, getSortColumn());  
}

void LoadRomList()
{
    ROM_DIRECTORY_PTR RomDirsHeader;
    SetRealColumnArray();
    ROM_DIRECTORY_HEADER = LoadRomBrowserDirs();
   //  ROM_DIRECTORY_HEADER = NULL;
    if (!LoadRomBrowserCache()) {
    RomDirsHeader = ROM_DIRECTORY_HEADER ; 
    while (RomDirsHeader) {
       AddDirToList(RomDirsHeader->RomDirectory,FALSE);
       RomDirsHeader = RomDirsHeader->next ;
      }
    }
    ListViewSort();
}

void ShowTotalRoms()
{
  if (!emu_launched)
  {
     Translate("Total ROMs",TempMessage);
     sprintf(TempMessage,"%s: %d",TempMessage,TOTAL_ROMS_NUMBER);
     SendMessage( hStatus, SB_SETTEXT, 1, (LPARAM)TempMessage );  
  }
}

void AddDirToList(char RomBrowserDir[MAX_PATH],BOOL sortflag)
{
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    char FullPath[MAX_PATH], SearchSpec[MAX_PATH];
    strcpy(SearchSpec,RomBrowserDir);
	if (SearchSpec[strlen(RomBrowserDir) - 1] != '\\') { strcat(SearchSpec,"\\"); }
	strcat(SearchSpec,"*.*");
    hFind = FindFirstFile(SearchSpec, &fd);
    
    do {
     if (strcmp(fd.cFileName, ".") == 0) { continue; }
     if (strcmp(fd.cFileName, "..") == 0) { continue; }
     strcpy(FullPath,RomBrowserDir);
     if (FullPath[strlen(RomBrowserDir) - 1] != '\\') { strcat(FullPath,"\\"); }
     strcat(FullPath,fd.cFileName);
     if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            if (Config.RomBrowserRecursion) { 
              		AddDirToList(FullPath,FALSE); }
			continue;
		}
	 ROM_SIZE = fill_header(FullPath);
     if (ROM_SIZE>10) {
           AddRomToList(FullPath);
           sprintf(TempMessage,"Adding ROM to list: %s",fd.cFileName);
           SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)TempMessage );  
          }
    } while (FindNextFile(hFind, &fd));
    SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)"" ); 
    if (sortflag) ListViewSort();
    SaveRomBrowserCache(); 
}

void CreateRomListControl (HWND hParent) {
   RECT rcl,rtool,rstatus; 
   INITCOMMONCONTROLSEX icex;
   HIMAGELIST hSmall;               // List View Images
   HICON hIcon;                     // Icon Handle
   icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
   icex.dwICC  = ICC_LISTVIEW_CLASSES;
   InitCommonControlsEx(&icex); 
   
   GetClientRect (hParent, &rcl); 
   GetWindowRect (hTool, &rtool); 
   GetWindowRect (hStatus, &rstatus); 
   hRomList = CreateWindowEx( WS_EX_CLIENTEDGE,WC_LISTVIEW,NULL,
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | 
					LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS ,
					0,rtool.bottom - rtool.top,rcl.right - rcl.left , rcl.bottom - rcl.top - rtool.bottom + rtool.top - rstatus.bottom + rstatus.top,hParent,(HMENU)IDC_ROMLIST,app_hInstance,NULL);
					
	ListView_SetExtendedListViewStyle(hRomList,LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );
	
    // Create the Image List

    hSmall = ImageList_Create( 16, 16, ILC_COLORDDB | ILC_MASK, 11, 0 );

    // Load Image List Icons

    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_GERMANY ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_USA ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_JAPAN ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_EUROPE ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_AUSTRALIA ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_ITALIA ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_FRANCE ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_SPAIN ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_UNKNOWN ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_DEMO ) );
    ImageList_AddIcon( hSmall, hIcon );
    hIcon = LoadIcon( app_hInstance, MAKEINTRESOURCE( IDI_BETA ) );
    ImageList_AddIcon( hSmall, hIcon );
    

    // Associate the Image List with the List View Control

    ListView_SetImageList( hRomList, hSmall, LVSIL_SMALL );
    ResetRomBrowserColomuns();
    LoadRomList();
}

void ShowRomBrowser(BOOL flag, BOOL showRom) //flag - hide/show browser; showRom - false means resize only
{
    static RECT rcSaveMainWindow;
    if (!flag) 
       {
        GetWindowRect(mainHWND, &rcSaveMainWindow);
        EnableWindow(hRomList,FALSE);
        ShowWindow(hRomList,SW_HIDE); 
        SendMessage(hTool, TB_AUTOSIZE, 0, 0);
        SendMessage(hStatus, WM_SIZE, 0, 0);
        SendMessage(mainHWND,WM_USER + 17,0,0);
       }
    else 
       { 
        MoveWindow(mainHWND, rcSaveMainWindow.left, rcSaveMainWindow.top, rcSaveMainWindow.right-rcSaveMainWindow.left, rcSaveMainWindow.bottom-rcSaveMainWindow.top, TRUE);
        if (showRom)
        {
            EnableWindow(hRomList, TRUE);
            ShowWindow(hRomList, SW_SHOW);
            SendMessage(hTool, TB_AUTOSIZE, 0, 0);
            SendMessage(hStatus, WM_SIZE, 0, 0);
            ResizeRomListControl();
        }
       } 
}

void FastRefreshBrowser()
{
    TOTAL_ROMS_NUMBER = 0 ;
    SetRealColumnArray();
    SaveRomBrowserCache();
    ResetRomBrowserColomuns();
    LoadRomBrowserCache();
    ListViewSort();
}

void RefreshRomBrowser()
{
    remove(get_cachepath());
    SaveRomBrowserDirs();
    TOTAL_ROMS_NUMBER = 0 ;
    ShowTotalRoms();
    ListView_DeleteAllItems (hRomList);
    freeRomList();
	LoadRomList();
}

void drawSortArrow(int SubItem)
{
    HDITEM HeaderItem; 
    HWND HeaderCtrl = ListView_GetHeader(hRomList); 
    HeaderItem.mask = HDI_FORMAT | HDI_BITMAP; 
    Header_GetItem(HeaderCtrl,SubItem, &HeaderItem); 
//    HeaderItem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT; 
    //XP arrow     0x0400 - Up,  0x0200 - Down
    HeaderItem.fmt = HDF_STRING | HDF_BITMAP | (SortAscending ? 0x0400:0x0200) | HDF_BITMAP_ON_RIGHT;  
    HeaderItem.hbm = (HBITMAP)LoadImage(app_hInstance, MAKEINTRESOURCE(SortAscending ? IDB_UP_ARROW : IDB_DOWN_ARROW), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS | LR_SHARED); 
    //HeaderItem.hbm = (HBITMAP)LoadIcon( app_hInstance, MAKEINTRESOURCE(SortAscending ? IDB_UP_ARROW:IDB_DOWN_ARROW) );
    Header_SetItem(HeaderCtrl,SubItem, &HeaderItem);
    
    if (m_CurrentSortItem != SubItem) { 
        Header_GetItem(HeaderCtrl,m_CurrentSortItem, &HeaderItem); 
        HeaderItem.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT | (SortAscending ? 0x0400:0x0200)); 
        
        Header_SetItem(HeaderCtrl,m_CurrentSortItem, &HeaderItem); 
        m_CurrentSortItem = SubItem; 
    } 
 
}

void RomList_ColoumnSortList(LPNMLISTVIEW pnmv)
{
    if (getSortColumn()==pnmv->iSubItem) {
        SortAscending = SortAscending?FALSE:TRUE;
        sprintf(Config.RomBrowserSortMethod,SortAscending?"ASC":"DESC");
    }
    Config.RomBrowserSortColumn = pnmv->iSubItem;
    ListViewSort();
}

int CALLBACK RomList_CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{  
    char country1[50],country2[50];
    ROM_INFO * pRomInfo1 = &ItemList.List[SortAscending?lParam1:lParam2];
	ROM_INFO * pRomInfo2 = &ItemList.List[SortAscending?lParam2:lParam1];
   switch ( RealColumn[lParamSort]) {
	case 0 : return (int)lstrcmpi(pRomInfo1->GoodName, pRomInfo2->GoodName);
	break;
	case 1 : return (int)lstrcmpi(pRomInfo1->InternalName, pRomInfo2->InternalName);
	break;
    case 2 : 
             CountryCodeToCountryName(pRomInfo1->Country,country1);
             CountryCodeToCountryName(pRomInfo2->Country,country2);  
             return (int)lstrcmpi(country1,country2); 
    break;
	case 3 : return (int)pRomInfo1->RomSize - (int)pRomInfo2->RomSize; 
	break;
	case 4 : return (int)lstrcmpi(pRomInfo1->UserNotes, pRomInfo2->UserNotes);
	break;
    case 5 : return (int)lstrcmpi(pRomInfo1->szFullFileName, pRomInfo2->szFullFileName);
	break;
	case 6 : return (int)lstrcmpi(pRomInfo1->MD5 , pRomInfo2->MD5);
	break;
    default :
	         return 0; 
	 break;
  }
}

void ResizeRomListControl() {
	WORD y = 0;
	RECT rc,rcMain;
	WORD nWidth,nHeight;
     if (!emu_launched) {
     if (IsWindow(hRomList)) {
		GetClientRect(mainHWND, &rcMain);
		nWidth = rcMain.right - rcMain.left;
		nHeight = rcMain.bottom - rcMain.top;
        if (IsWindow(hStatus)) {
			
            GetWindowRect(hStatus, &rc);
			nHeight -= (WORD)(rc.bottom - rc.top);
		}
		if (IsWindow(hTool))   {
		    GetWindowRect(hTool, &rc);
			y += (WORD)(rc.bottom - rc.top);
        }
		MoveWindow(hRomList, 0, y, nWidth, nHeight-y, TRUE);
	  }
	}
}

ROM_INFO *getSelectedRom() {
    HMENU hMenu;
    LV_ITEM lvItem;
    ROM_INFO * pRomInfo;
    LONG iItem;
    iItem = ListView_GetNextItem(hRomList, -1, LVNI_SELECTED);
    hMenu = GetMenu(mainHWND);
    EnableMenuItem(hMenu,ID_POPUP_ROM_SETTING,MF_GRAYED);   //Grays ROM Settings Button
    SendMessage( hTool, TB_ENABLEBUTTON, EMU_PLAY, FALSE );  //Grays start game button
    if (iItem == -1) { return NULL; }
    memset(&lvItem, 0, sizeof(LV_ITEM));
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = iItem;
    if (!ListView_GetItem(hRomList, &lvItem)) { return NULL; }
    pRomInfo = &ItemList.List[lvItem.lParam];
    if (!pRomInfo) { return NULL; }
    EnableMenuItem(hMenu,ID_POPUP_ROM_SETTING,MF_ENABLED);  //Enables ROM Settings Button
    SendMessage( hTool, TB_ENABLEBUTTON, EMU_PLAY, TRUE );  //Enagles start game button
    return pRomInfo;
}

void saveMD5toCache(char md5str[33]) {
     ROM_INFO * pRomInfo;
     char tempname[100];
     pRomInfo = getSelectedRom();
     if (pRomInfo==NULL) { 
        return ; 
      }
     strcpy(pRomInfo->MD5,md5str);
     if(getIniGoodNameByMD5(md5str,tempname))
	     strcpy(pRomInfo->GoodName,tempname);
	 else
	     sprintf(pRomInfo->GoodName,"%s (not found in INI file)", pRomInfo->InternalName);
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////// Recent Roms code ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

char* ParseName(char *rompath)
{
    char drive[_MAX_DRIVE], dirn[_MAX_DIR] ;
	static char fname[_MAX_FNAME], ext[_MAX_EXT] ;
	
	_splitpath(rompath, drive, dirn, fname, ext);
    return fname;
}

void ShiftRecentRoms()
{
    int i;
    for ( i = MAX_RECENT_ROMS - 1; i > 0; i--)
            {
                 sprintf( Config.RecentRoms[i], Config.RecentRoms[i-1] );     
            }
}


void ClearRecentList (HWND hwnd,BOOL clear_array) {
	int i;
    HMENU hMenu;
	
    hMenu = GetMenu(hwnd);
	for (i = 0; i < MAX_RECENT_ROMS; i ++ ) {
		DeleteMenu(hMenu, ID_RECENTROMS_FIRST + i, MF_BYCOMMAND);
	}
	if (clear_array) {
	    memset( Config.RecentRoms, 0, MAX_RECENT_ROMS * sizeof(Config.RecentRoms[0]));
    }
   
}

void SetRecentList(HWND hwnd) {
    int i;
    HMENU hMenu, hSubMenu ;
    MENUITEMINFO menuinfo;
    FreezeRecentRoms( hwnd, FALSE ) ;
    for ( i = 0 ; i < MAX_RECENT_ROMS  ; i++)   {
              if ( strcmp( Config.RecentRoms[i], "")==0 ) continue;
    
              hMenu = GetMenu(hwnd) ;
              hSubMenu = GetSubMenu(hMenu,0);
	          hSubMenu = GetSubMenu(hSubMenu,5);
	
              menuinfo.cbSize = sizeof(MENUITEMINFO);
	          menuinfo.fMask = MIIM_TYPE|MIIM_ID;
	          menuinfo.fType = MFT_STRING;
	          menuinfo.fState = MFS_ENABLED;
	          menuinfo.dwTypeData = ParseName( Config.RecentRoms[i]);
	          menuinfo.cch = sizeof( hSubMenu);
	          menuinfo.wID = ID_RECENTROMS_FIRST + i;
              InsertMenuItem( hSubMenu, 3 + i, TRUE, &menuinfo);
             
     }
   
}

void AddToRecentList(HWND hwnd,char *rompath) {
    
    int i,j;
    if ( Config.RecentRomsFreeze ) return;
    
    for (i=0;i<MAX_RECENT_ROMS-1 ;i++) {
          
          if ( strcmp(Config.RecentRoms[i],rompath)==0) 
          { // Shifting Array up
              for (j=i;j<MAX_RECENT_ROMS-1;j++) 
              {
                  sprintf( Config.RecentRoms[j], Config.RecentRoms[j+1] ); 
              }
              Config.RecentRoms[MAX_RECENT_ROMS-1][0] = '\0';
          }
    }
    
    ShiftRecentRoms();
    
    sprintf( Config.RecentRoms[0], rompath);
     
    ClearRecentList ( hwnd, FALSE) ;
    SetRecentList( hwnd) ;
   
}

void RunRecentRom(int id) {
    int i;
    static char rompath[MAX_PATH];
    i = id - ID_RECENTROMS_FIRST;
    if (i >= 0 && i<MAX_RECENT_ROMS) {
        if (strcmp(Config.RecentRoms[i],"")==0) return;
        sprintf( rompath,Config.RecentRoms[i]);
        StartRom( rompath );
    }
}

void DisableRecentRoms(HMENU hMenu,BOOL disable) {
    int i;
    
    if (disable) {
          for (i=0;i<MAX_RECENT_ROMS;i++) {
               EnableMenuItem(hMenu,ID_RECENTROMS_FIRST + i,MF_GRAYED);     
          }
          
          
    }
    else         {
             for (i=0;i<MAX_RECENT_ROMS;i++) {
               EnableMenuItem(hMenu,ID_RECENTROMS_FIRST + i,MF_ENABLED);
             }  
    }
}

void FreezeRecentRoms(HWND hWnd, BOOL ChangeConfigVariable) {
	HMENU hMenu = GetMenu(hWnd);
	if (ChangeConfigVariable) {
	   Config.RecentRomsFreeze = 1 - Config.RecentRomsFreeze ;
	}
	
	if (Config.RecentRomsFreeze)
    {
	   CheckMenuItem( hMenu, ID_RECENTROMS_FREEZE, MF_BYCOMMAND | MFS_CHECKED );
       
	}
	else
	{
       CheckMenuItem( hMenu, ID_RECENTROMS_FREEZE, MF_BYCOMMAND | MFS_UNCHECKED );
      
    } 
}
