/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// ReSharper disable CppInconsistentNaming
#pragma once

#include "core_plugin.h"

/**
 * An enum containing results that can be returned by the core.
 */
typedef enum {

#pragma region Generic
    // The operation completed successfully
    Res_Ok,

    // The operation was cancelled by the user
    Res_Cancelled,
#pragma endregion

#pragma region VCR
    // The provided data has an invalid format
    VCR_InvalidFormat,
    // The provided file is inaccessible or does not exist
    VCR_BadFile,
    // The cheat data couldn't be written to disk
    VCR_CheatWriteFailed,
    // The controller configuration is invalid
    VCR_InvalidControllers,
    // The movie's savestate is missing or invalid
    VCR_InvalidSavestate,
    // The resulting frame is outside the bounds of the movie
    VCR_InvalidFrame,
    // There is no rom which matches this movie
    VCR_NoMatchingRom,
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
    // The seek operation could not be initiated due to a savestate not being loaded successfully
    VCR_SeekSavestateLoadFailed,
    // The seek operation can't be initiated because the seek savestate interval is 0
    VCR_SeekSavestateIntervalZero,
#pragma endregion

#pragma region VR
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
    // The CPU registers contained invalid values
    ST_InvalidRegisters,
#pragma endregion

#pragma region Plugins
    // The plugin library couldn't be loaded
    Pl_LoadLibraryFailed,
    // The plugin doesn't export a GetDllInfo function
    Pl_NoGetDllInfo,
#pragma endregion
} core_result;

struct core_cfg {
    /// <summary>
    /// Statistic - Amount of state loads during recording
    /// </summary>
    uint64_t total_rerecords;

    /// <summary>
    /// Statistic - Amount of emulated frames
    /// </summary>
    uint64_t total_frames;

    /// <summary>
    /// The currently selected core type
    /// <para/>
    /// 0 - Cached Interpreter
    /// 1 - Dynamic Recompiler
    /// 2 - Pure Interpreter
    /// </summary>
    int32_t core_type = 1;

    /// <summary>
    /// The target FPS modifier as a percentage
    /// <para/>
    /// (100 = default)
    /// </summary>
    int32_t fps_modifier = 100;

    /// <summary>
    /// The frequency at which frames are skipped during fast-forward
    /// <para/>
    /// 0 = skip all frames
    /// 1 = skip no frames
    /// >0 = every nth frame is skipped
    /// </summary>
    int32_t frame_skip_frequency = 8;

    /// <summary>
    /// Whether fast-forward will mute audio
    /// This option improves performance by skipping additional do_rsp_cycles calls, but may cause issues
    /// </summary>
    int32_t fastforward_silent;

    /// <summary>
    /// Maximum number of entries into the rom cache
    /// <para/>
    /// 0 = disabled
    /// </summary>
    int32_t rom_cache_size;

    /// <summary>
    /// Saves video buffer to savestates, slow!
    /// </summary>
    int32_t st_screenshot;

    /// <summary>
    /// Whether a playing movie will loop upon ending
    /// </summary>
    int32_t is_movie_loop_enabled;

    /// <summary>
    /// The CPU's counter factor. Higher values will generate less lag frames in-game at the cost of higher native CPU usage.
    /// </summary>
    int32_t counter_factor = 1;

    /// <summary>
    /// Whether rom resets are not recorded in movies
    /// </summary>
    int32_t is_reset_recording_enabled;

    /// <summary>
    /// Whether undo savestate load functionality is enabled.
    /// </summary>
    int32_t st_undo_load = 1;

    /// <summary>
    /// SD card emulation
    /// </summary>
    int32_t use_summercart;

    /// <summary>
    /// Whether WiiVC emulation is enabled. Causes truncation instead of rounding when performing certain casts.
    /// </summary>
    int32_t wii_vc_emulation;

    /// <summary>
    /// Determines whether floating point exceptions are emulated.
    /// </summary>
    int32_t float_exception_emulation;

    /// <summary>
    /// Whether audio interrupts will be delayed
    /// </summary>
    int32_t is_audio_delay_enabled = 1;

    /// <summary>
    /// Whether jmp instructions will cause the following code to be JIT'd by dynarec
    /// </summary>
    int32_t is_compiled_jump_enabled = 1;

    /// <summary>
    /// The save interval for warp modify savestates in frames
    /// </summary>
    int32_t seek_savestate_interval = 0;

    /// <summary>
    /// The maximum amount of warp modify savestates to keep in memory
    /// </summary>
    int32_t seek_savestate_max_count = 20;

    /// <summary>
    /// The movie frame to automatically pause at
    /// -1 none
    /// </summary>
    int32_t pause_at_frame = -1;

    /// <summary>
    /// Whether to pause at the last movie frame
    /// </summary>
    int32_t pause_at_last_frame;

