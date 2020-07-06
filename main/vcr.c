//#include "../config.h"
#ifdef VCR_SUPPORT

#include "vcr.h"
#include "vcr_compress.h"
#include "vcr_resample.h"

#include "plugin.h"
#include "rom.h"
#include "savestates.h"
#include "../memory/memory.h"
#include "../lua/LuaConsole.h"

#include <errno.h>
#include <limits.h>
#include <memory.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#define snprintf	_snprintf
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#else
#include <unistd.h>
#endif
//#include <zlib.h>
#include <stdio.h>
#include <time.h>

#ifndef __WIN32__
#include <gtk/gtk.h> // for getting callback_startEmulation and callback_stopEmulation
#include <unistd.h> // for truncate
#include <gui_gtk/messagebox.h>
#define stricmp strcasecmp
#else
#include <commctrl.h> // for SendMessage, SB_SETTEXT
#include <windows.h> // for truncate functions
#include <../../winproject/resource.h> // for EMU_RESET
#include <WinUser.h>

#endif

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

//void ShowInfo(char*, ...);
//#define printf ShowInfo // temporary debuggification

#define MUP_MAGIC (0x1a34364d) // M64\0x1a
#define MUP_VERSION (3)
#define MUP_HEADER_SIZE_OLD (512) // bytes
#define MUP_HEADER_SIZE (sizeof(SMovieHeader))
#define MUP_HEADER_SIZE_CUR (m_header.version <= 2 ? MUP_HEADER_SIZE_OLD : MUP_HEADER_SIZE)



#define BUFFER_GROWTH_SIZE (4096)

static const char *m_errCodeName[] =
{
	"Success",
	"Wrong Format",
	"Wrong Version",
	"File Not Found",
	"Not From This Movie",
	"Not From A Movie",
	"Invalid Frame",
	"Unknown Error"
};


enum ETask
{
	Idle = 0,
	StartRecording,
	StartRecordingFromSnapshot,
	Recording,
	StartPlayback,
	StartPlaybackFromSnapshot,
	Playback
};
/*
static const char *m_taskName[] =
{
	"Idle",
	"StartRecording",
	"StartRecordingFromSnapshot",
	"Recording",
	"StartPlayback",
	"StartPlaybackFromSnapshot",
	"Playback"
};
*/

static char   m_filename[PATH_MAX];
static char   AVIFileName[PATH_MAX];
static FILE*  m_file = 0;
static int    m_task = Idle;

static SMovieHeader m_header;
static BOOL m_readOnly = FALSE;

long m_currentSample = -1;	// should = length_samples when recording, and be < length_samples when playing
int m_currentVI = -1;
static char* m_inputBuffer = NULL;
static unsigned long m_inputBufferSize = 0;
static char* m_inputBufferPtr = NULL;
//static BOOL m_intro = TRUE;
static unsigned long m_lastController1Keys = 0; // for input display

static int    m_capture = 0;			// capture movie
static int    m_audioFreq = 33000;		//0x30018;
static int    m_audioBitrate = 16;		// 16 bits
static float  m_videoFrame = 0;
static float  m_audioFrame = 0;
static char soundBuf [44100*2];
static char soundBufEmpty [44100*2];
static int soundBufPos = 0;
long lastSound = 0;
volatile BOOL captureFrameValid = FALSE;
static int AVIBreakMovie = 0;


static void printWarning (char* str)
{
#ifdef __WIN32__
	extern BOOL cmdlineNoGui;
	if(cmdlineNoGui)
		printf( "Warning: %s\n", str );
	else
		MessageBox(NULL, str, "Warning", MB_OK | MB_ICONWARNING);
#else
	extern int g_GuiEnabled;
	if (!g_GuiEnabled)
		printf( "Warning: %s\n", str );
	else
		messagebox(tr("Warning"), MB_OK, str );
#endif
}

static void printError (char* str)
{
#ifdef __WIN32__
	extern BOOL cmdlineNoGui;
	if(cmdlineNoGui)
		fprintf(stderr, "Error: %s\n", str );
	else
		MessageBox(NULL, str, "Error", MB_OK | MB_ICONERROR);
#else
	extern int g_GuiEnabled;
	if (!g_GuiEnabled)
		fprintf(stderr, "Error: %s\n", str );
	else
		messagebox(tr("Error"), MB_OK, str );
#endif
}

static void hardResetAndClearAllSaveData ()
{
#ifdef __WIN32__
//	extern void resetEmu();
	extern BOOL clear_sram_on_restart_mode;
	extern BOOL continue_vcr_on_restart_mode;
	extern HWND mainHWND;
	clear_sram_on_restart_mode = TRUE;
	continue_vcr_on_restart_mode = TRUE;
//	resetEmu();
	SendMessage(mainHWND, WM_COMMAND, EMU_RESET, 0);
#else
	extern void callback_startEmulation( GtkWidget *widget, gpointer data );
	extern void callback_stopEmulation( GtkWidget *widget, gpointer data );
	callback_stopEmulation( NULL, NULL );
	VCR_clearAllSaveData();
	callback_startEmulation( NULL, NULL );
#endif
}

static int visByCountrycode()
{
	switch(ROM_HEADER ? (ROM_HEADER->Country_code&0xFF) : -1)
	{
	case 0x44:
	case 0x46:
	case 0x49:
	case 0x50:
	case 0x53:
	case 0x55:
	case 0x58:
	case 0x59:
		return 50;
		break;

	case 0x37:
	case 0x41:
	case 0x45:
	case 0x4a:
		return 60;
		break;
	}
	printWarning("[VCR]: Warning - unknown country code, using 60 FPS for video.\n");
	return 60;
}

static void setROMInfo (SMovieHeader* header)
{

	// FIXME
     switch(ROM_HEADER ? (ROM_HEADER->Country_code&0xFF) : -1)
     {
		case 0x37:
		case 0x41:
		case 0x45:
		case 0x4a:
	    default:
		  header->vis_per_second = 60; // NTSC
		  break;
		case 0x44:
		case 0x46:
		case 0x49:
		case 0x50:
		case 0x53:
		case 0x55:
		case 0x58:
		case 0x59:
		  header->vis_per_second = 50; // PAL
		  break;
     }

	header->controllerFlags = 0;
	header->num_controllers = 0;
	if(Controls[0].Present)
		header->controllerFlags |= CONTROLLER_1_PRESENT, header->num_controllers++;
	if(Controls[1].Present)
		header->controllerFlags |= CONTROLLER_2_PRESENT, header->num_controllers++;
	if(Controls[2].Present)
		header->controllerFlags |= CONTROLLER_3_PRESENT, header->num_controllers++;
	if(Controls[3].Present)
		header->controllerFlags |= CONTROLLER_4_PRESENT, header->num_controllers++;
	if(Controls[0].Plugin == PLUGIN_MEMPAK)
		header->controllerFlags |= CONTROLLER_1_MEMPAK;
	if(Controls[1].Plugin == PLUGIN_MEMPAK)
		header->controllerFlags |= CONTROLLER_2_MEMPAK;
	if(Controls[2].Plugin == PLUGIN_MEMPAK)
		header->controllerFlags |= CONTROLLER_3_MEMPAK;
	if(Controls[3].Plugin == PLUGIN_MEMPAK)
		header->controllerFlags |= CONTROLLER_4_MEMPAK;
	if(Controls[0].Plugin == PLUGIN_RUMBLE_PAK)
		header->controllerFlags |= CONTROLLER_1_RUMBLE;
	if(Controls[1].Plugin == PLUGIN_RUMBLE_PAK)
		header->controllerFlags |= CONTROLLER_2_RUMBLE;
	if(Controls[2].Plugin == PLUGIN_RUMBLE_PAK)
		header->controllerFlags |= CONTROLLER_3_RUMBLE;
	if(Controls[3].Plugin == PLUGIN_RUMBLE_PAK)
		header->controllerFlags |= CONTROLLER_4_RUMBLE;

	extern rom_header *ROM_HEADER;
	if(ROM_HEADER)
	    strncpy(header->romNom, (const char*)ROM_HEADER->nom, 32);
    else
	    strncpy(header->romNom, "(Unknown)", 32);
    header->romCRC = ROM_HEADER ? ROM_HEADER->CRC1 : 0;
    header->romCountry = ROM_HEADER ? ROM_HEADER->Country_code : -1;

	header->inputPluginName[0] = '\0';
	header->videoPluginName[0] = '\0';
	header->soundPluginName[0] = '\0';
	header->rspPluginName[0] = '\0';
	
#ifdef __WIN32__
/*	ROM_INFO* pRomInfo = NULL;//getSelectedRom(); // FIXME - what if the user selected one ROM and File->Opened a different one?
    if(pRomInfo)
    {
	    DEFAULT_ROM_SETTINGS TempRomSettings = GetDefaultRomSettings( pRomInfo->InternalName);
	    strncpy(header->inputPluginName, TempRomSettings.InputPluginName, 64);
	    strncpy(header->videoPluginName, TempRomSettings.GfxPluginName, 64);
	    strncpy(header->soundPluginName, TempRomSettings.SoundPluginName, 64);
	    strncpy(header->rspPluginName, TempRomSettings.RspPluginName, 64);
	}*/

	extern char gfx_name[255];
	extern char input_name[255];
	extern char sound_name[255];
	extern char rsp_name[255];

//	if(!header->videoPluginName[0])
		strncpy(header->videoPluginName, gfx_name, 64);
//	if(!header->inputPluginName[0])
		strncpy(header->inputPluginName, input_name, 64);
//	if(!header->soundPluginName[0])
		strncpy(header->soundPluginName, sound_name, 64);
//	if(!header->rspPluginName[0])
		strncpy(header->rspPluginName, rsp_name, 64);
#else
	{
		char *name;
		extern char *plugin_name_by_filename(const char *filename); // from main_gtk.c
		extern const char * config_get_string( const char *key, const char *def ); // from config.c

		name = plugin_name_by_filename( config_get_string( "Gfx Plugin", "" ) );
		if(name)
			strncpy(header->videoPluginName, name, 64);

		name = plugin_name_by_filename( config_get_string( "Input Plugin", "" ) );
		if(name)
			strncpy(header->inputPluginName, name, 64);

		name = plugin_name_by_filename( config_get_string( "Audio Plugin", "" ) );
		if(name)
			strncpy(header->soundPluginName, name, 64);

		name = plugin_name_by_filename( config_get_string( "RSP Plugin", "" ) );
		if(name)
			strncpy(header->rspPluginName, name, 64);
	}
#endif
}

