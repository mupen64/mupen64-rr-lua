/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Describes the Mupen64 core-side Plugin API.
 *
 * This header can be used standalone by Mupen64 plugins.
 *
 */

#pragma once

extern "C"
{
    /**
     * \brief Describes a controller.
     */
    typedef struct
    {
        int32_t Present;
        int32_t RawData;
        int32_t Plugin;
    } core_controller;

    /**
     * \brief Represents an extension for a controller.
     */
    typedef enum
    {
        ce_none = 1,
        ce_mempak = 2,
        ce_rumblepak = 3,
        ce_transferpak = 4,
        ce_raw = 5
    } core_controller_extension;

    /**
     * \brief Represents a plugin type.
     */
    typedef enum
    {
        plugin_video = 2,
        plugin_audio = 3,
        plugin_input = 4,
        plugin_rsp = 1,
    } core_plugin_type;

    /**
     * \brief Describes generic information about a plugin.
     */
    typedef struct
    {
        /**
         * \brief <c>0x0100</c> (old)
         * <c>0x0101</c> (new).
         * If <c>0x0101</c> is specified and the plugin is an input plugin, <c>InitiateControllers</c> will be called
         * with the <c>INITIATECONTROLLERS</c> signature instead of <c>OLD_INITIATECONTROLLERS</c>.
         */
        uint16_t ver;

        /**
         * \brief The plugin type, see <c>core_plugin_type</c>.
         */
        uint16_t type;

        /**
         * \brief The plugin name.
         */
        char name[100];

        int32_t unused_normal_memory;
        int32_t unused_byteswapped;
    } core_plugin_info;

    /**
     * \brief Describes framebuffer information.
     */
    typedef struct
    {
        uint32_t addr;
        uint32_t size;
        uint32_t width;
        uint32_t height;
    } core_fb_info;

    /**
     * \brief Describes information about a video plugin.
     */
    typedef struct
    {
        void *main_hwnd;
        void *statusbar_hwnd;
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
        void(__cdecl *check_interrupts)(void);
    } core_gfx_info;

    /**
     * \brief Describes information about an audio plugin.
     */
    typedef struct
    {
        void *main_hwnd;
        void *hinst;
        int32_t byteswapped;
        uint8_t *rom;
        uint8_t *rdram;
        uint8_t *dmem;
        uint8_t *imem;
        uint32_t *mi_intr_reg;
        uint32_t *ai_dram_addr_reg;
        uint32_t *ai_len_reg;
        uint32_t *ai_control_reg;
        uint32_t *ai_status_reg;
        uint32_t *ai_dacrate_reg;
        uint32_t *ai_bitrate_reg;
        void(__cdecl *check_interrupts)(void);
    } core_audio_info;

    /**
     * \brief Describes information about an input plugin.
     */
    typedef struct
    {
        void *main_hwnd;
        void *hinst;
        int32_t byteswapped;
        uint8_t *header;
        core_controller *controllers;
    } core_input_info;

    /**
     * \brief Describes information about an RSP plugin.
     */
    typedef struct
    {
        void *hinst;
        int32_t byteswapped;
        uint8_t *rdram;
        uint8_t *dmem;
        uint8_t *imem;
        uint32_t *mi_intr_reg;
        uint32_t *sp_mem_addr_reg;
        uint32_t *sp_dram_addr_reg;
        uint32_t *sp_rd_len_reg;
        uint32_t *sp_wr_len_reg;
        uint32_t *sp_status_reg;
        uint32_t *sp_dma_full_reg;
        uint32_t *sp_dma_busy_reg;
        uint32_t *sp_pc_reg;
        uint32_t *sp_semaphore_reg;
        uint32_t *dpc_start_reg;
        uint32_t *dpc_end_reg;
        uint32_t *dpc_current_reg;
        uint32_t *dpc_status_reg;
        uint32_t *dpc_clock_reg;
        uint32_t *dpc_bufbusy_reg;
        uint32_t *dpc_pipebusy_reg;
        uint32_t *dpc_tmem_reg;
        void(__cdecl *check_interrupts)(void);
        void(__cdecl *process_dlist_list)(void);
        void(__cdecl *process_alist_list)(void);
        void(__cdecl *process_rdp_list)(void);
        void(__cdecl *show_cfb)(void);
    } core_rsp_info;

    /**
     * \brief Represents a controller state.
     */
    typedef union {
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
            signed y : 8;
            signed x : 8;
        };
    } core_buttons;

    typedef void(__cdecl *ROMCLOSED)();
    typedef void(__cdecl *ROMOPEN)();

    typedef void(__cdecl *PROCESSDLIST)();
    typedef void(__cdecl *PROCESSRDPLIST)();
    typedef void(__cdecl *SHOWCFB)();
    typedef void(__cdecl *VISTATUSCHANGED)();
    typedef void(__cdecl *VIWIDTHCHANGED)();
    typedef void(__cdecl *GETVIDEOSIZE)(int32_t *, int32_t *);
    typedef void(__cdecl *FBREAD)(uint32_t);
    typedef void(__cdecl *FBWRITE)(uint32_t addr, uint32_t size);
    typedef void(__cdecl *FBGETFRAMEBUFFERINFO)(void *);

    typedef void(__cdecl *AIDACRATECHANGED)(int32_t system_type);
    typedef void(__cdecl *AILENCHANGED)();
    typedef uint32_t(__cdecl *AIREADLENGTH)();
    typedef void(__cdecl *PROCESSALIST)();
    typedef void(__cdecl *AIUPDATE)(int32_t wait);

    typedef void(__cdecl *CONTROLLERCOMMAND)(int32_t controller, unsigned char *command);
    typedef void(__cdecl *GETKEYS)(int32_t controller, core_buttons *keys);
    typedef void(__cdecl *SETKEYS)(int32_t controller, core_buttons keys);
    typedef void(__cdecl *READCONTROLLER)(int32_t controller, unsigned char *command);

    typedef uint32_t(__cdecl *DORSPCYCLES)(uint32_t);
}

inline bool operator==(const core_buttons &lhs, const core_buttons &rhs)
{
    return lhs.value == rhs.value;
}