    /// <summary>
    /// Whether VCR operates in read-only mode
    /// </summary>
    int32_t vcr_readonly;

    /// <summary>
    /// Whether VCR creates movie backups
    /// </summary>
    int32_t vcr_backups = 1;

    /// <summary>
    /// Whether movies are written using the new extended format.
    /// If disabled, the extended format sections are set to 0.
    /// </summary>
    int32_t vcr_write_extended_format = 1;

    /// <summary>
    /// Makes the emulator wait at the last frame of a movie.
    /// </summary>
    int32_t wait_at_movie_end{};
    
    /// <summary>
    /// The maximum amount of VIs allowed to be generated since the last input poll before a warning dialog is shown
    /// 0 - no warning
    /// </summary>
    int32_t max_lag = 480;

    /// <summary>
    /// Whether lag frames will cause updateScreen to be invoked
    /// </summary>
    int32_t skip_rendering_lag;

    /// <summary>
    /// Throttles game rendering to 60 FPS.
    /// </summary>
    int32_t render_throttling = 1;
};

#pragma region Emulator

typedef std::common_type_t<std::chrono::duration<int64_t, std::ratio<1, 1000000000>>, std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> core_timer_delta;
constexpr uint8_t core_timer_max_deltas = 60;

typedef struct {
    uint32_t rdram_config;
    uint32_t rdram_device_id;
    uint32_t rdram_delay;
    uint32_t rdram_mode;
    uint32_t rdram_ref_interval;
    uint32_t rdram_ref_row;
    uint32_t rdram_ras_interval;
    uint32_t rdram_min_interval;
    uint32_t rdram_addr_select;
    uint32_t rdram_device_manuf;
} core_rdram_reg;

typedef struct {
    uint32_t sp_mem_addr_reg;
    uint32_t sp_dram_addr_reg;
    uint32_t sp_rd_len_reg;
    uint32_t sp_wr_len_reg;
    uint32_t w_sp_status_reg;
    uint32_t sp_status_reg;
    char halt;
    char broke;
    char dma_busy;
    char dma_full;
    char io_full;
    char single_step;
    char intr_break;
    char signal0;
    char signal1;
    char signal2;
    char signal3;
    char signal4;
    char signal5;
    char signal6;
    char signal7;
    uint32_t sp_dma_full_reg;
    uint32_t sp_dma_busy_reg;
    uint32_t sp_semaphore_reg;
} core_sp_reg;

typedef struct {
    uint32_t rsp_pc;
    uint32_t rsp_ibist;
} core_rsp_reg;

typedef struct {
    uint32_t dpc_start;
    uint32_t dpc_end;
    uint32_t dpc_current;
    uint32_t w_dpc_status;
    uint32_t dpc_status;
    char xbus_dmem_dma;
    char freeze;
    char flush;
    char start_glck;
    char tmem_busy;
    char pipe_busy;
    char cmd_busy;
    char cbuf_busy;
    char dma_busy;
    char end_valid;
    char start_valid;
    uint32_t dpc_clock;
    uint32_t dpc_bufbusy;
    uint32_t dpc_pipebusy;
    uint32_t dpc_tmem;
} core_dpc_reg;

typedef struct {
    uint32_t dps_tbist;
    uint32_t dps_test_mode;
    uint32_t dps_buftest_addr;
    uint32_t dps_buftest_data;
} core_dps_reg;

typedef struct {
    uint32_t w_mi_init_mode_reg;
    uint32_t mi_init_mode_reg;
    char init_length;
    char init_mode;
    char ebus_test_mode;
    char RDRAM_reg_mode;
    uint32_t mi_version_reg;
    uint32_t mi_intr_reg;
    uint32_t mi_intr_mask_reg;
    uint32_t w_mi_intr_mask_reg;
    char SP_intr_mask;
    char SI_intr_mask;
    char AI_intr_mask;
    char VI_intr_mask;
    char PI_intr_mask;
    char DP_intr_mask;
} core_mips_reg;

typedef struct {
    uint32_t vi_status;
    uint32_t vi_origin;
    uint32_t vi_width;
    uint32_t vi_v_intr;
    uint32_t vi_current;
    uint32_t vi_burst;
    uint32_t vi_v_sync;
    uint32_t vi_h_sync;
    uint32_t vi_leap;
    uint32_t vi_h_start;
    uint32_t vi_v_start;
    uint32_t vi_v_burst;
    uint32_t vi_x_scale;
    uint32_t vi_y_scale;
    uint32_t vi_delay;
} core_vi_reg;

