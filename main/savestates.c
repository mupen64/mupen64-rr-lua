/**
 * Mupen64 - savestates.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include <zlib.h>
#include <stdlib.h>
#include <string.h>

#include "../lua/LuaConsole.h"

#include "vcr.h"

#include "savestates.h"
#include "guifuncs.h"
#include "rom.h"
#include "../memory/memory.h"
#include "../memory/flashram.h"
#include "../r4300/r4300.h"
#include "../r4300/interupt.h"

#ifdef WIN32
#include "win/main_win.h"
#endif


extern unsigned long interp_addr;
extern int *autoinc_save_slot;

int savestates_job = 0;
int savestates_job_success = 1;
bool savestates_ignore_nonmovie_warnings = false;

bool old_st; //.st that comes from no delay fix mupen, it has some differences compared to new st:
			 //- one frame of input is "embedded", that is the pif ram holds already fetched controller info.
			 //- execution continues at exception handler (after input poll) at 0x80000180.
bool lockNoStWarn;

bool fix_new_st = true; //this is a switch to enable/disable fixing .st to work for old mupen (and m64plus)
						//read savestate save function for more info
						//right now its hardcoded to enabled
bool st_skip_dma = false;
#define MUPEN64NEW_ST_FIXED (1<<31) //last bit seems to be free

static unsigned int slot = 1;
static char fname[MAX_PATH] = {0,};

void savestates_select_slot(unsigned int s)
{
   if (s > 9) 
		 return;
   slot = s;
}

void savestates_select_filename(const char *fn)
{
   slot += 10;
   if (strlen((const char *)fn) >= MAX_PATH) //don't remove, this could happen when saving st with lua probably
		 return;
   strcpy(fname, (const char *)fn);
}

unsigned const char * savestates_get_selected_filename()
{
	return (unsigned const char *) fname;
}

void savestates_save()
{
	char *filename, buf[1024];
	gzFile f;
	int len, i, filename_f = 0;
   
	savestates_job_success = TRUE;
   
	if (*autoinc_save_slot)
	{
		if (++slot == 10)
		{
			slot = 0;
		}
	}
   
	if (slot <= 9)
	{
		filename = (char*)malloc(strlen(get_savespath())+
		strlen(ROM_SETTINGS.goodname)+4+1);
		strcpy(filename, get_savespath());
		strcat(filename, ROM_SETTINGS.goodname);
		strcat(filename, ".st");
		sprintf(buf, "%d", slot);
		strcat(filename, buf);
	}
	else
	{
		filename = (char*)malloc(strlen(fname)+1);
		strcpy(filename, fname);
		slot -= 10;
		filename_f = 1;
	}

	{
		char str [256];
		if(filename_f)
			sprintf(str, "saving %-200s", filename);
		else
			sprintf(str, "saving slot %d", slot);
		display_status(str);
	}
   	
	f = gzopen(filename, "wb");
	free(filename);
   
	gzwrite(f, ROM_SETTINGS.MD5, 32);
   
	//if fixing enabled...
	if (fix_new_st)
	{
		//this is code taken from dma.c:dma_si_read(), it finishes up the dma.
		//it copies data from pif (should contain commands and controller states), updates count reg and adds SI interrupt to queue
		//so why does old mupen and mupen64plus dislike savestates without doing this? well in case of mario 64 it leaves pif command buffer uninitialised
		//and it never can poll input properly (hence the inability to frame advance which is handled inside controller read).
		
		//But we dont want to do this then load such .st and dma again... so I notify mupen about this in .st,
		//since .st is filled up to the brim with data (not even a single unused offset) I have to store one bit in... rdram manufacturer register
		//this 99.999% wont break on any game, and that bit will be cleared by mupen64plus converter as well, so only old old mupen ever sees this trick.
		for (i = 0; i < (64 / 4); i++)
			rdram[si_register.si_dram_addr / 4 + i] = sl(PIF_RAM[i]);
		update_count();
		add_interupt_event(SI_INT, /*0x100*/0x900);
		rdram_register.rdram_device_manuf |= MUPEN64NEW_ST_FIXED;
		st_skip_dma = true;
		//hack end
	}
	gzwrite(f, &rdram_register, sizeof(RDRAM_register));
	gzwrite(f, &MI_register, sizeof(mips_register));
	gzwrite(f, &pi_register, sizeof(PI_register));
	gzwrite(f, &sp_register, sizeof(SP_register));
	gzwrite(f, &rsp_register, sizeof(RSP_register));
	gzwrite(f, &si_register, sizeof(SI_register));
	gzwrite(f, &vi_register, sizeof(VI_register));
	gzwrite(f, &ri_register, sizeof(RI_register));
	gzwrite(f, &ai_register, sizeof(AI_register));
	gzwrite(f, &dpc_register, sizeof(DPC_register));
	gzwrite(f, &dps_register, sizeof(DPS_register));
	gzwrite(f, rdram, 0x800000);
	gzwrite(f, SP_DMEM, 0x1000);
	gzwrite(f, SP_IMEM, 0x1000);
	gzwrite(f, PIF_RAM, 0x40);

	save_flashram_infos(buf);
	gzwrite(f, buf, 24);
   
	gzwrite(f, tlb_LUT_r, 0x100000);
	gzwrite(f, tlb_LUT_w, 0x100000);
   
	gzwrite(f, &llbit, 4);
	gzwrite(f, reg, 32*8);
	for (i=0; i<32; i++)
		gzwrite(f, reg_cop0+i, 8); // *8 for compatibility with old versions purpose
	gzwrite(f, &lo, 8);
	gzwrite(f, &hi, 8);
	gzwrite(f, reg_cop1_fgr_64, 32*8);
	gzwrite(f, &FCR0, 4);
	gzwrite(f, &FCR31, 4);
	gzwrite(f, tlb_e, 32*sizeof(tlb));
	if (!dynacore && interpcore) 
		gzwrite(f, &interp_addr, 4);
	else 
		gzwrite(f, &PC->addr, 4);
   
	gzwrite(f, &next_interupt, 4);
	gzwrite(f, &next_vi, 4);
	gzwrite(f, &vi_field, 4);
   
	len = save_eventqueue_infos(buf);
	gzwrite(f, buf, len);

	// re-recording
	BOOL movieActive = VCR_isActive();
	gzwrite(f, &movieActive, sizeof(movieActive));
	if(movieActive)
	{
		char* movie_freeze_buf = NULL;
		unsigned long movie_freeze_size = 0;

		VCR_movieFreeze(&movie_freeze_buf, &movie_freeze_size);
		if(movie_freeze_buf)
		{
			gzwrite(f, &movie_freeze_size, sizeof(movie_freeze_size));
			gzwrite(f, movie_freeze_buf, movie_freeze_size);
			free(movie_freeze_buf);
		}
		else
		{
			fprintf(stderr, "Failed to save movie snapshot.\n");
			savestates_job_success = FALSE;
		}
	}
	AtSaveStateLuaCallback();
	// /re-recording

	gzclose(f);
}

