/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Describes the M64RR plugin specification. This specification is volatile, with no compatibility guarantees across
 * Mupen64 versions or C++ ABIs.
 */

#pragma once

#ifndef __cplusplus
#error "The M64RR specification is only for C++"
#endif

#include "core_types.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALL __cdecl
#else
#error "Unsupported platform"
#endif

namespace M64RRSpec
{
extern "C"
{
    /**
     * \brief Represents the platform the plugin is running on.
     */
    enum class Platform : uint8_t
    {
        Windows,
        LinuxWayland,
    };

    struct WindowHandle
    {
        void *ptr1;
        void *ptr2;

        WindowHandle()
        {
            ptr1 = nullptr;
            ptr2 = nullptr;
        }

#ifdef _WIN32
        WindowHandle(HWND hwnd)
        {
            ptr1 = reinterpret_cast<void *>(hwnd);
            ptr2 = nullptr;
        }
        HWND hwnd() const { return reinterpret_cast<HWND>(ptr1); }
#endif
    };

    /**
     * \brief Represents a plugin type.
     */
    enum class PluginType : uint8_t
    {
        Video,
        Audio,
        Input,
        RSP,
    };

    /**
     * \brief Represents an extension for a controller.
     */
    enum class ControllerExtension : uint8_t
    {
        None,
        Mempak,
        Rumblepak,
        Transferpak,
        Raw,
    };

    /**
     * \brief Describes a controller.
     */
    struct Controller
    {
        bool present;
        bool raw;
        ControllerExtension plugin;
    };

    /**
     * \brief Represents a controller state.
     */
    union Buttons {
        uint32_t value;

        struct
        {
            unsigned dr : 1;
            unsigned dl : 1;
            unsigned dd : 1;
            unsigned du : 1;
            unsigned start : 1;
            unsigned z : 1;
            unsigned b : 1;
            unsigned a : 1;
            unsigned cr : 1;
            unsigned cl : 1;
            unsigned cd : 1;
            unsigned cu : 1;
            unsigned r : 1;
            unsigned l : 1;
            unsigned reserved_1 : 1;
            unsigned reserved_2 : 1;
            signed x : 8;
            signed y : 8;
        };
    };

    struct PluginMetadata
    {
        PluginType type;
        char name[128];
        char description[1024];
        char target_version[32];
    };

    struct PluginInit
    {
        Platform platform;
        WindowHandle main_window;
        int32_t byteswapped;
        uint8_t *rom;
        uint8_t *rdram;
        uint8_t *dmem;
        uint8_t *imem;
        uint32_t *mi_intr_reg;
        uint32_t *dpc_start_reg;
        uint32_t *dpc_end_reg;
        uint32_t *dpc_current_reg;
        uint32_t *dpc_status_reg;
        uint32_t *dpc_clock_reg;
        uint32_t *dpc_bufbusy_reg;
        uint32_t *dpc_pipebusy_reg;
        uint32_t *dpc_tmem_reg;
        uint32_t *vi_status_reg;
        uint32_t *vi_origin_reg;
        uint32_t *vi_width_reg;
        uint32_t *vi_intr_reg;
        uint32_t *vi_v_current_line_reg;
        uint32_t *vi_timing_reg;
        uint32_t *vi_v_sync_reg;
        uint32_t *vi_h_sync_reg;
        uint32_t *vi_leap_reg;
        uint32_t *vi_h_start_reg;
        uint32_t *vi_v_start_reg;
        uint32_t *vi_v_burst_reg;
        uint32_t *vi_x_scale_reg;
        uint32_t *vi_y_scale_reg;
        uint32_t *ai_dram_addr_reg;
        uint32_t *ai_len_reg;
        uint32_t *ai_control_reg;
        uint32_t *ai_status_reg;
        uint32_t *ai_dacrate_reg;
        uint32_t *ai_bitrate_reg;
        uint32_t *sp_mem_addr_reg;
        uint32_t *sp_dram_addr_reg;
        uint32_t *sp_rd_len_reg;
        uint32_t *sp_wr_len_reg;
        uint32_t *sp_status_reg;
        uint32_t *sp_dma_full_reg;
        uint32_t *sp_dma_busy_reg;
        uint32_t *sp_pc_reg;
        uint32_t *sp_semaphore_reg;

        void(CALL *process_dlist)(void);

        uint8_t *header;
        Controller *controllers;

        /**
         * \brief Logs the specified message at the trace level.
         */
        void (*log_trace)(const wchar_t *);

        /**
         * \brief Logs the specified message at the info level.
         */
        void (*log_info)(const wchar_t *);

        /**
         * \brief Logs the specified message at the warning level.
         */
        void (*log_warn)(const wchar_t *);

        /**
         * \brief Logs the specified message at the error level.
         */
        void (*log_error)(const wchar_t *);

        /**
         * \brief Gets the effective speed mode.
         * \return The current effective speed mode.
         */
        CoreSpeedMode (*get_effective_speed_mode)();

        /**
         * \brief See `core_ctx::vr_get_frame_skipped`.
         */
        bool (*frame_skipped)();

        /**
         * @brief Gets the path to the configuration directory, as a UTF-8 string.
         *
         * Writes the path to the configuration directory to `data`, provided that there is
         * enough space for path and terminating null character (up to `len`). Returns the
         * number of characters written (including the terminating null), or 0 if the buffer
         * wasn't big enough.
         *
         * If `data` is null, returns the expected size of the buffer.
         */
        size_t (*config_path)(char *data, size_t len);

        /**
         * \brief Counter for RCP work in an arbitrary unit, ideally proportional to real-time lag per unit.
         */
        size_t *rcp_counter;

        /**
         * \brief Requests the main window to asynchronously resize to the specified width and height.
         * \param width The desired width of the window.
         * \param height The desired height of the window.
         */
        void (*request_size)(uint32_t width, uint32_t height);

        PluginInit() = default;
        PluginInit(const PluginInit &) = delete;
        PluginInit &operator=(const PluginInit &) = delete;
    };

    /**
     * \brief Represents an event related to the plugin.
     */
    union Event {
        enum class Type : uint8_t
        {
            // The plugin is being initialized. The valid field is `initiate`.
            Initiate,

            // The plugin is being shut down. There will be no more calls to it after this event.
            Shutdown,

            // Emulation has started.
            RomOpened,

            // Emulation has stopped.
            RomClosed,
        };

        struct InitiateEvent
        {
            Type type;
            PluginInit *init;
        };

        // The type of the event.
        Type type;

        // The initiate event details. Only valid when `type` is `Initiate`.
        InitiateEvent initiate;
    };

    typedef void(CALL *PtrGetMetadata)(PluginMetadata *metadata);
    typedef void(CALL *PtrProcessEvent)(Event event);
    typedef void(CALL *PtrShowConfig)(WindowHandle parent_window);

    typedef void(CALL *PtrProcessDList)();
    typedef void(CALL *PtrReadVideo)(void *buffer, int32_t *width, int32_t *height);

    typedef void(CALL *PtrAIDacrateChanged)(CoreSystemType system_type);
    typedef void(CALL *PtrAILenChanged)();

    typedef void(CALL *PtrGetKeys)(uint8_t index, Buttons *buttons);
    typedef void(CALL *PtrSetKeys)(uint8_t index, const Buttons *buttons);
    typedef void(CALL *PtrReadController)(int32_t controller, unsigned char *command);

    typedef void(CALL *PtrDoRSPCycles)(uint8_t);
};

} // namespace M64RRSpec

