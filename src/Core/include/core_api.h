/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Describes the Mupen64 Core API.
 */

#pragma once

#include "core_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * \brief Callbacks for the core to call into the host.
     */
    struct core_callbacks
    {
        std::function<void()> vi = [] {};
        std::function<void(core_buttons *input, int index)> input = [](core_buttons *, int) {};
        std::function<void()> frame = [] {};
        std::function<void()> interval = [] {};
        std::function<void()> ai_len_changed = [] {};
        std::function<void()> play_movie = [] {};
        std::function<void()> stop_movie = [] {};
        std::function<void()> loop_movie = [] {};
        std::function<void()> save_state = [] {};
        std::function<void()> load_state = [] {};
        std::function<void()> reset = [] {};
        std::function<void()> seek_completed = [] {};
        std::function<void(bool)> core_executing_changed = [](bool) {};
        std::function<void(bool)> emu_paused_changed = [](bool) {};
        std::function<void(bool)> emu_launched_changed = [](bool) {};
        std::function<void(bool)> emu_starting_changed = [](bool) {};
        std::function<void()> emu_starting = [] {};
        std::function<void()> emu_stopped = [] {};
        std::function<void()> emu_stopping = [] {};
        std::function<void()> reset_completed = [] {};
        std::function<void(int32_t)> speed_modifier_changed = [](int32_t) {};
        std::function<void(bool)> warp_modify_status_changed = [](bool) {};
        std::function<void(int32_t)> current_sample_changed = [](int32_t) {};
        std::function<void(core_vcr_task)> task_changed = [](core_vcr_task) {};
        std::function<void(uint64_t)> rerecords_changed = [](uint64_t) {};
        std::function<void()> unfreeze_completed = [] {};
        std::function<void(size_t)> seek_savestate_changed = [](size_t) {};
        std::function<void(bool)> readonly_changed = [](bool) {};
        std::function<void(core_system_type)> dacrate_changed = [](core_system_type) {};
        std::function<void(bool)> debugger_resumed_changed = [](bool) {};
        std::function<void(core_dbg_cpu_state *)> debugger_cpu_state_changed = [](core_dbg_cpu_state *) {};
        std::function<void()> lag_limit_exceeded = [] {};
        std::function<void()> seek_status_changed = [] {};
    };

#pragma region Dialog IDs

#define CORE_DLG_FLOAT_EXCEPTION "CORE_DLG_FLOAT_EXCEPTION"
#define CORE_DLG_ST_HASH_MISMATCH "CORE_DLG_ST_HASH_MISMATCH"
#define CORE_DLG_ST_UNFREEZE_WARNING "CORE_DLG_ST_UNFREEZE_WARNING"
#define CORE_DLG_ST_NOT_FROM_MOVIE "CORE_DLG_ST_NOT_FROM_MOVIE"
#define CORE_DLG_VCR_RAWDATA_WARNING "CORE_DLG_VCR_RAWDATA_WARNING"
#define CORE_DLG_VCR_WIIVC_WARNING "CORE_DLG_VCR_WIIVC_WARNING"
#define CORE_DLG_VCR_ROM_NAME_WARNING "CORE_DLG_VCR_ROM_NAME_WARNING"
#define CORE_DLG_VCR_ROM_CCODE_WARNING "CORE_DLG_VCR_ROM_CCODE_WARNING"
#define CORE_DLG_VCR_ROM_CRC_WARNING "CORE_DLG_VCR_ROM_CRC_WARNING"
#define CORE_DLG_VCR_CHEAT_LOAD_ERROR "CORE_DLG_VCR_CHEAT_LOAD_ERROR"

