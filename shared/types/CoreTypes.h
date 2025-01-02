// ReSharper disable CppInconsistentNaming
#pragma once

/**
 * An enum containing results that can be returned by the core.
 */
enum class CoreResult {
  // TODO: Maybe unify all Busy and Cancelled results?

  // The operation completed successfully
  Ok,

#pragma region VCR
  // The provided data has an invalid format
  VCR_InvalidFormat,
  // The provided file is inaccessible or does not exist
  VCR_BadFile,
  // The user cancelled the operation
  VCR_Cancelled,
  // The controller configuration is invalid
  VCR_InvalidControllers,
  // The movie's savestate is missing or invalid
  VCR_InvalidSavestate,
  // The resulting frame is outside the bounds of the movie
  VCR_InvalidFrame,
  // There is no rom which matches this movie
  VCR_NoMatchingRom,
  // The callee is already performing another task
  VCR_Busy,
  // The VCR engine is idle, but must be active to complete this operation
  VCR_Idle,
  // The provided freeze buffer is not from the currently active movie
  VCR_NotFromThisMovie,
  // The movie's version is invalid
  VCR_InvalidVersion,
  // The movie's extended version is invalid
  VCR_InvalidExtendedVersion,
  // The operation requires a playback or recording task
  VCR_NeedsPlaybackOrRecording,
  // The provided start type is invalid.
  VCR_InvalidStartType,
  // Another warp modify operation is already running
  VCR_WarpModifyAlreadyRunning,
  // Warp modifications can only be performed during recording
  VCR_WarpModifyNeedsRecordingTask,
  // The provided input buffer is empty
  VCR_WarpModifyEmptyInputBuffer,
  // Another seek operation is already running
  VCR_SeekAlreadyRunning,
  // The seek operation could not be initiated due to a savestate not being
  // loaded successfully
  VCR_SeekSavestateLoadFailed,
  // The seek operation can't be initiated because the seek savestate interval
  // is 0
  VCR_SeekSavestateIntervalZero,
#pragma endregion

#pragma region VR
  // The callee is already performing another task
  VR_Busy,
  // Couldn't find a rom matching the provided movie
  VR_NoMatchingRom,
  // An error occured during plugin loading
  VR_PluginError,
  // The ROM or alternative rom source is invalid
  VR_RomInvalid,
  // The emulator isn't running yet
  VR_NotRunning,
  // Failed to open core streams
  VR_FileOpenFailed,
#pragma endregion

#pragma region Savestates
  // The core isn't launched
  ST_CoreNotLaunched,
  // The savestate file wasn't found
  ST_NotFound,
  // The savestate couldn't be written to disk
  ST_FileWriteError,
  // Couldn't decompress the savestate
  ST_DecompressionError,
  // The event queue was too long
  ST_EventQueueTooLong,
  // The user cancelled the operation
  ST_Cancelled,
#pragma endregion

#pragma region Plugins
  // The plugin library couldn't be loaded
  Pl_LoadLibraryFailed,
  // The plugin doesn't export a GetDllInfo function
  Pl_NoGetDllInfo,
#pragma endregion
};

enum class PluginType {
  Video = 2,
  Audio = 3,
  Input = 4,
  RSP = 1,
};

enum class ControllerExtension {
  None = 1,
  Mempak = 2,
  Rumblepak = 3,
  Transferpak = 4,
  Raw = 5
};

enum class SystemType { NTSC, PAL, MPAL };

typedef struct {
  uint16_t Version;
  uint16_t Type;
  char Name[100];

  /* If DLL supports memory these memory options then set them to TRUE or FALSE
     if it does not support it */
  int32_t NormalMemory;  /* a normal uint8_t array */
  int32_t MemoryBswaped; /* a normal uint8_t array where the memory has been pre
                                                   bswap on a dword (32 bits)
                            boundry */
} PLUGIN_INFO;

typedef struct {
  void *hInst;
  // Whether the memory has been pre-byteswapped on a uint32_t boundary
  int32_t MemoryBswaped;
  uint8_t *RDRAM;
  uint8_t *DMEM;
  uint8_t *IMEM;

  uint32_t *MI_INTR_REG;

  uint32_t *SP_MEM_ADDR_REG;
  uint32_t *SP_DRAM_ADDR_REG;
  uint32_t *SP_RD_LEN_REG;
  uint32_t *SP_WR_LEN_REG;
  uint32_t *SP_STATUS_REG;
  uint32_t *SP_DMA_FULL_REG;
  uint32_t *SP_DMA_BUSY_REG;
  uint32_t *SP_PC_REG;
  uint32_t *SP_SEMAPHORE_REG;

  uint32_t *DPC_START_REG;
  uint32_t *DPC_END_REG;
  uint32_t *DPC_CURRENT_REG;
  uint32_t *DPC_STATUS_REG;
  uint32_t *DPC_CLOCK_REG;
  uint32_t *DPC_BUFBUSY_REG;
  uint32_t *DPC_PIPEBUSY_REG;
  uint32_t *DPC_TMEM_REG;

  void(__cdecl *CheckInterrupts)(void);
  void(__cdecl *ProcessDlistList)(void);
  void(__cdecl *ProcessAlistList)(void);
  void(__cdecl *ProcessRdpList)(void);
  void(__cdecl *ShowCFB)(void);
} RSP_INFO;