typedef struct {
    uint32_t ai_dram_addr;
    // source address (in rdram) of sound sample to be played
    uint32_t ai_len; // amount of bytes(?) to be played
    uint32_t ai_control;
    uint32_t ai_status; // info about whether dma active and is fifo full
    uint32_t ai_dacrate;
    // clock rate / audio rate, tells sound controller how to interpret the audio samples
    uint32_t ai_bitrate;
    // possible values 2 to 16, bits per sample?, this is always (dacRate / 66)-1 (by libultra)
    uint32_t next_delay;
    uint32_t next_len;
    uint32_t current_delay;
    uint32_t current_len;
} core_ai_reg;

typedef struct {
    uint32_t pi_dram_addr_reg;
    uint32_t pi_cart_addr_reg;
    uint32_t pi_rd_len_reg;
    uint32_t pi_wr_len_reg;
    uint32_t read_pi_status_reg;
    uint32_t pi_bsd_dom1_lat_reg;
    uint32_t pi_bsd_dom1_pwd_reg;
    uint32_t pi_bsd_dom1_pgs_reg;
    uint32_t pi_bsd_dom1_rls_reg;
    uint32_t pi_bsd_dom2_lat_reg;
    uint32_t pi_bsd_dom2_pwd_reg;
    uint32_t pi_bsd_dom2_pgs_reg;
    uint32_t pi_bsd_dom2_rls_reg;
} core_pi_reg;

typedef struct {
    uint32_t ri_mode;
    uint32_t ri_config;
    uint32_t ri_current_load;
    uint32_t ri_select;
    uint32_t ri_refresh;
    uint32_t ri_latency;
    uint32_t ri_error;
    uint32_t ri_werror;
} core_ri_reg;

typedef struct {
    uint32_t si_dram_addr;
    uint32_t si_pif_addr_rd64b;
    uint32_t si_pif_addr_wr64b;
    uint32_t si_status;
} core_si_reg;

/**
 * \brief Represents a system type.
 */
typedef enum {
    sys_ntsc,
    sys_pal,
} core_system_type;

typedef struct {
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
} core_rom_header;

#pragma endregion

#pragma region VCR


enum {
    MOVIE_START_FROM_SNAPSHOT = (1 << 0),
    MOVIE_START_FROM_NOTHING = (1 << 1),
    MOVIE_START_FROM_EEPROM = (1 << 2),
    MOVIE_START_FROM_EXISTING_SNAPSHOT = (1 << 3)
};

#define CONTROLLER_X_PRESENT(x) (1 << (x))
#define CONTROLLER_X_MEMPAK(x) (1 << ((x) + 4))
#define CONTROLLER_X_RUMBLE(x) (1 << ((x) + 8))

#pragma pack(push, 1)
/**
 * The movie flag bitfield for extended movies.
 */
typedef union ExtendedMovieFlags {
    uint8_t data;

    struct
    {
        /**
         * Whether the movie was recorded with WiiVC mode enabled.
         */
        bool wii_vc : 1;
        bool unused_1 : 1;
        bool unused_2 : 1;
        bool unused_3 : 1;
        bool unused_4 : 1;
        bool unused_5 : 1;
        bool unused_6 : 1;
        bool unused_7 : 1;
    };
} core_vcr_extended_movie_flags;

/**
 * Additional data for extended movies. Must be 32 bytes large.
 */
typedef struct ExtendedMovieData {
    /**
     * Special authorship information, such as the program which created the movie.
     */
    uint8_t authorship_tag[4] = {0x4D, 0x55, 0x50, 0x4E};

    /**
     * Additional data regarding bruteforcing.
     */
    uint32_t bruteforce_extra_data;

    /**
     * The high word of the amount of loaded states during recording.
     */
    uint32_t rerecord_count;

    uint64_t unused_1;
    uint64_t unused_2;
    uint32_t unused_3;
} t_extended_movie_data;

/**
 * \brief
 */