#pragma endregion

    /**
     * \brief Represents parameters passed to the core when creating it.
     */
    struct core_params
    {
        /**
         * \brief The core's configuration.
         */
        core_cfg *cfg;

        /**
         * \brief An IIOHelperService implementation.
         */
        // PlatformService *io_service;

        /**
         * \brief The core callbacks.
         */
        core_callbacks callbacks;

        core_controller controls[4]{};

        /**
         * \brief Logs the specified message at the trace level.
         */
        void (*log_trace)(std::string_view);

        /**
         * \brief Logs the specified message at the info level.
         */
        void (*log_info)(std::string_view);

        /**
         * \brief Logs the specified message at the warning level.
         */
        void (*log_warn)(std::string_view);

        /**
         * \brief Logs the specified message at the error level.
         */
        void (*log_error)(std::string_view);

        /**
         * \brief Loads the plugins specified by the config paths.
         * \return Whether the plugins were loaded successfully.
         */
        bool (*load_plugins)(void);

        /**
         * \brief Called after load_plugins, this function loads plugin functions into plugin_funcs and calls the
         * "initiate" family of functions for all plugins. \remark This function must be infallible.
         */
        void (*initiate_plugins)(void);

        /**
         * \brief Executes a function asynchronously.
         * \param func The function to be executed.
         */
        void (*submit_task)(const std::function<void()> &func);

        /**
         * \brief Gets the directory in which savestates and persistent game saves should be stored.
         */
        std::filesystem::path (*get_saves_directory)(void);

        /**
         * \brief Gets the directory in which VCR backups should be stored.
         */
        std::filesystem::path (*get_backups_directory)(void);

        /**
         * \brief Gets the path to the summercart directory.
         */
        std::filesystem::path (*get_summercart_directory)(void);

        /**
         * \brief Gets the path to the summercart vhd.
         */
        std::filesystem::path (*get_summercart_path)(void);

        /**
         * Prompts the user to select from a provided collection of choices.
         * \param id The dialog's unique identifier. Used for correlating a user's choice with a dialog.
         * \param choices The collection of choices.
         * \param str The dialog content.
         * \param title The dialog title.
         * \return The index of the chosen choice. If the user has chosen to not use modals, this function will return
         * the index specified by the user's preferences in the view. If the user has chosen to not show the dialog
         * again, this function will return the last choice.
         */
        std::function<size_t(std::string_view id, const std::vector<std::string> &choices, const char *str,
                             const char *title, core_dialog_type type)>
            show_multiple_choice_dialog;

        /**
         * \brief Asks the user a Yes/No question.
         * \param id The dialog's unique identifier. Used for correlating a user's choice with a dialog.
         * \param str The dialog content.
         * \param title The dialog title.
         * \param warning Whether the tone of the message is perceived as a warning.
         * \return Whether the user answered Yes. If the user has chosen to not use modals, this function will return
         * the value specified by the user's preferences in the view. If the user has chosen to not show the dialog
         * again, this function will return the last choice.
         */
        std::function<bool(std::string_view id, const char *str, const char *title, bool warning)> show_ask_dialog;

        /**
         * \brief Shows the user a dialog.
         * \param str The dialog content.
         * \param title The dialog title.
         * \param type The dialog's tone.
         */
        std::function<void(const char *str, const char *title, core_dialog_type type)> show_dialog;

        /**
         * \brief Shows text in the notification section of the statusbar.
         */
        std::function<void(const char *str)> show_statusbar;

        /**
         * \brief Updates the screen.
         */
        void (*update_screen)(void);

        /**
         * \brief Writes the MGE compositor's current emulation front buffer into the destination buffer.
         * \param buffer The video buffer. Must be at least of size <c>width * height * 3</c>, as acquired by
         * <c>plugin_funcs.get_video_size</c>.
         */
        void (*copy_video)(void *buffer);

        /**
         * \brief Finds the first rom from the available ROM list which matches the predicate.
         * \param predicate A predicate which determines if the rom matches.
         * \return The rom's path, or an empty string if no rom was found.
         */
        std::filesystem::path (*find_available_rom)(const std::function<bool(const core_rom_header &)> &predicate);

        /**
         * \return Whether MGE functionality is currently available.
         */
        bool (*mge_available)(void);

        /**
         * \brief Fills the screen with the specified data.
         * The size of the buffer is determined by the resolution returned by the get_video_size (MGE) or readScreen
         * (Non-MGE) functions. Note that the buffer format is 24bpp.
         */
        void (*load_screen)(void *data);

        /**
         * \brief Gets the plugin names.
         * \param video Pointer to the video plugin name buffer. Destination must be at least 64 bytes large. If null,
         * no data will be written. \param audio Pointer to the audio plugin name buffer. Destination must be at least
         * 64 bytes large. If null, no data will be written. \param input Pointer to the input plugin name buffer.
         * Destination must be at least 64 bytes large. If null, no data will be written. \param rsp Pointer to the RSP
         * plugin name buffer. Destination must be at least 64 bytes large. If null, no data will be written. \note Must
         * be called after loading plugins and their globals.
         */
        void (*get_plugin_names)(char *video, char *audio, char *input, char *rsp);

        /**
         * \brief The savestate callback wrapper, which is invoked prior to individual savestate callbacks.
         * Can be used to display generic error information.
         */
        void (*st_pre_callback)(const core_st_callback_info &info, const std::vector<uint8_t> &buffer) =
            [](const core_st_callback_info &, const std::vector<uint8_t> &) {};

        PROCESSDLIST video_process_dlist;
        PROCESSRDPLIST video_process_rdp_list;
        SHOWCFB video_show_cfb;
        VISTATUSCHANGED video_vi_status_changed;
        VIWIDTHCHANGED video_vi_width_changed;
        GETVIDEOSIZE video_get_video_size;
        FBREAD video_fb_read;
        FBWRITE video_fb_write;
        FBGETFRAMEBUFFERINFO video_fb_get_frame_buffer_info;

        AIDACRATECHANGED audio_ai_dacrate_changed;
        AILENCHANGED audio_ai_len_changed;
        AIREADLENGTH audio_ai_read_length;
        PROCESSALIST audio_process_alist;
        AIUPDATE audio_ai_update;

        CONTROLLERCOMMAND input_controller_command;
        GETKEYS input_get_keys;
        SETKEYS input_set_keys;
        READCONTROLLER input_read_controller;

        DORSPCYCLES rsp_do_rsp_cycles;
    };

    struct core_ctx
    {
        uint8_t *rom;
        uint32_t *rdram;
        core_rdram_reg *rdram_register;
        core_pi_reg *pi_register;
        core_mips_reg *MI_register;
        core_sp_reg *sp_register;
        core_si_reg *si_register;
        core_vi_reg *vi_register;
        core_rsp_reg *rsp_register;
        core_ri_reg *ri_register;
        core_ai_reg *ai_register;
        core_dpc_reg *dpc_register;
        core_dps_reg *dps_register;
        uint32_t *SP_DMEM;
        uint32_t *SP_IMEM;
        uint32_t *PIF_RAM;

#pragma region Emulator

        /**
         * \brief Performs an in-place endianness correction on a rom's header.
         * \param rom The rom buffer, which must be at least 131 bytes large.
         */
        std::function<void(uint8_t *rom)> vr_byteswap;

        /**
         * \brief Gets the currently loaded rom's path. If no rom is loaded, the function returns an empty string.
         */
        std::function<std::filesystem::path()> vr_get_rom_path;

        /**
         * \brief Gets the number of lag frames.
         */
        std::function<size_t()> vr_get_lag_count;

        /**
         * \brief Gets whether the core is currently executing.
         */
        std::function<bool()> vr_get_core_executing;

        /**
         * \brief Gets whether the emu is launched.
         */
        std::function<bool()> vr_get_launched;

        /**
         * \brief Gets whether the core is currently frame-advancing.
         */
        std::function<bool()> vr_get_frame_advance;

        /**
         * \brief Gets whether the core is currently paused.
         */
        std::function<bool()> vr_get_paused;

        /**
         * \brief Pauses the emulator.
         */
        std::function<void()> vr_pause_emu;

        /**
         * \brief Resumes the emulator.
         */
        std::function<void()> vr_resume_emu;

        /**
         * \brief Increments the wait counter.
         * \remarks If the counter is greater than 0, the core will wait for the counter to be 0 before continuing to
         * the next frame. This function is thread-safe.
         */
        std::function<void()> vr_wait_increment;

        /**
         * \brief Decrements the wait counter.
         * \remarks If the counter is greater than 0, the core will wait for the counter to be 0 before continuing to
         * the next frame. This function is thread-safe.
         */
        std::function<void()> vr_wait_decrement;

        /**
         * \brief Starts the emulator.
         * \param path Path to a rom or corresponding movie file.
         * \return The operation result.
         * \remarks This function must be called from a thread that isn't interlocked with the emulator thread.
         */
        std::function<core_result(std::filesystem::path path)> vr_start_rom;

        /**
         * \brief Stops the emulator.
         * \param stop_vcr Whether all VCR operations will be stopped. When resetting the ROM due to an in-movie
         * restart, this needs to be false. \return The operation result. \remarks This function must be called from a
         * thread that isn't interlocked with the emulator thread.
         */
        std::function<core_result(bool stop_vcr)> vr_close_rom;

        /**
         * \brief Resets the emulator.
         * \param reset_save_data Whether save data (e.g.: EEPROM, SRAM, Mempak) will be reset.
         * \param stop_vcr Whether all VCR operations will be stopped. When resetting the ROM due to an in-movie
         * restart, this needs to be false. \return The operation result. \remarks This function must be called from a
         * thread that isn't interlocked with the emulator thread.
         */
        std::function<core_result(bool reset_save_data, bool stop_vcr)> vr_reset_rom;

        /**
         * \brief Starts frame advancing the specified amount of frames.
         */
        std::function<void(size_t)> vr_frame_advance;

        /**
         * \brief Sets the fast-forward state.
         */
        std::function<void(bool)> vr_set_fast_forward;

        /**
         * \brief Gets the GS button state.
         */
        std::function<bool()> vr_get_gs_button;

        /**
         * \brief Sets the GS button state.
         */
        std::function<void(bool)> vr_set_gs_button;

        /**
         * \param country_code A rom's country code.
         * \return The maximum amount of VIs per second intended.
         */
        std::function<uint32_t(uint16_t country_code)> vr_get_vis_per_second;

        /**
         * \breif Gets the rom header.
         */
        std::function<core_rom_header *()> vr_get_rom_header;

        /**
         * \param country_code A rom's country code.
         * \return The rom's country name.
         */
        std::function<std::string(uint16_t country_code)> vr_country_code_to_country_name;

        /**
         * \brief Updates internal timings after the speed modifier changes.
         */
        std::function<void()> vr_on_speed_modifier_changed;

        /**
         * \brief Invalidates the visuals, allowing an updateScreen call to happen.
         */
        std::function<void()> vr_invalidate_visuals;

        /**
         * \brief Invalidates the dynarec code cache for the block containing the specified address.
         * \param addr The address. If UINT32_MAX, the entire cache is invalidated.
         */
        std::function<void(uint32_t addr)> vr_recompile;

        /**
         * \brief Returns the FPS and VI/s timings.
         * \remark This function is thread-safe.
         */
        std::function<void(float &, float &)> vr_get_timings;

#pragma endregion

#pragma region VCR

        /**
         * \brief Parses a movie's header
         * \param path The movie's path
         * \param header The header to fill
         * \return The operation result
         */
        std::function<core_result(std::filesystem::path path, core_vcr_movie_header *header)> vcr_parse_header;

        /**
         * \brief Reads the inputs from a movie
         * \param path The movie's path
         * \param inputs The button collection to fill
         * \return The operation result
         */
        std::function<core_result(std::filesystem::path path, std::vector<core_buttons> &inputs)> vcr_read_movie_inputs;

        /**
         * \brief Starts playing back a movie
         * \param path The movie's path
         * \return The operation result
         */
        std::function<core_result(std::filesystem::path path)> vcr_start_playback;

        /**
         * \brief Starts recording a movie
         * \param path The movie's path
         * \param flags Start flags, see MOVIE_START_FROM_X
         * \param author The movie author's name
         * \param description The movie's description
         * \return The operation result
         */
        std::function<core_result(std::filesystem::path path, uint16_t flags, std::string author,
                                  std::string description)>
            vcr_start_record;

        /**
         * \brief Continues recording a movie.
         * \return The operation result
         */
        std::function<core_result()> vcr_continue_recording;
        
        /**
         * \brief Replaces the author and description information of a movie
         * \param path The movie's path
         * \param author The movie author's name
         * \param description The movie's description
         * \return The operation result
         */
        std::function<core_result(const std::filesystem::path &path, std::string_view author,
                                  std::string_view description)>
            vcr_replace_author_info;

        /**
         * \brief Gets information about the current seek operation.
         */
        std::function<core_vcr_seek_info()> vcr_get_seek_info;

        /**
         * \brief Begins seeking to a frame in the current movie.
         * \param str A seek format string
         * \param pause_at_end Whether the emu should be paused when the seek operation ends
         * \return The operation result
         * \remarks When the seek operation completes, the SeekCompleted message will be sent
         *
         * Seek string format possibilities:
         *	"n" - Frame
         *	"+n", "-n" - Relative to current sample
         *	"^n" - Sample n from the end
         *
         */
        std::function<core_result(std::string str, bool pause_at_end)> vcr_begin_seek;

        /**
         * \brief Stops the current seek operation
         */
        std::function<void()> vcr_stop_seek;

        /**
         * \brief Gets whether the VCR engine is currently performing a seek operation
         */
        std::function<bool()> vcr_is_seeking;

        /**
         * \brief Writes a backup of the current movie to the backup folder.
         * \return The operation result
         */
        std::function<core_result()> vcr_write_backup;

        /**
         * \brief Stops all running tasks
         * \return The operation result
         */
        std::function<core_result()> vcr_stop_all;

        /**
         * \brief Gets the current movie path. Only valid when task is not idle.
         */
        std::function<std::filesystem::path()> vcr_get_path;

        /**
         * \brief Gets the current task
         */
        std::function<core_vcr_task()> vcr_get_task;

        /**
         * Gets the sample length of the current movie. If no movie is active, the function returns UINT32_MAX.
         */
        std::function<uint32_t()> vcr_get_length_samples;

        /**
         * Gets the VI length of the current movie. If no movie is active, the function returns UINT32_MAX.
         */
        std::function<uint32_t()> vcr_get_length_vis;

        /**
         * Gets the current VI in the current movie. If no movie is active, the function returns -1.
         */
        std::function<int32_t()> vcr_get_current_vi;

        /**
         * Gets a copy of the current input buffer.
         */
        std::function<std::vector<core_buttons>()> vcr_get_inputs;

        /**
         * Begins a warp modification operation. A "warp modification operation" is the changing of sample data which is
         * temporally behind the current sample.
         *
         * The VCR engine will find the last common sample between the current input buffer and the provided one.
         * Then, the closest savestate prior to that sample will be loaded and recording will be resumed with the
         * modified inputs up to the sample the function was called at.
         *
         * This operation is long-running and status is reported via the WarpModifyStatusChanged message.
         * A successful warp modify operation can be detected by the status changing from warping to none with no errors
         * inbetween.
         *
         * If the provided buffer is identical to the current input buffer (in both content and size), the operation
         * will succeed with no seek.
         *
         * If the provided buffer is larger than the current input buffer and the first differing input is after the
         * current sample, the operation will succeed with no seek. The input buffer will be overwritten with the
         * provided buffer and when the modified frames are reached in the future, they will be "applied" like in
         * playback mode.
         *
         * If the provided buffer is smaller than the current input buffer, the VCR engine will seek to the last frame
         * and otherwise perform the warp modification as normal.
         *
         * An empty input buffer will cause the operation to fail.
         *
         * \param inputs The input buffer to use.
         * \return The operation result
         */
        std::function<core_result(const std::vector<core_buttons> &inputs)> vcr_begin_warp_modify;

        /**
         * Gets the warp modify status
         */
        std::function<bool()> vcr_get_warp_modify_status;

        /**
         * Gets the first differing frame when performing a warp modify operation.
         * If no warp modify operation is active, the function returns SIZE_MAX.
         */
        std::function<size_t()> vcr_get_warp_modify_first_difference_frame;

        /**
         * Gets the current seek savestate frames.
         * Keys are frame numbers, values are unimportant and abscence of a seek savestate at a frame is marked by the
         * respective key not existing.
         */
        std::function<void(std::unordered_map<size_t, bool> &map)> vcr_get_seek_savestate_frames;

        /**
         * Gets whether a seek savestate exists at the specified frame.
         * The returned state changes when the <c>SeekSavestatesChanged</c> message is sent.
         */
        std::function<bool(size_t frame)> vcr_has_seek_savestate_at_frame;

#pragma endregion

#pragma region Tracelog

        /**
         * \brief Gets whether tracelogging is active.
         */
        std::function<bool()> tl_active;

        /**
         * \brief Starts trace logging to the specified file.
         * \param path The output path.
         * \param binary Whether log output is in a binary format.
         * \param append Whether log output will be appended to the file.
         */
        std::function<void(std::filesystem::path path, bool binary, bool append)> tl_start;

        /**
         * \brief Stops trace logging.
         */
        std::function<void()> tl_stop;

#pragma endregion

#pragma region Savestates

        /**
         * \brief Executes a savestate operation to a path.
         * \param path The savestate's path.
         * \param job The job to set.
         * \param callback The callback to call when the operation is complete.
         * \param ignore_warnings Whether warnings, such as those about ROM compatibility, shouldn't be shown.
         * \warning The operation won't complete immediately. Must be called via AsyncExecutor unless calls are
         * originating from the emu thread. \return Whether the operation was enqueued.
         */
        std::function<bool(const std::filesystem::path &path, core_st_job job, const core_st_callback &callback,
                           bool ignore_warnings)>
            st_do_file;

        /**
         * Executes a savestate operation in-memory.
         * \param buffer The buffer to use for the operation. Can be empty if the <see cref="job"/> is <see
         * cref="e_st_job::save"/>. \param job The job to set. \param callback The callback to call when the operation
         * is complete. \param ignore_warnings Whether warnings, such as those about ROM compatibility, shouldn't be
         * shown. \warning The operation won't complete immediately. Must be called via AsyncExecutor unless calls are
         * originating from the emu thread. \return Whether the operation was enqueued.
         */
        std::function<bool(const std::vector<uint8_t> &buffer, core_st_job job, const core_st_callback &callback,
                           bool ignore_warnings)>
            st_do_memory;

        /**
         * Gets the undo savestate buffer. Will be empty will no undo savestate is available.
         */
        std::function<void(std::vector<uint8_t> &buffer)> st_get_undo_savestate;

#pragma endregion

#pragma region Debugger

        /**
         * \brief Gets whether execution is resumed.
         */
        std::function<bool()> dbg_get_resumed;

        /**
         * \brief Sets execution resumed status.
         */
        std::function<void(bool)> dbg_set_is_resumed;

        /**
         * Steps execution by one instruction.
         */
        std::function<void()> dbg_step;

        /**
         * \brief Gets whether DMA reads are allowed. If false, reads should return 0xFF.
         */
        std::function<bool()> dbg_get_dma_read_enabled;

        /**
         * \brief Sets whether DMA reads are allowed.
         */
        std::function<void(bool)> dbg_set_dma_read_enabled;

        /**
         * \brief Gets whether the RSP is enabled.
         */
        std::function<bool()> dbg_get_rsp_enabled;

        /**
         * \brief Sets whether the RSP is enabled.
         */
        std::function<void(bool)> dbg_set_rsp_enabled;

        /**
         * \brief Disassembles an instruction at a given address.
         * TODO: Refactor
         */
        std::function<char *(char *buf, uint32_t w, uint32_t pc)> dbg_disassemble;

#pragma endregion

#pragma region Cheats

        /**
         * \brief Compiles a cheat code from code.
         * \param code The cheat code. Must be in the GameShark format.
         * \param cheat The compiled cheat. If the compilation fails, the cheat won't be mutated.
         * \return Whether the compilation was successful.
         */
        std::function<bool(std::string_view code, core_cheat &cheat)> cht_compile;

        /**
         * \brief Gets the cheat override stack.
         */
        std::function<void(std::stack<std::vector<core_cheat>> &)> cht_get_override_stack;

        /**
         * \brief Gets the cheat list.
         * \remarks The returned cheat list may not be the one set via core_cht_set_list, as the core can apply cheat
         * overrides.
         */
        std::function<void(std::vector<core_cheat> &)> cht_get_list;

        /**
         * \brief Sets the cheat list.
         * \remarks If a core cheat override is active, core_cht_set_list will do nothing.
         */
        std::function<void(const std::vector<core_cheat> &)> cht_set_list;

#pragma endregion
    };