void savestates_load(bool silenceNotFoundError)
{
#define BUFLEN 1024
	char *filename, buf[BUFLEN];
	gzFile f; //handle to st
	int len, i;

	savestates_job_success = TRUE;
   
	//construct .st name for 1-9 slots based on rom name and number
	if (slot <= 9)
	{
		filename = (char*)malloc(strlen(get_savespath())+
		strlen(ROM_SETTINGS.goodname)+4+1);
		strcpy(filename, get_savespath());
		strcat(filename, ROM_SETTINGS.goodname);
		strcat(filename, ".st");
		sprintf(buf, "%d", slot);
		strcat(filename, buf);
	}

	//tricky method, slot is greater than 9, so it uses a global fname array, imo bad programming there but whatever
	else
	{
		filename = (char*)malloc(strlen(fname)+11);
		strcpy(filename, fname);
		//slot -= 10;
	}
	char str[256];

	//print info about loading slot to console,
	if (slot > 9)
	{
		_splitpath(filename, 0, 0, fname,0);
		strcat(fname, ".st");
		sprintf(str, "Loading %s", fname);
	}
	else{
		sprintf(str, "Loading slot %d", slot);
		sprintf(fname, "Slot %d", slot);
	}
	f = gzopen(filename, "rb");

	//try loading .savestate, workaround for discord...
	if (f == NULL && slot > 9)
	{
		filename[strlen(filename) - 3] = '\0';
		strcat(filename, ".savestate");
		f = gzopen(filename, "rb");
	}

    //failed opening st
	if (f == NULL)
	{
		if (f == NULL)
		{
			if (silenceNotFoundError) {
				printf("Silent st fail: Savestate \"%s\" not found.\n", filename);
				return;
			}
			if (slot > 9)
			{
				//print .st not .savestate because
				filename[strlen(filename) - 10] = '\0';
				strcat(filename, ".st");
			}
			printf("Savestate \"%s\" not found.\n", filename); //full path for debug
			free(filename);
			warn_savestate(0, "Savestate not found"); // example: removing this (also happens sometimes normally) will make "loading slot" text flicker for like a milisecond which looks awful,
												  // by moving the warn function it doesn't do this anymore 
			savestates_job_success = FALSE;
			return;
		}
	}
	display_status(str);
	free(filename);

	//printf("--------st start---------\n");
	gzread(f, buf, 32);
	if (memcmp(buf, ROM_SETTINGS.MD5, 32))
	{
#ifdef WIN32
		extern CONFIG Config;
		if (Config.moviesERRORS)
			warn_savestate("Savestate Warning", "You have option 'Allow loading movies on wrong roms' selected.\nMismatched .st is going to be loaded", TRUE);
#endif
		else
		{
			if (!VCR_isRecording())
				warn_savestate("Savestates Wrong Region", "Savestate Wrong Region");
			else
				warn_savestate("Savestates Wrong Region", "This savestate is from another ROM or version\nRecording will be stopped!", TRUE);
			gzclose(f);
			savestates_job_success = FALSE;
			if (VCR_isRecording()) VCR_stopRecord(1);
			else VCR_stopPlayback();
			return;
		}
	}
   
	gzread(f, &rdram_register, sizeof(RDRAM_register));
	if (rdram_register.rdram_device_manuf & MUPEN64NEW_ST_FIXED)
	{
		rdram_register.rdram_device_manuf &= ~MUPEN64NEW_ST_FIXED; //remove the trick
		st_skip_dma = true; //tell dma.c to skip it
	}
	gzread(f, &MI_register, sizeof(mips_register));
	gzread(f, &pi_register, sizeof(PI_register));
	gzread(f, &sp_register, sizeof(SP_register));
	gzread(f, &rsp_register, sizeof(RSP_register));
	gzread(f, &si_register, sizeof(SI_register));
	gzread(f, &vi_register, sizeof(VI_register));
	gzread(f, &ri_register, sizeof(RI_register));
	gzread(f, &ai_register, sizeof(AI_register));
	gzread(f, &dpc_register, sizeof(DPC_register));
	gzread(f, &dps_register, sizeof(DPS_register));
	gzread(f, rdram, 0x800000);
	gzread(f, SP_DMEM, 0x1000);
	gzread(f, SP_IMEM, 0x1000);
	gzread(f, PIF_RAM, 0x40);

	gzread(f, buf, 24);
	load_flashram_infos(buf);
   
	gzread(f, tlb_LUT_r, 0x100000);
	gzread(f, tlb_LUT_w, 0x100000);
   
	gzread(f, &llbit, 4);
	gzread(f, reg, 32*8);
	for (i=0; i<32; i++) 
	{
		gzread(f, reg_cop0+i, 4);
		gzread(f, buf, 4); // for compatibility with old versions purpose
	}
	gzread(f, &lo, 8);
	gzread(f, &hi, 8);
	gzread(f, reg_cop1_fgr_64, 32*8);
	gzread(f, &FCR0, 4);
	gzread(f, &FCR31, 4);
	gzread(f, tlb_e, 32*sizeof(tlb));
	if (!dynacore && interpcore)
		gzread(f, &interp_addr, 4);
	else
	{
		int i;
		gzread(f, &len, 4);
		for (i=0; i<0x100000; i++) 
			invalid_code[i] = 1;
		jump_to(len);
	}
   
	gzread(f, &next_interupt, 4);
	gzread(f, &next_vi, 4);
	gzread(f, &vi_field, 4);
   
	for (len = 0; len < BUFLEN; len += 8)
	{
		gzread(f, buf+len, 4);
		if (*((unsigned long*)&buf[len]) == 0xFFFFFFFF) 
			break;
		gzread(f, buf+len+4, 4);
	}
	if (len == BUFLEN) { // Exhausted the buffer and still no terminator. Prevents the buffer overflow "Queuecrush".
		fprintf(stderr, "Snapshot event queue terminator not reached.\n");
		savestates_job_success = FALSE;
		warn_savestate("Savestate error", "Event queue too long (Savestate corrupted?)");
		return;
	}
	load_eventqueue_infos(buf);
      
	BOOL movieSnapshot;
	gzread(f, &movieSnapshot, sizeof(movieSnapshot));
	if(VCR_isActive() && !movieSnapshot)
	{
		fprintf(stderr, "Can't load a non-movie snapshot while a movie is active.\n");
		savestates_job_success = FALSE;
		//goto failedLoad;
	}
   
	if(movieSnapshot) // even if a movie isn't active we still want to parse through this in case other stuff is added later on in the save format
	{
		unsigned long movieInputDataSize = 0;
		gzread(f, &movieInputDataSize, sizeof(movieInputDataSize));
		char* local_movie_data = (char*)malloc(movieInputDataSize * sizeof(char));
		int readBytes = gzread(f, local_movie_data, movieInputDataSize);
		if(readBytes != movieInputDataSize)
		{
			fprintf(stderr, "Corrupt movie snapshot.\n");
			if(local_movie_data)
				free(local_movie_data);
			savestates_job_success = FALSE;
			goto failedLoad;
		}
		int code = VCR_movieUnfreeze(local_movie_data, movieInputDataSize);
		if(local_movie_data) 
			free(local_movie_data);
		if(code != SUCCESS && !VCR_isIdle())
		{
			bool stop = false;
			char errStr [1024];
			strcpy(errStr, "Failed to load movie snapshot,\n");
			switch(code)
			{
				case NOT_FROM_THIS_MOVIE: 
					strcat(errStr, "snapshot not from this movie\n"); 
					break;
				case NOT_FROM_A_MOVIE:
					strcat(errStr, "not a movie snapshot\n"); 
					break;// shouldn't get here...
				case INVALID_FRAME:
					strcat(errStr, "invalid frame number\n");
					break;
				case WRONG_FORMAT: 
					strcat(errStr, "wrong format\n");
					stop = true;
					break;
			}
			if (savestates_ignore_nonmovie_warnings) {
				display_status("Warning: mismatched savestate\n");
			}
			else {
				printWarning(errStr);
				if (stop && VCR_isRecording()) VCR_stopRecord(1);
				else if (stop) VCR_stopPlayback();
				savestates_job_success = FALSE;
				goto failedLoad;
			}
		}
	}
	else // loading a non-movie snapshot from a movie
	{
		if (VCR_isActive() && savestates_ignore_nonmovie_warnings) {
			display_status("Warning: non-movie savestate\n");
		}
		else if (VCR_isActive()&&!silenceNotFoundError&&!lockNoStWarn)
		{
			if (MessageBox(NULL, "This savestate isn't from this movie, do you want to load it? (will desync your movie)", "Warning", MB_YESNO | MB_ICONWARNING)== 7) {
			printf("[VCR]: Warning: The movie has been stopped to load this non-movie snapshot.\n");
			if (VCR_isPlaying())
				VCR_stopPlayback();
			else
				VCR_stopRecord(1);
			}
			lockNoStWarn = false; //reset
		}
	}
	AtLoadStateLuaCallback();
	//extern long m_currentSample;
	//printf(".st frame: %d\n", m_currentSample);
	//printf("--------st end-----------\n");
failedLoad:

	gzclose(f);
	extern bool ignore,old_st;
	//legacy .st fix, makes BEQ instruction ignore jump, because .st writes new address explictly.
	//This should cause issues anyway but libultra seems to be flexible (this means there's a chance it fails).
	//For safety, load .sts in dynarec because it completely avoids this issue by being differently coded
	old_st = (interp_addr == 0x80000180 || PC->addr == 0x80000180);
	//doubled because can't just reuse this variable
	if (interp_addr == 0x80000180 || (PC->addr == 0x80000180 && !dynacore))
		ignore = true;
	if (!dynacore && interpcore)
	{
		//printf(".st jump: %x, stopped here:%x\n", interp_addr, last_addr);
		last_addr = interp_addr;
	}
	else
	{
		//printf(".st jump: %x, stopped here:%x\n", PC->addr, last_addr);
		last_addr = PC->addr;
	}
}
#undef BUFLEN