typedef struct {
  void *hWnd; /* Render window */
  void *hStatusBar;
  /* if render window does not have a status bar then this is NULL */

  int32_t MemoryBswaped; // If this is set to TRUE, then the memory has been pre
  //   bswap on a dword (32 bits) boundry
  //	eg. the first 8 bytes are stored like this:
  //        4 3 2 1   8 7 6 5

  uint8_t *HEADER; // This is the rom header (first 40h bytes of the rom
  // This will be in the same memory format as the rest of the memory.
  uint8_t *RDRAM;
  uint8_t *DMEM;
  uint8_t *IMEM;

  uint32_t *MI_INTR_REG;

  uint32_t *DPC_START_REG;
  uint32_t *DPC_END_REG;
  uint32_t *DPC_CURRENT_REG;
  uint32_t *DPC_STATUS_REG;
  uint32_t *DPC_CLOCK_REG;
  uint32_t *DPC_BUFBUSY_REG;
  uint32_t *DPC_PIPEBUSY_REG;
  uint32_t *DPC_TMEM_REG;

  uint32_t *VI_STATUS_REG;
  uint32_t *VI_ORIGIN_REG;
  uint32_t *VI_WIDTH_REG;
  uint32_t *VI_INTR_REG;
  uint32_t *VI_V_CURRENT_LINE_REG;
  uint32_t *VI_TIMING_REG;
  uint32_t *VI_V_SYNC_REG;
  uint32_t *VI_H_SYNC_REG;
  uint32_t *VI_LEAP_REG;
  uint32_t *VI_H_START_REG;
  uint32_t *VI_V_START_REG;
  uint32_t *VI_V_BURST_REG;
  uint32_t *VI_X_SCALE_REG;
  uint32_t *VI_Y_SCALE_REG;

  void(__cdecl *CheckInterrupts)(void);
} GFX_INFO;

typedef struct {
  void *hwnd;
  void *hinst;

  int32_t MemoryBswaped; // If this is set to TRUE, then the memory has been pre
  //   bswap on a dword (32 bits) boundry
  //	eg. the first 8 bytes are stored like this:
  //        4 3 2 1   8 7 6 5
  uint8_t *HEADER; // This is the rom header (first 40h bytes of the rom
  // This will be in the same memory format as the rest of the memory.
  uint8_t *RDRAM;
  uint8_t *DMEM;
  uint8_t *IMEM;

  uint32_t *MI_INTR_REG;

  uint32_t *AI_DRAM_ADDR_REG;
  uint32_t *AI_LEN_REG;
  uint32_t *AI_CONTROL_REG;
  uint32_t *AI_STATUS_REG;
  uint32_t *AI_DACRATE_REG;
  uint32_t *AI_BITRATE_REG;

  void(__cdecl *CheckInterrupts)(void);
} AUDIO_INFO;

typedef struct {
  int32_t Present;
  int32_t RawData;
  int32_t Plugin;
} CONTROL;

typedef union {
  uint32_t Value;

  struct {
    unsigned R_DPAD : 1;
    unsigned L_DPAD : 1;
    unsigned D_DPAD : 1;
    unsigned U_DPAD : 1;
    unsigned START_BUTTON : 1;
    unsigned Z_TRIG : 1;
    unsigned B_BUTTON : 1;
    unsigned A_BUTTON : 1;

    unsigned R_CBUTTON : 1;
    unsigned L_CBUTTON : 1;
    unsigned D_CBUTTON : 1;
    unsigned U_CBUTTON : 1;
    unsigned R_TRIG : 1;
    unsigned L_TRIG : 1;
    unsigned Reserved1 : 1;
    unsigned Reserved2 : 1;

    signed Y_AXIS : 8;

    signed X_AXIS : 8;
  };
} BUTTONS;

typedef struct {
  void *hMainWindow;
  void *hinst;

  int32_t MemoryBswaped; // If this is set to TRUE, then the memory has been pre
  //   bswap on a dword (32 bits) boundry, only effects header.
  //	eg. the first 8 bytes are stored like this:
  //        4 3 2 1   8 7 6 5
  uint8_t *HEADER;   // This is the rom header (first 40h bytes of the rom)
  CONTROL *Controls; // A pointer to an array of 4 controllers .. eg:
                     // CONTROL Controls[4];
} CONTROL_INFO;

typedef struct s_rom_header {
  uint8_t init_PI_BSB_DOM1_LAT_REG;
  uint8_t init_PI_BSB_DOM1_PGS_REG;
  uint8_t init_PI_BSB_DOM1_PWD_REG;
  uint8_t init_PI_BSB_DOM1_PGS_REG2;
  uint32_t ClockRate;
  uint32_t PC;
  uint32_t Release;
  uint32_t CRC1;
  uint32_t CRC2;
  uint32_t Unknown[2];
  uint8_t nom[20];
  uint32_t unknown;
  uint32_t Manufacturer_ID;
  uint16_t Cartridge_ID;
  uint16_t Country_code;
  uint32_t Boot_Code[1008];
} t_rom_header;