#ifdef __cplusplus
}

#pragma region Helper Functions

constexpr uint32_t CORE_ADDR_MASK = 0x7FFFFF;

/**
 * \brief Converts an address for RDRAM operations with the specified size.
 * \param addr An address.
 * \param size The window size.
 * \return The converted address.
 */
constexpr uint32_t to_addr(const uint32_t addr, const uint8_t size)
{
    constexpr auto s8 = 3;
    constexpr auto s16 = 2;

    if (size == 4)
    {
        return addr;
    }
    if (size == 2)
    {
        return addr ^ s16;
    }
    if (size == 1)
    {
        return addr ^ s8;
    }
    return UINT32_MAX;
}

/**
 * \brief Gets the value at the specified address from RDRAM.
 * \tparam T The value's type.
 * \param addr The start address of the value.
 * \return The value at the address.
 */
template <typename T> constexpr T core_rdram_load(uint8_t *rdram, const uint32_t addr)
{
    return *(T *)(rdram + (to_addr(addr, sizeof(T)) & CORE_ADDR_MASK));
}

/**
 * \brief Sets the value at the specified address in RDRAM.
 * \tparam T The value's type.
 * \param addr The start address of the value.
 * \param value The value to set.
 */
template <typename T> void core_rdram_store(uint8_t *rdram, const uint32_t addr, T value)
{
    *(T *)(rdram + (to_addr(addr, sizeof(T)) & CORE_ADDR_MASK)) = value;
}

#pragma endregion

/**
 * \brief Creates a core instance with the specified parameters.
 * \remark Only one core instance is currently supported.
 */
core_result core_create(core_params *params, core_ctx **ctx);

#endif