static void reserve_buffer_space(unsigned long space_needed)
{
	if(space_needed > m_inputBufferSize)
	{
		unsigned long ptr_offset = m_inputBufferPtr - m_inputBuffer;
		unsigned long alloc_chunks = space_needed / BUFFER_GROWTH_SIZE;
		m_inputBufferSize = BUFFER_GROWTH_SIZE * (alloc_chunks+1);
		m_inputBuffer = (char*)realloc(m_inputBuffer, m_inputBufferSize);
		m_inputBufferPtr = m_inputBuffer + ptr_offset;
	}
}

static void truncateMovie()
{
	// truncate movie controller data to header.length_samples length

	long truncLen = MUP_HEADER_SIZE + sizeof(BUTTONS)*(m_header.length_samples+1);

#ifdef __WIN32__
	HANDLE fileHandle = CreateFile(m_filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
	if(fileHandle != NULL)
	{
		SetFilePointer(fileHandle, truncLen, 0, FILE_BEGIN);
		SetEndOfFile(fileHandle);
		CloseHandle(fileHandle);
	}
#else
	truncate(m_filename, truncLen);
#endif
}

static int read_movie_header(FILE * file, SMovieHeader * header)
{
//	assert(file != NULL);
//	assert(MUP_HEADER_SIZE == sizeof(SMovieHeader)); // sanity check on the header type definition

//		ShowInfo("SIZE = %d",sizeof(SMovieHeader));
		
	fseek(file, 0L, SEEK_SET);

    SMovieHeader newHeader;
	memset(&newHeader, 0, sizeof(SMovieHeader));

	if(fread(&newHeader, 1, MUP_HEADER_SIZE_OLD, file) != MUP_HEADER_SIZE_OLD)
		return WRONG_FORMAT;

	if(newHeader.magic != MUP_MAGIC)
		return WRONG_FORMAT;

	if(newHeader.version <= 0 || newHeader.version > MUP_VERSION)
		return WRONG_VERSION;

	if(newHeader.version == 1 || newHeader.version == 2)
	{
		// attempt to recover screwed-up plugin data caused by
		// version mishandling and format problems of first versions

		#define isAlpha(x) (((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z') || ((x) == '1'))
		int i;
		for(i = 0 ; i < 56+64 ; i++)
			if(isAlpha(newHeader.reservedBytes[i])
			&& isAlpha(newHeader.reservedBytes[i+64])
			&& isAlpha(newHeader.reservedBytes[i+64+64])
			&& isAlpha(newHeader.reservedBytes[i+64+64+64]))
				break;
		if(i != 56+64)
		{
			memmove(newHeader.videoPluginName, newHeader.reservedBytes + i, 256);
		}
		else
		{
			for(i = 0 ; i < 56+64 ; i++)
				if(isAlpha(newHeader.reservedBytes[i])
				&& isAlpha(newHeader.reservedBytes[i+64])
				&& isAlpha(newHeader.reservedBytes[i+64+64]))
					break;
			if(i != 56+64)
				memmove(newHeader.soundPluginName, newHeader.reservedBytes + i, 256-64);
			else
			{
				for(i = 0 ; i < 56+64 ; i++)
					if(isAlpha(newHeader.reservedBytes[i])
					&& isAlpha(newHeader.reservedBytes[i+64]))
						break;
				if(i != 56+64)
					memmove(newHeader.inputPluginName, newHeader.reservedBytes + i, 256-64-64);
				else
				{
					for(i = 0 ; i < 56+64 ; i++)
						if(isAlpha(newHeader.reservedBytes[i]))
							break;
					if(i != 56+64)
						memmove(newHeader.rspPluginName, newHeader.reservedBytes + i, 256-64-64-64);
					else
						strncpy(newHeader.rspPluginName, "(unknown)", 64);
						
					strncpy(newHeader.inputPluginName, "(unknown)", 64);
				}
				strncpy(newHeader.soundPluginName, "(unknown)", 64);
			}
			strncpy(newHeader.videoPluginName, "(unknown)", 64);
		}
		// attempt to convert old author and description to utf8
		strncpy(newHeader.authorInfo, newHeader.oldAuthorInfo, 48);
		strncpy(newHeader.description, newHeader.oldDescription, 80);
	}
	if(newHeader.version >= 3 && newHeader.version <= MUP_VERSION)
	{
		// read rest of header
		if(fread((char*)(&newHeader) + MUP_HEADER_SIZE_OLD, 1, MUP_HEADER_SIZE-MUP_HEADER_SIZE_OLD, file) != MUP_HEADER_SIZE-MUP_HEADER_SIZE_OLD)
			return WRONG_FORMAT;	
	}

    *header = newHeader;

	return SUCCESS;
}


static void write_movie_header(FILE * file, int numBytes)
{
//	assert(ftell(file) == 0); // we assume file points to beginning of movie file
	fseek(file, 0L, SEEK_SET);

	m_header.version = MUP_VERSION; // make sure to update the version!
///	m_header.length_vis = m_header.length_samples / m_header.num_controllers; // wrong

	fwrite(&m_header, 1, MUP_HEADER_SIZE, file);

	fseek(file, 0L, SEEK_END);
}


void flush_movie()
{
	if(m_file && (m_task == Recording || m_task == StartRecording || m_task == StartRecordingFromSnapshot))
	{
		// (over-)write the header
	    write_movie_header(m_file, MUP_HEADER_SIZE);
	
		// (over-)write the controller data
		fseek(m_file, MUP_HEADER_SIZE, SEEK_SET);
		fwrite(m_inputBuffer, 1, sizeof(BUTTONS)*(m_header.length_samples+1), m_file);
	
		fflush(m_file);
	}
}

SMovieHeader VCR_getHeaderInfo(const char* filename)
{
	char buf [PATH_MAX];
	char temp_filename [PATH_MAX];
	FILE* tempFile = NULL;
	SMovieHeader tempHeader;
	memset(&tempHeader, 0, sizeof(SMovieHeader));
	tempHeader.romCountry = -1;
	strcpy(tempHeader.romNom, "(no ROM)");

	flush_movie();

	strncpy( temp_filename, filename, PATH_MAX );
	char *p = strrchr( temp_filename, '.' );
	if (p)
	{
		if (!strcasecmp( p, ".m64" ) || !strcasecmp( p, ".st" ))
			*p = '\0';
	}
	// open record file
	strncpy( buf, temp_filename, PATH_MAX );
	tempFile = fopen( buf, "rb+" );
	if (tempFile == 0 && (tempFile = fopen( buf, "rb" )) == 0)
	{
        strncat( buf, ".m64", PATH_MAX );
	    tempFile = fopen( buf, "rb+" );
    	if (tempFile == 0 && (tempFile = fopen( buf, "rb" )) == 0)
    	{
    		fprintf( stderr, "[VCR]: Could not get header info of .m64 file\n\"%s\": %s\n", filename, strerror( errno ) );
    		return tempHeader;
        }
	}

    read_movie_header(tempFile, &tempHeader);
	fclose(tempFile);
    return tempHeader;
}

// clear all SRAM, EEPROM, and mempaks
void VCR_clearAllSaveData ()
{
    int i;
	extern char *get_savespath(); // defined in either win\guifuncs.c or gui_gtk/main_gtk.c
	
	// clear SRAM
    {
     char *filename;
     FILE *f;
     filename = (char*)malloc(strlen(get_savespath())+
		       strlen(ROM_SETTINGS.goodname)+4+1);
     strcpy(filename, get_savespath());
     strcat(filename, ROM_SETTINGS.goodname);
     strcat(filename, ".sra");
     f = fopen(filename, "rb");
     if(f)
     {
         fclose(f);
	     f = fopen(filename, "wb");
	     if(f)
	     {
          extern unsigned char sram[0x8000];
          for (i=0; i<0x8000; i++) sram[i] = 0;
		  fwrite(sram, 1, 0x8000, f);
		  fclose(f);
         }
     }
     free(filename);
    }
	// clear EEPROM
    {
     char *filename;
     FILE *f;
     int i;
     filename = (char*)malloc(strlen(get_savespath())+
		       strlen(ROM_SETTINGS.goodname)+4+1);
     strcpy(filename, get_savespath());
     strcat(filename, ROM_SETTINGS.goodname);
     strcat(filename, ".eep");
     f = fopen(filename, "rb");
     if (f)
       {
	  fclose(f);
      f = fopen(filename, "wb");
	     if(f)
	     {
          extern unsigned char eeprom[0x8000];
          for (i=0; i<0x800; i++) eeprom[i] = 0;
          fwrite(eeprom, 1, 0x800, f);
          fclose(f);
        }
       }
     free(filename);
    }
	// clear mempaks
    {
	 char *filename;
	 FILE *f;
	 filename = (char*)malloc(strlen(get_savespath())+
			   strlen(ROM_SETTINGS.goodname)+4+1);
	 strcpy(filename, get_savespath());
	 strcat(filename, ROM_SETTINGS.goodname);
	 strcat(filename, ".mpk");
	 
     f = fopen(filename, "rb");
     if (f)
       {
	    fclose(f);
        f = fopen(filename, "wb");
	     if(f)
	     {
          extern unsigned char mempack[4][0x8000];
          int j;
          for (j=0; j<4; j++)
          {
              for (i=0; i<0x800; i++) mempack[j][i] = 0;
                  fwrite(mempack[j], 1, 0x800, f);
          }
          fclose(f);
        }
       }

     free(filename);
    }
	
}



BOOL
VCR_isActive( )
{
	return (m_task == Recording || m_task == Playback) ? TRUE : FALSE;
}

BOOL
VCR_isIdle( )
{
	return (m_task == Idle) ? TRUE : FALSE;
}

BOOL
VCR_isPlaying( )
{
	return (m_task == Playback) ? TRUE : FALSE;
}

BOOL
VCR_isRecording( )
{
	return (m_task == Recording) ? TRUE : FALSE;
}

BOOL
VCR_isCapturing( )
{
	return m_capture ? TRUE : FALSE;
}

BOOL
VCR_getReadOnly( )
{
	return m_readOnly;
}
void
VCR_setReadOnly(BOOL val)
{
#ifdef __WIN32__
	extern HWND mainHWND;
	if(m_readOnly != val)
		CheckMenuItem( GetMenu(mainHWND), EMU_VCRTOGGLEREADONLY, MF_BYCOMMAND | (val ? MFS_CHECKED : MFS_UNCHECKED));
#endif
	m_readOnly = val;
}

unsigned long VCR_getLengthVIs()
{
	return VCR_isActive() ? m_header.length_vis : 0;
}
unsigned long VCR_getLengthSamples()
{
	return VCR_isActive() ? m_header.length_samples : 0;
}
void VCR_setLengthVIs(unsigned long val)
{
	m_header.length_vis = val;
}
void VCR_setLengthSamples(unsigned long val)
{
	m_header.length_samples = val;
}

void
VCR_movieFreeze (char** buf, unsigned long* size)
{
	// sanity check
	if(!VCR_isActive())
	{
		return;
	}

	*buf = NULL;
	*size = 0;

	// compute size needed for the buffer
	unsigned long size_needed = sizeof(m_header.uid) + sizeof(m_currentSample) + sizeof(m_currentVI) + sizeof(m_header.length_samples);			// room for header.uid, currentFrame, and header.length_samples
	size_needed += (unsigned long)(sizeof(BUTTONS) * (m_header.length_samples+1));
	*buf=(char*)malloc(size_needed);
	*size=size_needed;

	char* ptr = *buf;
	if(!ptr)
	{
		return;
	}

	*((unsigned long*)ptr) = m_header.uid;
	ptr += sizeof(m_header.uid);
	*((unsigned long*)ptr) = m_currentSample;
	ptr += sizeof(m_currentSample);
	*((unsigned long*)ptr) = m_currentVI;
	ptr += sizeof(m_currentVI);
	*((unsigned long*)ptr) = m_header.length_samples;
	ptr += sizeof(m_header.length_samples);

/// // temp debugging check
///char str [1024];
///sprintf(str, "size_needed=%d, ptr=0x%x, m_inputBuffer=0x%x, m_header.uid=%d, m_currentSample=%d, m_header.length_samples=%d\n", size_needed, ptr, m_inputBuffer, m_header.uid, m_currentSample, m_header.length_samples);
///ShowInfo(str);

	memcpy(ptr, m_inputBuffer, sizeof(BUTTONS) * (m_header.length_samples+1));
}

int VCR_movieUnfreeze (const char* buf, unsigned long size)
{
//	m_intro = FALSE;
	
	// sanity check
	if(VCR_isIdle())
	{
		return NOT_FROM_A_MOVIE; // probably wrong error code
	}

	const char* ptr = buf;
	if(size < sizeof(m_header.uid) + sizeof(m_currentSample) + sizeof(m_currentVI) + sizeof(m_header.length_samples))
	{
		return WRONG_FORMAT;
	}

	unsigned long movie_id = *((unsigned long*)ptr);
	ptr += sizeof(unsigned long);
	unsigned long current_sample = *((unsigned long*)ptr);
	ptr += sizeof(unsigned long);
	unsigned long current_vi = *((unsigned long*)ptr);
	ptr += sizeof(unsigned long);
	unsigned long max_sample = *((unsigned long*)ptr);
	ptr += sizeof(unsigned long);

	unsigned long space_needed = (sizeof(BUTTONS) * (max_sample+1));

	if(movie_id != m_header.uid)
		return NOT_FROM_THIS_MOVIE;

	if(current_sample > max_sample)
		return INVALID_FRAME;

	if(space_needed > size)
		return WRONG_FORMAT;

	int lastTask = m_task;
	if(!m_readOnly)
	{
		// here, we are going to take the input data from the savestate
		// and make it the input data for the current movie, then continue
		// writing new input data at the currentFrame pointer
//		change_state(MOVIE_STATE_RECORD);
		m_task = Recording;
		flush_movie();
///		systemScreenMessage("Movie re-record");

#ifdef __WIN32__
		extern void EnableEmulationMenuItems(BOOL flag);
		if(lastTask == Playback)
			EnableEmulationMenuItems(TRUE);
#else
		// FIXME: how to update enable/disable state of StopPlayback and StopRecord with gtk GUI?
#endif
		// update header with new ROM info
		if(lastTask == Playback)
			setROMInfo(&m_header);

		m_currentSample = current_sample;
		m_header.length_samples = current_sample;
		m_currentVI = current_vi;

		m_header.rerecord_count++;

		reserve_buffer_space(space_needed);
		memcpy(m_inputBuffer, ptr, space_needed);
		flush_movie();
		fseek(m_file, MUP_HEADER_SIZE_CUR+(sizeof(BUTTONS) * (m_currentSample+1)), SEEK_SET);
	}
	else
	{
		// here, we are going to keep the input data from the movie file
		// and simply rewind to the currentFrame pointer
		// this will cause a desync if the savestate is not in sync
		// with the on-disk recording data, but it's easily solved
		// by loading another savestate or playing the movie from the beginning

		// and older savestate might have a currentFrame pointer past
		// the end of the input data, so check for that here
		if(current_sample > m_header.length_samples)
			return INVALID_FRAME;

		m_task = Playback;
		flush_movie();
//		change_state(MOVIE_STATE_PLAY);
///		systemScreenMessage("Movie rewind");

#ifdef __WIN32__
		extern void EnableEmulationMenuItems(BOOL flag);
		if(lastTask == Recording)
			EnableEmulationMenuItems(TRUE);
#else
		// FIXME: how to update enable/disable state of StopPlayback and StopRecord with gtk GUI?
#endif

		m_currentSample = current_sample;
		m_currentVI = current_vi;
	}

	m_inputBufferPtr = m_inputBuffer + (sizeof(BUTTONS) * m_currentSample);

///	for(int controller = 0 ; controller < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS ; controller++)
///		if((m_header.controllerFlags & MOVIE_CONTROLLER(controller)) != 0)
///			read_frame_controller_data(controller);
///	read_frame_controller_data(0); // correct if we can assume the first controller is active, which we can on all GBx/xGB systems

	return SUCCESS;
}


extern BOOL continue_vcr_on_restart_mode;
extern BOOL just_restarted_flag;

void
VCR_getKeys( int Control, BUTTONS *Keys )
{
	if (m_task != Playback && m_task != StartPlayback && m_task != StartPlaybackFromSnapshot) {
		getKeys( Control, Keys );
#ifdef LUA_JOYPAD
			 lastInputLua[Control] = *(DWORD*)Keys;
			 AtInputLuaCallback(Control);
			 if(0 <= Control && Control < 4) {
					 if(rewriteInputFlagLua[Control]) {
						   *(DWORD*)Keys = 
								 lastInputLua[Control] =
								 rewriteInputLua[Control];
							 rewriteInputFlagLua[Control] = false;
					 }
			 }
#endif
	}

	if(Control == 0)
		memcpy(&m_lastController1Keys, Keys, sizeof(unsigned long));

	if (m_task == Idle)
		return;

	if (m_task == StartRecording)
	{
		if(!continue_vcr_on_restart_mode)
		{
			if(just_restarted_flag)
			{
		       just_restarted_flag = FALSE;
		       m_currentSample = 0;
		       m_currentVI = 0;
		       m_task = Recording;
               memset(Keys, 0, sizeof(BUTTONS));
			}
			else
			{
		       printf( "[VCR]: Starting recording...\n" );
		       hardResetAndClearAllSaveData();
			}
		}
///       return;
	}

	if (m_task == StartRecordingFromSnapshot)
	{
		// wait until state is saved, then record
		if ((savestates_job & SAVESTATE) == 0)
		{
			printf( "[VCR]: Starting recording from Snapshot...\n" );
			m_task = Recording;
            memset(Keys, 0, sizeof(BUTTONS));
		}
///		return;
	}

	if (m_task == StartPlayback)
	{
#ifdef __WIN32__
		if(!continue_vcr_on_restart_mode)
		{
			if(just_restarted_flag)
			{
		       just_restarted_flag = FALSE;
		       m_currentSample = 0;
		       m_currentVI = 0;
		       m_task = Playback;
			}
			else
			{
		       printf( "[VCR]: Starting playback...\n" );
		       hardResetAndClearAllSaveData();
			}
		}
#else
       printf( "[VCR]: Starting playback...\n" );
       m_currentSample = 0;
       m_currentVI = 0;
       m_task = Playback;
       hardResetAndClearAllSaveData();
#endif
///		return;
	}

	if (m_task == StartPlaybackFromSnapshot)
	{
		// wait until state is loaded, then playback
		if ((savestates_job & LOADSTATE) == 0)
		{
			extern BOOL savestates_job_success;
			if(!savestates_job_success)
			{
				char str [2048];
				sprintf(str, "Couldn't find or load this movie's snapshot,\n\"%s\".\nMake sure that file is where Mupen64 can find it.", savestates_get_selected_filename());
				printError(str);
				m_task = Idle;
				getKeys( Control, Keys );
				return;
			}
			printf( "[VCR]: Starting playback...\n" );
			m_task = Playback;
		}
///		return;
	}

	if (m_task == Recording)
	{
//		long cont = Control;
//		fwrite( &cont, 1, sizeof (long), m_file ); // write the controller #

		reserve_buffer_space((unsigned long)((m_inputBufferPtr+sizeof(BUTTONS))-m_inputBuffer));
		
		*((BUTTONS*)m_inputBufferPtr) = *Keys;
		m_inputBufferPtr += sizeof(BUTTONS);

//		fwrite( Keys, 1, sizeof (BUTTONS), m_file ); // write the data for this controller (sizeof(BUTTONS) == 4 the last time I checked)
		m_header.length_samples++;
		m_currentSample++;
		
		// flush data every 5 seconds or so
		if((m_header.length_samples % (m_header.num_controllers * 150)) == 0)
		{
			flush_movie();
		}

		return;
	}

	if (m_task == Playback)
	{
//		long cont;
//		fread( &cont, 1, sizeof (long), m_file );
//		if (cont == -1)	// end

		if(m_currentSample >= m_header.length_samples
		|| m_currentVI >= m_header.length_vis)
		{
//			if (m_capture != 0)
//				VCR_stopCapture();
//			else
				VCR_stopPlayback();
				BUTTONS zero = { 0 };
				setKeys(Control,zero);
				getKeys(Control,Keys);
				if (Control == 0)
					memcpy(&m_lastController1Keys, Keys, sizeof(unsigned long));
			return;
		}
//		if (cont != Control)
//		{
//			printf( "[VCR]: Warning - controller num from file doesn't match requested number\n" );
//			// ...
//		}

		if(m_header.controllerFlags & CONTROLLER_X_PRESENT(Control))
		{
			*Keys = *((BUTTONS*)m_inputBufferPtr);
			m_inputBufferPtr += sizeof(BUTTONS);
				setKeys(Control, *Keys);
			
	
#ifdef LUA_JOYPAD
			 lastInputLua[Control] = *(DWORD*)Keys;
			 AtInputLuaCallback(Control);
			 if(0 <= Control && Control < 4) {
					 if(rewriteInputFlagLua[Control]) {
						   *(DWORD*)Keys = 
								 lastInputLua[Control] =
								 rewriteInputLua[Control];
							 rewriteInputFlagLua[Control] = false;
					 }
			 }
#endif
	//		fread( Keys, 1, sizeof (BUTTONS), m_file );
			m_currentSample++;
		}
		else
		{
			memset(Keys, 0, sizeof(BUTTONS));
		}

		if(Control == 0)
			memcpy(&m_lastController1Keys, Keys, sizeof(unsigned long));
			
		return;
	}
}




int
VCR_startRecord( const char *filename, BOOL fromSnapshot, const char *authorUTF8, const char *descriptionUTF8 )
{
	VCR_coreStopped();
	
	char buf[PATH_MAX];
/*
	if (m_task != Idle)
	{
		fprintf( stderr, "[VCR]: Cannot start recording, current task is \"%s\"\n", m_taskName[m_task] );
		return -1;
	}
*/
	strncpy( m_filename, filename, PATH_MAX );

	// open record file
	strcpy( buf, m_filename );
    {
        char* dot = strrchr(buf, '.');
        char* s1 = strrchr(buf, '\\');
        char* s2 = strrchr(buf, '/');
    	if (!dot || ((s1 && s1 > dot) || (s2 && s2 > dot)) ) {
        	strncat( buf, ".m64", PATH_MAX );
        }
    }
	m_file = fopen( buf, "wb" );
	if (m_file == 0)
	{
		fprintf( stderr, "[VCR]: Cannot start recording, could not open file '%s': %s\n", filename, strerror( errno ) );
		return -1;
    }
    
    VCR_setReadOnly(FALSE);

	memset(&m_header, 0, MUP_HEADER_SIZE);

    m_header.magic = MUP_MAGIC;
    m_header.version = MUP_VERSION;
	m_header.uid = (unsigned long)time(NULL);
    m_header.length_vis = 0;
    m_header.length_samples = 0;
	m_header.rerecord_count = 0;
	m_header.startFlags = fromSnapshot ? MOVIE_START_FROM_SNAPSHOT : MOVIE_START_FROM_NOTHING;

	reserve_buffer_space(4096);

    if(fromSnapshot)
    {
    	// save state
    	printf( "[VCR]: Saving state...\n" );
    	strcpy( buf, m_filename );

		// remove extension
		for(;;)
		{
	    	char* dot = strrchr(buf, '.');
	    	if(dot && (dot > strrchr(buf, '\\') && dot > strrchr(buf, '/')))
				*dot = '\0';
			else
				break;
		}

    	strncat( buf, ".st", PATH_MAX );
    	savestates_select_filename( buf );
    	savestates_job |= SAVESTATE;
     	m_task = StartRecordingFromSnapshot;
    } else{
     	m_task = StartRecording;
    }

	setROMInfo(&m_header);

	// utf8 strings are also null-terminated so this method still works
	if(authorUTF8)
		strncpy(m_header.authorInfo, authorUTF8, MOVIE_AUTHOR_DATA_SIZE);
	m_header.authorInfo[MOVIE_AUTHOR_DATA_SIZE-1] = '\0';
	if(descriptionUTF8)
		strncpy(m_header.description, descriptionUTF8, MOVIE_DESCRIPTION_DATA_SIZE);
	m_header.description[MOVIE_DESCRIPTION_DATA_SIZE-1] = '\0';

//int i;
//for(i = 0 ; i < MOVIE_AUTHOR_DATA_SIZE*4 ; i++)
//  ShowInfo("authorUTF8[%d] = %c", i, authorUTF8[i]);
//for(i = 0 ; i < MOVIE_AUTHOR_DATA_SIZE ; i++)
//  ShowInfo("authorWC[%d] = %lc", i, authorWC[i]);

    write_movie_header(m_file, MUP_HEADER_SIZE);

	m_currentSample = 0;
	m_currentVI = 0;

	return 0;

}


int
VCR_stopRecord()
{
	int retVal = -1;
	
	if (m_task == StartRecording)
	{
		char buf[PATH_MAX];

		m_task = Idle;
		if (m_file)
		{
			fclose( m_file );
			m_file = 0;
		}
		printf( "[VCR]: Removing files (nothing recorded)\n" );

		strcpy( buf, m_filename );
		strncat( m_filename, ".st", PATH_MAX );
		if (unlink( buf ) < 0)
			fprintf( stderr, "[VCR]: Couldn't remove save state: %s\n", strerror( errno ) );

		strcpy( buf, m_filename );
		strncat( m_filename, ".m64", PATH_MAX );
		if (unlink( buf ) < 0)
			fprintf( stderr, "[VCR]: Couldn't remove recorded file: %s\n", strerror( errno ) );

		retVal = 0;
	}

	if (m_task == Recording)
	{
//		long end = -1;
		
		setROMInfo(&m_header);
		
		flush_movie();

		m_task = Idle;

//		fwrite( &end, 1, sizeof (long), m_file );
//		fwrite( &m_header.length_samples, 1, sizeof (long), m_file );
		fclose( m_file );
		m_file = NULL;

		truncateMovie();

		printf( "[VCR]: Record stopped. Recorded %ld input samples\n", m_header.length_samples );
		
#ifdef __WIN32__
		extern void EnableEmulationMenuItems(BOOL flag);
		EnableEmulationMenuItems(TRUE);
#else
		// FIXME: how to update enable/disable state of StopPlayback and StopRecord with gtk GUI?
#endif
		
#ifdef __WIN32__
		extern HWND hStatus/*, hStatusProgress*/;
		SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"Stopped recording.");
#endif
		
		retVal = 0;
	}

	if(m_inputBuffer)
	{
		free(m_inputBuffer);
		m_inputBuffer = NULL;
		m_inputBufferPtr = NULL;
		m_inputBufferSize = 0;
	}

	return retVal;
}