#if defined(PLUGIN_WITH_CALLBACKS)

extern "C"
{
    using namespace M64RRSpec;

    // ReSharper disable CppInconsistentNaming

    /**
     * \brief Retrieves the plugin metadata. Always called before any other plugin function.
     * \param metadata The plugin metadata to be filled in.
     */
    EXPORT void CALL M64RRGetMetadata(PluginMetadata *metadata);

    /**
     * \brief Notifies the plugin of an event.
     * \param event The event.
     */
    EXPORT void CALL M64RRProcessEvent(Event event);

    /**
     * \brief Shows the configuration window.
     * \param parent_window The parent window handle.
     */
    EXPORT void CALL M64RRRShowConfig(WindowHandle parent_window);

    // ---

    /**
     * \brief Processes the display list.
     */
    EXPORT void CALL M64RRProcessDList(void);

    /**
     * \brief Reads video data.
     * \param buffer The buffer to store the video data. Can be `nullptr`.
     * \param width The width of the video. Can be `nullptr`.
     * \param height The height of the video. Can be `nullptr`.
     */
    EXPORT void CALL M64RRReadVideo(void *buffer, int32_t *width, int32_t *height);

    /**
     * \brief Called when the audio DAC rate changes.
     * \param system_type The system type.
     */
    EXPORT void CALL M64RRAIDacrateChanged(CoreSystemType system_type);

    /**
     * \brief Called when the audio length changes.
     */
    EXPORT void CALL M64RRAILenChanged(void);

    // ---

    /**
     * \brief Gets the keys for the specified controller.
     * \param controller The controller index.
     * \param keys The buttons to be filled in.
     */
    EXPORT void CALL M64RRGetKeys(uint8_t index, Buttons *buttons);

    /**
     * \brief Notifies the plugin that the keys for the specified controller have changed.
     * \param controller The controller index.
     * \param keys The buttons to be set.
     */
    EXPORT void CALL M64RRSetKeys(uint8_t index, const Buttons *buttons);

    /**
     * \brief Notifies the plugin of a controller command.
     * \param controller The controller index.
     * \param command The controller command.
     */
    EXPORT void CALL M64RRReadController(int32_t controller, unsigned char *command);

    // ---

    /**
     * \brief Does RSP cycles.
     * \param cycles The number of RSP cycles to do.
     */
    EXPORT void CALL M64RRDoRSPCycles(uint8_t cycles);

    // ReSharper restore CppInconsistentNaming
}

#endif

// #undef EXPORT
// #undef CALL