typedef struct CoreVCRMovieHeader {
    /**
     * \brief <c>M64\0x1a</c>
     */
    uint32_t magic;

    /**
     * \brief Currently <c>3</c>
     */
    uint32_t version;

    /**
     * \brief Movie creation time, used to correlate savestates to a movie
     */
    uint32_t uid;

    /**
     * \brief Amount of VIs in the movie
     */
    uint32_t length_vis;

    /**
     * \brief The low word of the amount of loaded states during recording.
     */
    uint32_t rerecord_count;

    /**
     * \brief The VIs per second of the rom this movie was recorded on
     * \remarks (unused)
     * \remarks This field makes no sense, as VI/s are already known via ROM cc
     */
    unsigned char vis_per_second;

    /**
     * \brief The amount of controllers connected during recording
     * \remarks (pretty much unused)
     * \remarks This field makes no sense, as this value can already be figured out by checking controller flags post-load
     */
    unsigned char num_controllers;

    /**
     * The extended format version. Old movies have it set to <c>0</c>.
     */
    uint8_t extended_version = 1;

    /**
     * The extended movie flags. Only valid if <c>extended_version != 0</c>.
     */
    core_vcr_extended_movie_flags extended_flags;

    /**
     * \brief Amount of input samples in the movie, ideally equal to <c>(file_length - 1024) / 4</c>
     */
    uint32_t length_samples;

    /**
     * \brief What state the movie is expected to start from
     * \remarks vcr.h:32
     */
    uint16_t startFlags;

    uint16_t reserved2;

    /**
     * \brief Bitfield of flags for each controller. Sequence of present, mempak and rumble bits with per-controller values respectively (c1,c2,c3,c4,r1,...)
     */
    uint32_t controller_flags;

    /**
     * The extended movie data. Only valid if <c>extended_version != 0</c>.
     */
    t_extended_movie_data extended_data{};

    /**
     * \brief The name of the movie's author
     * \remarks Used in .rec (version 2)
     */
    char old_author_info[48];

    /**
     * \brief A description of what the movie is about
     * \remarks Used in .rec (version 2)
     */
    char old_description[80];

    /**
     * \brief The internal name of the rom the movie was recorded on
     */
    char rom_name[32];

    /**
     * \brief The primary checksum of the rom the movie was recorded on
     */
    uint32_t rom_crc1;

    /**
     * \brief The rom country of the rom the movie was recorded on
     */
    uint16_t rom_country;

    char reserved_bytes[56];

    /**
     * \brief The name of the video plugin the movie was recorded with, as specified by the plugin
     */
    char video_plugin_name[64];

    /**
     * \brief The name of the audio plugin the movie was recorded with, as specified by the plugin
     */
    char audio_plugin_name[64];

    /**
     * \brief The name of the input plugin the movie was recorded with, as specified by the plugin
     */
    char input_plugin_name[64];

    /**
     * \brief The name of the RSP plugin the movie was recorded with, as specified by the plugin
     */
    char rsp_plugin_name[64];

    /**
     * \brief The name of the movie's author as a UTF-8 string
     */
    char author[222];

    /**
     * \brief A description of what the movie is about as a UTF-8 string
     */
    char description[256];
} core_vcr_movie_header;
#pragma pack(pop)

typedef enum {
    task_idle,
    task_start_recording_from_reset,
    task_start_recording_from_snapshot,
    task_start_recording_from_existing_snapshot,
    task_recording,
    task_start_playback_from_reset,
    task_start_playback_from_snapshot,
    task_playback
} core_vcr_task;

/**
 * \brief The movie freeze buffer, which is used to store the movie (with only essential data) associated with a savestate inside the savestate.
 */
typedef struct {
    uint32_t size;
    uint32_t uid;
    uint32_t current_sample;
    uint32_t current_vi;
    uint32_t length_samples;
    std::vector<core_buttons> input_buffer;
} core_vcr_freeze_info;

#pragma endregion

#pragma region Debugger

typedef struct
{
    uint32_t opcode;
    uint32_t address;
} core_dbg_cpu_state;

#pragma endregion

#pragma region Cheats

/**
 * \brief Represents a cheat.
 */
typedef struct CoreCheat {
    // The script's name. FIXME: This should be read from the script
    std::wstring name = L"Unnamed Cheat";

    // The script's code. Mutating after creation does nothing unless instructions are recompiled.
    std::wstring code;

    // Whether the cheat is active.
    bool active = true;

    // The cheat's instructions. The pair's 1st element tells us whether instruction is a conditional, which is required for special handling of buggy kaze blj anywhere code
    std::vector<std::tuple<bool, std::function<bool()>>> instructions;
} core_cheat;

#pragma endregion

#pragma region Savestates

typedef enum {
    // A save operation
    core_st_job_save,
    // A load operation
    core_st_job_load
} core_st_job;

typedef enum {
    // The target medium is a file with a path.
    core_st_medium_path,
    // The target medium is in-memory.
    core_st_medium_memory,
} core_st_medium;

struct core_st_job_params {
    /// The path to the savestate file.
    /// Valid if the task's medium is <see cref="e_st_medium::path"/>.
    std::filesystem::path path{};

    /// The buffer containing the savestate data.
    /// Valid if the task's medium is <see cref="e_st_medium::memory"/> and the job is <see cref="e_st_job::load"/>.
    std::vector<uint8_t> buffer{};
};

struct core_st_callback_info {
    core_result result{};
    core_st_job job{};
    core_st_medium medium{};
    core_st_job_params params{};
};

using core_st_callback = std::function<void(const core_st_callback_info&, const std::vector<uint8_t>&)>;

#pragma endregion

#pragma region Host API Types

/**
 * The tone of a dialog.
 */
typedef enum {
    fsvc_error,
    fsvc_warning,
    fsvc_information
} core_dialog_type;

#pragma endregion