int
VCR_startPlayback( const char *filename, const char *authorUTF8, const char *descriptionUTF8 )
{
	VCR_coreStopped();
//	m_intro = TRUE;
	
	char buf[PATH_MAX];
/*
	if (m_task != Idle)
	{
		fprintf( stderr, "[VCR]: Cannot start playback, current task is \"%s\"\n", m_taskName[m_task] );
		return -1;
	}
*/
	strncpy( m_filename, filename, PATH_MAX );
	char *p = strrchr( m_filename, '.' );
	if (p)
	{
		if (!strcasecmp( p, ".m64" ) || !strcasecmp( p, ".st" ))
			*p = '\0';
	}
	// open record file
	strcpy( buf, m_filename );
	m_file = fopen( buf, "rb+" );
	if (m_file == 0 && (m_file = fopen( buf, "rb" )) == 0)
	{
        strncat( buf, ".m64", PATH_MAX );
	    m_file = fopen( buf, "rb+" );
    	if (m_file == 0 && (m_file = fopen( buf, "rb" )) == 0)
    	{
    		fprintf( stderr, "[VCR]: Cannot start playback, could not open .m64 file '%s': %s\n", filename, strerror( errno ) );
    		return -1;
        }
	}

    {
        int code = read_movie_header(m_file, &m_header);
        
        switch(code)
        {
            case SUCCESS:
			{
				char warningStr [8092];
				warningStr[0] = '\0';

				BOOL dontPlay = FALSE;
				
				if(!Controls[0].Present && (m_header.controllerFlags & CONTROLLER_1_PRESENT))
				{
					strcat(warningStr, "Error: You have controller 1 disabled, but it is enabled in the movie file.\nIt cannot play back correctly unless you fix this first (in your input settings).\n");
					dontPlay = TRUE;
				}
				if(Controls[0].Present && !(m_header.controllerFlags & CONTROLLER_1_PRESENT))
					strcat(warningStr, "Warning: You have controller 1 enabled, but it is disabled in the movie file.\nIt might not play back correctly unless you change this first (in your input settings).\n");
				else
				{
					if(Controls[0].Present && (Controls[0].Plugin != PLUGIN_MEMPAK) && (m_header.controllerFlags & CONTROLLER_1_MEMPAK))
						strcat(warningStr, "Warning: Controller 1 has a rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
					if(Controls[0].Present && (Controls[0].Plugin != PLUGIN_RUMBLE_PAK) && (m_header.controllerFlags & CONTROLLER_1_RUMBLE))
						strcat(warningStr, "Warning: Controller 1 has a memory pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
					if(Controls[0].Present && (Controls[0].Plugin != PLUGIN_NONE) && !(m_header.controllerFlags & (CONTROLLER_1_MEMPAK|CONTROLLER_1_RUMBLE)))
						strcat(warningStr, "Warning: Controller 1 does not have a mempak or rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
				}

				if(!Controls[1].Present && (m_header.controllerFlags & CONTROLLER_2_PRESENT))
				{
					strcat(warningStr, "Error: You have controller 2 disabled, but it is enabled in the movie file.\nIt cannot back correctly unless you change this first (in your input settings).\n");
					dontPlay = TRUE;
				}
				if(Controls[1].Present && !(m_header.controllerFlags & CONTROLLER_2_PRESENT))
					strcat(warningStr, "Warning: You have controller 2 enabled, but it is disabled in the movie file.\nIt might not play back correctly unless you fix this first (in your input settings).\n");
				else
				{
					if(Controls[1].Present && (Controls[1].Plugin != PLUGIN_MEMPAK) && (m_header.controllerFlags & CONTROLLER_2_MEMPAK))
						strcat(warningStr, "Warning: Controller 2 has a rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
					if(Controls[1].Present && (Controls[1].Plugin != PLUGIN_RUMBLE_PAK) && (m_header.controllerFlags & CONTROLLER_2_RUMBLE))
						strcat(warningStr, "Warning: Controller 2 has a memory pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
					if(Controls[1].Present && (Controls[1].Plugin != PLUGIN_NONE) && !(m_header.controllerFlags & (CONTROLLER_2_MEMPAK|CONTROLLER_2_RUMBLE)))
						strcat(warningStr, "Warning: Controller 2 does not have a mempak or rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
				}

				if(!Controls[2].Present && (m_header.controllerFlags & CONTROLLER_3_PRESENT))
				{
					strcat(warningStr, "Error: You have controller 3 disabled, but it is enabled in the movie file.\nIt cannot play back correctly unless you change this first (in your input settings).\n");
					dontPlay = TRUE;
				}
				if(Controls[2].Present && !(m_header.controllerFlags & CONTROLLER_3_PRESENT))
					strcat(warningStr, "Warning: You have controller 3 enabled, but it is disabled in the movie file.\nIt might not play back correctly unless you fix this first (in your input settings).\n");
				else
				{
					if(Controls[2].Present && (Controls[2].Plugin != PLUGIN_MEMPAK) && !(m_header.controllerFlags & CONTROLLER_3_MEMPAK))
						strcat(warningStr, "Warning: Controller 3 has a rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
					if(Controls[2].Present && (Controls[2].Plugin != PLUGIN_RUMBLE_PAK) && !(m_header.controllerFlags & CONTROLLER_3_RUMBLE))
						strcat(warningStr, "Warning: Controller 3 has a memory pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
					if(Controls[2].Present && (Controls[2].Plugin != PLUGIN_NONE) && !(m_header.controllerFlags & (CONTROLLER_3_MEMPAK|CONTROLLER_3_RUMBLE)))
						strcat(warningStr, "Warning: Controller 3 does not have a mempak or rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
				}

				if(!Controls[3].Present && (m_header.controllerFlags & CONTROLLER_4_PRESENT))
				{
					strcat(warningStr, "Error: You have controller 4 disabled, but it is enabled in the movie file.\nIt cannot play back correctly unless you change this first (in your input settings).\n");
					dontPlay = TRUE;
				}
				if(Controls[3].Present && !(m_header.controllerFlags & CONTROLLER_4_PRESENT))
					strcat(warningStr, "Error: You have controller 4 enabled, but it is disabled in the movie file.\nIt might not play back correctly unless you fix this first (in your input settings).\n");
				else
				{
					if(Controls[3].Present && (Controls[3].Plugin != PLUGIN_MEMPAK) && !(m_header.controllerFlags & CONTROLLER_4_MEMPAK))
						strcat(warningStr, "Warning: Controller 4 has a rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
					if(Controls[3].Present && (Controls[3].Plugin != PLUGIN_RUMBLE_PAK) && !(m_header.controllerFlags & CONTROLLER_4_RUMBLE))
						strcat(warningStr, "Warning: Controller 4 has a memory pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
					if(Controls[3].Present && (Controls[3].Plugin != PLUGIN_NONE) && !(m_header.controllerFlags & (CONTROLLER_4_MEMPAK|CONTROLLER_4_RUMBLE)))
						strcat(warningStr, "Warning: Controller 4 does not have a mempak or rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n");
				}

				char str [512], name [512];
				extern rom_header *ROM_HEADER;
                if(ROM_HEADER && stricmp(m_header.romNom, (const char*)ROM_HEADER->nom) != 0)
                {
				    sprintf(str, "The movie was recorded with the ROM \"%s\",\nbut you are using the ROM \"%s\",\nso the movie probably won't play properly.\n", m_header.romNom, ROM_HEADER->nom);
					strcat(warningStr, str);
				}
				else 
				{
	                if(ROM_HEADER && m_header.romCountry != ROM_HEADER->Country_code)
	                {
					    sprintf(str, "The movie was recorded with a ROM with country code \"%d\",\nbut you are using a ROM with country code \"%d\",\nso the movie may not play properly.\n", m_header.romCountry, ROM_HEADER->Country_code);
						strcat(warningStr, str);
					}
					else if(ROM_HEADER && m_header.romCRC != ROM_HEADER->CRC1)
	                {
					    sprintf(str, "The movie was recorded with a ROM that has CRC \"0x%x\",\nbut you are using a ROM with CRC \"0x%x\",\nso the movie may not play properly.\n", (unsigned int)m_header.romCRC, (unsigned int)ROM_HEADER->CRC1);
						strcat(warningStr, str);
					}
				}
				
				if(strlen(warningStr) > 0)
				{
					if(dontPlay)
						printError(warningStr);
					else
						printWarning(warningStr);
				}
				
				extern char gfx_name[255];
				extern char input_name[255];
				extern char sound_name[255];
				extern char rsp_name[255];
//			    ROM_INFO* pRomInfo = getSelectedRom(); // FIXME
//			    DEFAULT_ROM_SETTINGS TempRomSettings = GetDefaultRomSettings( pRomInfo->InternalName);
//				if(TempRomSettings.InputPluginName[0])
//					strncpy(name, TempRomSettings.InputPluginName, 64);
//				else
					strncpy(name, input_name, 64);
                if(name[0] && m_header.inputPluginName[0] && stricmp(m_header.inputPluginName, name) != 0)
                {
				    printf("Warning: The movie was recorded with the input plugin \"%s\",\nbut you are using the input plugin \"%s\",\nso the movie may not play properly.", m_header.inputPluginName, name);
				}
//				if(TempRomSettings.GfxPluginName[0])
//					strncpy(name, TempRomSettings.GfxPluginName, 64);
//				else
					strncpy(name, gfx_name, 64);
                if(name[0] && m_header.videoPluginName[0] && stricmp(m_header.videoPluginName, name) != 0)
                {
				    printf("Warning: The movie was recorded with the graphics plugin \"%s\",\nbut you are using the graphics plugin \"%s\",\nso the movie might not play properly.", m_header.videoPluginName, name);
				}
//				if(TempRomSettings.SoundPluginName[0])
//					strncpy(name, TempRomSettings.SoundPluginName, 64);
//				else
					strncpy(name, sound_name, 64);
                if(name[0] && m_header.soundPluginName[0] && stricmp(m_header.soundPluginName, name) != 0)
                {
				    printf("Warning: The movie was recorded with the sound plugin \"%s\",\nbut you are using the sound plugin \"%s\",\nso the movie might not play properly.", m_header.soundPluginName, name);
				}
//				if(TempRomSettings.RspPluginName[0])
//					strncpy(name, TempRomSettings.RspPluginName, 64);
//				else
					strncpy(name, rsp_name, 64);
                if(name[0] && m_header.rspPluginName[0] && stricmp(m_header.rspPluginName, name) != 0)
                {
				    printf("Warning: The movie was recorded with the RSP plugin \"%s\",\nbut you are using the RSP plugin \"%s\",\nso the movie probably won't play properly.", m_header.rspPluginName, name);
				}



				if(dontPlay)
					return -1;


				// recalculate length of movie from the file size
//				fseek(m_file, 0, SEEK_END);
//				int fileSize = ftell(m_file);
//				m_header.length_samples = (fileSize - MUP_HEADER_SIZE) / sizeof(BUTTONS) - 1;

				fseek(m_file, MUP_HEADER_SIZE_CUR, SEEK_SET);

				// read controller data
				m_inputBufferPtr = m_inputBuffer;
				unsigned long to_read = sizeof(BUTTONS) * (m_header.length_samples+1);
				reserve_buffer_space(to_read);
				fread(m_inputBufferPtr, 1, to_read, m_file);

				// read "baseline" controller data
///				read_frame_controller_data(0); // correct if we can assume the first controller is active, which we can on all GBx/xGB systems
//				m_currentSample = 0;

				fseek(m_file, 0, SEEK_END);
				#ifdef _WIN32
				char buf[50];
				sprintf(buf, "%d rr", m_header.rerecord_count);
				extern HWND hStatus;
				SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)buf);
				#endif
			}	break;
            default:
				fprintf( stderr, "[VCR]: Error playing movie: %s.\n", m_errCodeName[code]);
				break;
        }

		m_currentSample = 0;
		m_currentVI = 0;
	
	    if(m_header.startFlags & MOVIE_START_FROM_SNAPSHOT)
	    {
	    	// load state
	    	printf( "[VCR]: Loading state...\n" );
	    	strcpy( buf, m_filename );

			// remove extension
			for(;;)
			{
		    	char* dot = strrchr(buf, '.');
		    	if(dot && (dot > strrchr(buf, '\\') && dot > strrchr(buf, '/')))
					*dot = '\0';
				else
					break;
			}

	    	strncat( buf, ".st", PATH_MAX );
	    	savestates_select_filename( buf );
	    	savestates_job |= LOADSTATE;
	    	m_task = StartPlaybackFromSnapshot;
	    } else {
	    	m_task = StartPlayback;
	    }

		// utf8 strings are also null-terminated so this method still works
		if(authorUTF8)
			strncpy(m_header.authorInfo, authorUTF8, MOVIE_AUTHOR_DATA_SIZE);
		m_header.authorInfo[MOVIE_AUTHOR_DATA_SIZE-1] = '\0';
		if(descriptionUTF8)
			strncpy(m_header.description, descriptionUTF8, MOVIE_DESCRIPTION_DATA_SIZE);
		m_header.description[MOVIE_DESCRIPTION_DATA_SIZE-1] = '\0';

        return code;
	}
}


int
VCR_stopPlayback()
{
	if (m_file && m_task != StartRecording && m_task != Recording)
	{
		fclose( m_file );
		m_file = 0;
	}
	
	if (m_task == StartPlayback)
	{
		m_task = Idle;
		return 0;
	}

	if (m_task == Playback)
	{
		m_task = Idle;
		printf( "[VCR]: Playback stopped (%ld samples played)\n", m_currentSample );

#ifdef __WIN32__
		extern void EnableEmulationMenuItems(BOOL flag);
		EnableEmulationMenuItems(TRUE);

		extern HWND hStatus;
		SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"Stopped playback.");
#else
		// FIXME: how to update enable/disable state of StopPlayback and StopRecord with gtk GUI?
#endif

		return 0;
	}

	return -1;
}






void VCR_invalidatedCaptureFrame()
{
	captureFrameValid = FALSE;
}

void
VCR_updateScreen()
{
//	ShowInfo("VCR_updateScreen()");
	extern int externalReadScreen;
	void *image;
//	static void* lastImage = NULL;
	long width, height;
	static int frame = 0;
	int redraw = 1;

	if (m_capture == 0 || readScreen == 0)
	{
#ifdef __WIN32__
		extern HWND mainHWND;
		extern BOOL manualFPSLimit;
		if(!manualFPSLimit)
		{
			// skip 7/8 frames if fast-forwarding and not capturing to AVI
			frame++;
			if((frame % 8) != 0)
				redraw = 0;
		}
#endif
#ifdef LUA_SPEEDMODE
		if(maximumSpeedMode)redraw = 1;
#endif
		if(redraw) {
			updateScreen();
		}
#ifdef LUA_GUI
		extern HWND mainHWND;
		LuaDCUpdate(redraw);
#endif
//		captureFrameValid = TRUE;
		return;
	}

//  // skip every other frame
//	frame ^= 1;
//	if (!frame)
//		return;

#ifdef LUA_SPEEDMODE
	if(maximumSpeedMode)redraw=0;
#endif

		if(redraw) {
	updateScreen();
		}
#ifdef LUA_GUI
		LuaDCUpdate(redraw);
#endif
//	captureFrameValid = TRUE;
	readScreen( &image, &width, &height );
	if (image == 0)
	{
		fprintf( stderr, "[VCR]: Couldn't read screen (out of memory?)\n" );
		return;
	}

	if(VCRComp_GetSize() > 0x7B9ACA00)
	{
		if(AVIBreakMovie)
		{
			VCRComp_finishFile(1);
			AVIBreakMovie=2;
		}
		else
		{
			VCRComp_finishFile(1);
			AVIBreakMovie=1;
		}
		int	fnlen=strlen(AVIFileName);
		if(AVIBreakMovie==2)
		{
			AVIFileName[fnlen-5]++;
			if(AVIFileName[fnlen-5]==90)
				AVIBreakMovie=1;
		}
		else
		{
			AVIFileName[fnlen+1]=AVIFileName[fnlen];
			AVIFileName[fnlen]=AVIFileName[fnlen-1];
			AVIFileName[fnlen-1]=AVIFileName[fnlen-2];
			AVIFileName[fnlen-2]=AVIFileName[fnlen-3];
			AVIFileName[fnlen-3]=AVIFileName[fnlen-4];
			AVIFileName[fnlen-4]=65;
		}
		VCRComp_startFile( AVIFileName, width, height, visByCountrycode(), 0);
	}

//char str[256];
//sprintf(str, "[VCR]: width = %d, height = %d, image = 0x%x", (int)width, (int)height, image);
//ShowInfo(str);

//	if(captureFrameValid || externalReadScreen)
//	{
//		if(image)
//		{
			if(!VCRComp_addVideoFrame((unsigned char*)image))
			{
//				ShowInfo("Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
				printError("Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
				VCR_stopCapture();
			}
//		}
//	}
//	else
//	{
//		if(lastImage)
//		{
//			image = lastImage;
//			if(!VCRComp_addVideoFrame(image);)
//			{
//				printError("Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
//				VCR_stopCapture();
//			}
//		}
//	}
	
	m_videoFrame += 1.0;

	if(externalReadScreen /*|| (!captureFrameValid && lastImage != image)*/)
	{
		if(image)
			free(image);
	}
/*	else
	{
		if(lastImage != image)
		{
			if(lastImage)
				free(lastImage);
			lastImage = image;
		}
	}*/
//	ShowInfo("VCR_updateScreen() done");
}


void
VCR_aiDacrateChanged( int SystemType )
{
	aiDacrateChanged( SystemType );

	m_audioBitrate = ai_register.ai_bitrate+1;
	switch (SystemType)
	{
	case SYSTEM_NTSC:
		m_audioFreq = 48681812 / (ai_register.ai_dacrate + 1);
		break;
	case SYSTEM_PAL:
		m_audioFreq = 49656530 / (ai_register.ai_dacrate + 1);
		break;
	case SYSTEM_MPAL:
		m_audioFreq = 48628316 / (ai_register.ai_dacrate + 1);
		break;
	}
}

// assumes: len <= writeSize
static void writeSound(char* buf, int len, int minWriteSize, int maxWriteSize, BOOL force)
{
	if((len <= 0 && !force) || len > maxWriteSize)
		return;
//	ShowInfo("writeSound()");

	if(soundBufPos + len > minWriteSize || force)
	{
		int len2 = VCR_getResampleLen( 44100, m_audioFreq, m_audioBitrate, soundBufPos );
		if((len2 % 8) == 0 || len > maxWriteSize)
		{
			static short *buf2 = NULL;
			len2 = VCR_resample( &buf2, 44100, (short*)soundBuf, m_audioFreq, m_audioBitrate, soundBufPos );
			if(len2 > 0)
			{
				if((len2 % 4) != 0)
				{
					printf("[VCR]: Warning: Possible stereo sound error detected.\n");
					fprintf(stderr, "[VCR]: Warning: Possible stereo sound error detected.\n");
				}
				if(!VCRComp_addAudioData((unsigned char*)buf2, len2))
				{
//					ShowInfo("Audio output failure!\nA call to addAudioData() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
					printError("Audio output failure!\nA call to addAudioData() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
					VCR_stopCapture();
				}
			}
//			if(buf2)
//				free(buf2);
			soundBufPos = 0;
		}
	}
	
	if(len > 0)
	{
		memcpy(soundBuf + soundBufPos, (char*)buf, len);
		m_audioFrame += ((len/4)/(float)m_audioFreq)*visByCountrycode();
		soundBufPos += len;
	}
//	ShowInfo("writeSound() done");
}

void VCR_aiLenChanged()
{
	short *p = (short *)((char*)rdram + (ai_register.ai_dram_addr & 0xFFFFFF));
	char* buf = (char*)p;
	int aiLen = ai_register.ai_len;

	aiLenChanged();
	if (m_capture == 0)
		return;

	// hack - mupen64 updates bitrate after calling aiDacrateChanged
	m_audioBitrate = ai_register.ai_bitrate+1;

	if (aiLen > 0)
	{
		int len = aiLen;
		int writeSize = 2*m_audioFreq; // we want (writeSize * 44100 / m_audioFreq) to be an integer
		
/*		
		// TEMP PARANOIA
		if(len > writeSize)
		{
			char str [256];
			printf("Sound AVI Output Failure: %d > %d", len, writeSize);
			fprintf(stderr, "Sound AVI Output Failure: %d > %d", len, writeSize);
			sprintf(str, "Sound AVI Output Failure: %d > %d", len, writeSize);
			printError(str);
			printWarning(str);
			exit(0);
		}
*/
		
		
		{
			float desync = m_videoFrame - m_audioFrame;
			if (desync >= 0.0)
			{
				int len3;
				printf( "[VCR]: Correcting for A/V desynchronization of %+f frames\n", desync );
				len3 = (m_audioFreq/(float)visByCountrycode()) * desync;
				len3 <<= 2;

				int emptySize = len3 > writeSize ? writeSize : len3;
				int i;
				for(i = 0 ; i < emptySize ; i += 4)
					*((long*)(soundBufEmpty + i)) = lastSound;
				while(len3 > writeSize)
				{
					writeSound(soundBufEmpty, writeSize, m_audioFreq, writeSize, FALSE);
					len3 -= writeSize;
				}
				writeSound(soundBufEmpty, len3, m_audioFreq, writeSize, FALSE);
			}
			else if (desync <= -10.0)
			{
				printf( "[VCR]: Waiting from A/V desynchronization of %+f frames\n", desync );
			}
		}	


		writeSound(buf, len, m_audioFreq, writeSize, FALSE);
		
		lastSound = *((long*)(buf + len) - 1);

	}
}






#ifdef __WIN32__
void init_readScreen();
#endif

int
VCR_startCapture( const char *recFilename, const char *aviFilename )
{
#ifdef __WIN32__
	extern BOOL emu_paused;
	BOOL wasPaused = emu_paused;
	if(!emu_paused)
	{
		extern void pauseEmu(BOOL quiet);
		pauseEmu(TRUE);
	}
    init_readScreen();
#endif
	if (readScreen == 0)
	{
		printError("AVI capture failed because the active video plugin does not support ReadScreen()!");
		return -1;
	}

	memset(soundBufEmpty, 0, 44100*2);
	memset(soundBuf, 0, 44100*2);
	lastSound = 0;

	m_videoFrame = 0.0;
	m_audioFrame = 0.0;
	void *dest;
	long width, height;
	readScreen( &dest, &width, &height );
	if (dest)
		free( dest );
	VCRComp_startFile( aviFilename, width, height, visByCountrycode(), 1);
	m_capture = 1;
	strncpy( AVIFileName, aviFilename, PATH_MAX );
/*	if (VCR_startPlayback( recFilename ) < 0)
	{
		printError("Cannot start capture; could not play movie file!\n" );
		return -1;
	}*/

#ifdef __WIN32__
	// disable the toolbar (m_capture==1 causes this call to do that)
	// because a bug means part of it could get captured into the AVI
	extern void EnableToolbar();
	EnableToolbar();

	if(!wasPaused || (m_task == Playback || m_task == StartPlayback || m_task == StartPlaybackFromSnapshot))
	{
		extern void resumeEmu(BOOL quiet);
		resumeEmu(TRUE);
	}
#endif

	VCR_invalidatedCaptureFrame();
	
	printf( "[VCR]: Starting capture...\n" );

	return 0;
}

void
VCR_toggleReadOnly ()
{
	if (m_task == Recording)
	{
		flush_movie();
	}
	VCR_setReadOnly(!m_readOnly);

#ifdef __WIN32__
	extern HWND hStatus/*, hStatusProgress*/;
	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)(m_readOnly ? "read-only" : "read+write"));
#else
	printf("%s\n", m_readOnly ? "read-only" : "read+write");
#endif
}


int
VCR_stopCapture()
{
//	ShowInfo("VCR_stopCapture()");
	m_capture = 0;
	writeSound(NULL, 0, m_audioFreq, m_audioFreq*2, TRUE);
	VCR_invalidatedCaptureFrame();

#ifdef __WIN32__
	// re-enable the toolbar (m_capture==0 causes this call to do that)
	extern void EnableToolbar();
	EnableToolbar();
#else
	usleep( 100000 ); // HACK - is this really necessary?
#endif
//	VCR_stopPlayback();
	VCRComp_finishFile(0);
	AVIBreakMovie = 0;
	printf( "[VCR]: Capture finished.\n" );
//	ShowInfo("VCR_stopCapture() done");
	return 0;
}




void
VCR_coreStopped()
{
    extern BOOL continue_vcr_on_restart_mode;
    if(continue_vcr_on_restart_mode)
	    return;
	    
	switch (m_task)
	{
		case StartRecording:
		case StartRecordingFromSnapshot:
		case Recording:
			VCR_stopRecord();
			break;
		case StartPlayback:
		case StartPlaybackFromSnapshot:
		case Playback:
	        VCR_stopPlayback();
			break;
	}

	if (m_capture != 0)
		VCR_stopCapture();
}

// update frame counter
void VCR_updateFrameCounter ()
{
	// input display
	char inputDisplay [64];
	inputDisplay[0] = '\0';
	{
		BOOL a, b, z, l, r, s, cl, cu, cr, cd, dl, du, dr, dd;
		signed char x, y;
		dr = (m_lastController1Keys & (0x0001)) != 0;
		dl = (m_lastController1Keys & (0x0002)) != 0;
		dd = (m_lastController1Keys & (0x0004)) != 0;
		du = (m_lastController1Keys & (0x0008)) != 0;
		s  = (m_lastController1Keys & (0x0010)) != 0; // start button
		z  = (m_lastController1Keys & (0x0020)) != 0;
		b  = (m_lastController1Keys & (0x0040)) != 0;
		a  = (m_lastController1Keys & (0x0080)) != 0;
		cr = (m_lastController1Keys & (0x0100)) != 0;
		cl = (m_lastController1Keys & (0x0200)) != 0;
		cd = (m_lastController1Keys & (0x0400)) != 0;
		cu = (m_lastController1Keys & (0x0800)) != 0;
		r  = (m_lastController1Keys & (0x1000)) != 0;
		l  = (m_lastController1Keys & (0x2000)) != 0;
		x = ((m_lastController1Keys & (0x00FF0000)) >> 16);
		y = ((m_lastController1Keys & (0xFF000000)) >> 24);

		if(!x && !y)
			strcpy(inputDisplay, "");
		else
		{
			int xamt = (x<0?-x:x) * 99/127; if(!xamt && x) xamt = 1;
			int yamt = (y<0?-y:y) * 99/127; if(!yamt && y) yamt = 1;
			if(x && y)
				sprintf(inputDisplay, "%c%d %c%d ", x<0?'<':'>', xamt, y<0?'v':'^', yamt);
			else if(x)
				sprintf(inputDisplay, "%c%d ", x<0?'<':'>', xamt);
			else //if(y)
				sprintf(inputDisplay, "%c%d ", y<0?'v':'^', yamt);
		}

		if(s) strcat(inputDisplay, "S");
		if(z) strcat(inputDisplay, "Z");
		if(a) strcat(inputDisplay, "A");
		if(b) strcat(inputDisplay, "B");
		if(l) strcat(inputDisplay, "L");
		if(r) strcat(inputDisplay, "R");
		if(cu||cd||cl||cr)
		{
			strcat(inputDisplay, " C");
			if(cu) strcat(inputDisplay, "^");
			if(cd) strcat(inputDisplay, "v");
			if(cl) strcat(inputDisplay, "<");
			if(cr) strcat(inputDisplay, ">");
		}
		if(du||dd||dl||dr)
		{
			strcat(inputDisplay, " D");
			if(du) strcat(inputDisplay, "^");
			if(dd) strcat(inputDisplay, "v");
			if(dl) strcat(inputDisplay, "<");
			if(dr) strcat(inputDisplay, ">");
		}
	}

	char str [128];
	char rr[50];
	if (VCR_isRecording())
	{
		sprintf(str, "%d (%d) %s", (int)m_currentVI, (int)m_currentSample, inputDisplay);
		sprintf(rr, "%d rr", m_header.rerecord_count);
	}
	else if (VCR_isPlaying())
	{
		sprintf(str, "%d/%d (%d/%d) %s", (int)m_currentVI, (int)VCR_getLengthVIs(), (int)m_currentSample, (int)VCR_getLengthSamples(), inputDisplay);
		sprintf(rr, "%d rr", m_header.rerecord_count);
	}
	else
		strcpy(str, inputDisplay);
		
#ifdef __WIN32__
	extern HWND hStatus/*, hStatusProgress*/;

// // oops, this control isn't even visible during emulation...
//	if(VCR_isPlaying())
//		SendMessage( hStatusProgress, PBM_SETPOS, (int)(100.0f * (float)m_currentSample / (float)VCR_getLengthFrames() + 0.5f), 0 );

//	if(m_intro && m_currentSample < 60 && VCR_isPlaying())
//		sprintf(str, "%d re-records, %d frames", (int)m_header.rerecord_count, (int)m_header.length_vis);
	if (VCR_isRecording() || VCR_isPlaying())
		SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)rr);
	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)str);

#else
	fprintf(stderr, "%s\r", str);
#endif
}



#endif // VCR_SUPPORT
