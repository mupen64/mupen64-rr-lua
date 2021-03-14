/***************************************************************************
                          config.h  -  description
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

void WriteCfgString   (char *Section,char *Key,char *Value) ;
void WriteCfgInt      (char *Section,char *Key,int Value) ;
void ReadCfgString    (char *Section,char *Key,char *DefaultValue,char *retValue) ;
int ReadCfgInt        (char *Section,char *Key,int DefaultValue) ;

extern int round_to_zero;
extern int input_delay;
extern int LUA_double_buffered; 
extern int resetRec;


void LoadConfig()  ;
void SaveConfig()  ; 
    
