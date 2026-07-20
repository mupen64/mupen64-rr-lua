/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Describes the new Mupen64-RR Plugin API.
 *
 * This header can be used standalone by Mupen64 plugins, just make sure to define PLUGIN_WITH_CALLBACKS first.
 *
 */

#pragma once

#include "core_types.h"
#include "ZilmarExtSpecPlugin.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALL __cdecl
#else
#error "Unsupported platform"
#endif

namespace MupenRRSpecPlugin
{
extern "C"
{
    using ExtendedFuncs = ZilmarExtSpec::ExtendedFuncs;

    /**
     * \brief Represents the platform the plugin is running on.
     */
    enum class Platform
    {
        Windows,
        LinuxWayland,
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

        CoreController to_core_controller() const
        {
            CoreControllerExtension extension;
            switch (plugin)
            {
            case ControllerExtension::None:
                extension = CoreControllerExtension::None;
                break;
            case ControllerExtension::Mempak:
                extension = CoreControllerExtension::Mempak;
                break;
            case ControllerExtension::Rumblepak:
                extension = CoreControllerExtension::Rumblepak;
                break;
            case ControllerExtension::Transferpak:
                extension = CoreControllerExtension::Transferpak;
                break;
            case ControllerExtension::Raw:
                extension = CoreControllerExtension::Raw;
                break;
            default:
                assert(false && "Unknown controller extension");
                break;
            }

            return CoreController{
                .Present = present ? 1 : 0,
                .RawData = raw ? 1 : 0,
                .Plugin = extension,
            };
        }
    };

    /**
     * \brief Describes framebuffer information.
     */
    struct FBInfo
    {
        uint32_t addr;
        uint32_t size;
        uint32_t width;
        uint32_t height;
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

        void(CALL *process_d_list)(void);
        void(CALL *process_a_list)(void);
        void(CALL *process_rdp_list)(void);
        void(CALL *check_interrupts)(void);

        uint8_t *header;
        Controller *controllers;
        ZilmarExtSpec::ExtendedFuncs *ef;
    };

    struct WindowHandle
    {
        void *ptr1;
        void *ptr2;

#ifdef _WIN32
        WindowHandle(HWND hwnd)
        {
            ptr1 = reinterpret_cast<void *>(hwnd);
            ptr2 = nullptr;
        }
#endif
    };

    typedef void(CALL *PtrGetMetadata)(PluginMetadata *metadata);
    typedef void(CALL *PtrInitiate)(PluginInit *init);
    typedef void(CALL *PtrRomOpened)();
    typedef void(CALL *PtrRomClosed)();
    typedef void(CALL *PtrShowConfig)(WindowHandle parent_window);

    typedef void(CALL *PtrProcessDList)();
    typedef void(CALL *PtrGetVideoSize)(int32_t *, int32_t *);
    typedef void(CALL *PtrReadVideo)(void **);

    typedef void(CALL *PtrAIDacrateChanged)(int32_t system_type);
    typedef void(CALL *PtrAILenChanged)();

    typedef void(CALL *PtrGetKeys)(int32_t controller, Buttons *keys);
    typedef void(CALL *PtrSetKeys)(int32_t controller, Buttons keys);
    typedef void(CALL *PtrReadController)(int32_t controller, unsigned char *command);

    typedef void(CALL *PtrDoRSPCycles)(uint8_t);
};
} // namespace MupenRRSpecPlugin

#if defined(PLUGIN_WITH_CALLBACKS)

extern "C"
{
    using namespace MupenRRSpecPlugin;

    // ReSharper disable CppInconsistentNaming

    EXPORT void CALL M64RRGetMetadata(PluginMetadata *metadata);
    EXPORT void CALL M64RRInitiate(PluginInit *init);
    EXPORT void CALL M64RRRomOpened(void);
    EXPORT void CALL M64RRRomClosed(void);
    EXPORT void CALL M64RRRShowConfig(WindowHandle parent_window);

    EXPORT void CALL M64RRProcessDList(void);
    EXPORT void CALL M64RRGetVideoSize(int32_t *width, int32_t *height);
    EXPORT void CALL M64RRReadVideo(void **video);

    EXPORT void CALL M64RRAIDacrateChanged(int32_t system_type);
    EXPORT void CALL M64RRAILenChanged(void);

    EXPORT void CALL M64RRGetKeys(int32_t controller, Buttons *keys);
    EXPORT void CALL M64RRSetKeys(int32_t controller, Buttons keys);
    EXPORT void CALL M64RRReadController(int32_t controller, unsigned char *command);

    EXPORT void CALL M64RRDoRSPCycles(uint8_t cycles);

    // ReSharper restore CppInconsistentNaming
}

#endif

// #undef EXPORT
// #undef CALL
