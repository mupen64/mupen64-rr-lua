/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Describes the new Mupen64 ("Zilmar Ext Spec") Plugin API.
 *
 * This header can be used standalone by Mupen64 plugins, just make sure to define PLUGIN_WITH_CALLBACKS first.
 *
 */

#pragma once

#include "core_plugin.h"
#include "core_types.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALL __cdecl
#else
#define EXPORT
#define CALL
#endif

extern "C"
{
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
     * \brief Exposes an extended set of functions to plugins.
     */
    struct core_plugin_extended_funcs
    {
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
    };

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

        // --- Zilmar spec struct ends here

        /*
         * \brief The mupen version this plugin targets. It must be an exact match, or the plugin will be rejected.
         */
        char target_version[16];
    } core_plugin_info;

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
        void(CALL *check_interrupts)(void);

        // --- Zilmar spec struct ends here

        /**
         * \brief Pointer to extended functions provided by Mupen.
         * \remarks **Unstable API** might change frequently.
         */
        core_plugin_extended_funcs *extended_funcs;
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
        void(CALL *check_interrupts)(void);

        // --- Zilmar spec struct ends here

        /**
         * \brief Pointer to extended functions provided by Mupen.
         * \remarks **Unstable API** might change frequently.
         */
        core_plugin_extended_funcs *extended_funcs;
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

        // --- Zilmar spec struct ends here

        /**
         * \brief Pointer to extended functions provided by Mupen.
         * \remarks **Unstable API** might change frequently.
         */
        core_plugin_extended_funcs *extended_funcs;
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
        void(CALL *check_interrupts)(void);
        void(CALL *process_dlist_list)(void);
        void(CALL *process_alist_list)(void);
        void(CALL *process_rdp_list)(void);
        void(CALL *show_cfb)(void);

        // --- Zilmar spec struct ends here

        /**
         * \brief Pointer to extended functions provided by Mupen.
         * \remarks **Unstable API** might change frequently.
         */
        core_plugin_extended_funcs *extended_funcs;
    } core_rsp_info;

    typedef void(CALL *CLOSEDLL)();
    typedef void(CALL *DLLABOUT)(void *);
    typedef void(CALL *DLLCONFIG)(void *);
    typedef void(CALL *DLLTEST)(void *);
    typedef void(CALL *GETDLLINFO)(core_plugin_info *);
    typedef void(CALL *ROMCLOSED)();
    typedef void(CALL *ROMOPEN)();

    typedef void(CALL *PROCESSDLIST)();
    typedef void(CALL *PROCESSRDPLIST)();
    typedef void(CALL *SHOWCFB)();
    typedef void(CALL *VISTATUSCHANGED)();
    typedef void(CALL *VIWIDTHCHANGED)();
    typedef void(CALL *GETVIDEOSIZE)(int32_t *, int32_t *);
    typedef void(CALL *FBREAD)(uint32_t);
    typedef void(CALL *FBWRITE)(uint32_t addr, uint32_t size);
    typedef void(CALL *FBGETFRAMEBUFFERINFO)(void *);
    typedef void(CALL *CHANGEWINDOW)();
    typedef int32_t(CALL *INITIATEGFX)(core_gfx_info);
    typedef void(CALL *UPDATESCREEN)();
    typedef void(CALL *READSCREEN)(void **, int32_t *, int32_t *);
    typedef void(CALL *DLLCRTFREE)(void *);
    typedef void(CALL *MOVESCREEN)(int32_t, int32_t);
    typedef void(CALL *CAPTURESCREEN)(char *);
    typedef void(CALL *READVIDEO)(void **);

    typedef int32_t(CALL *INITIATEAUDIO)(core_audio_info);
    typedef void(CALL *AIUPDATE)(int32_t wait);
    typedef void(CALL *AIDACRATECHANGED)(int32_t system_type);
    typedef void(CALL *AILENCHANGED)();
    typedef uint32_t(CALL *AIREADLENGTH)();
    typedef void(CALL *PROCESSALIST)();

    typedef void(CALL *OLD_INITIATECONTROLLERS)(void *hwnd, core_controller controls[4]);
    typedef void(CALL *INITIATECONTROLLERS)(core_input_info control_info);
    typedef void(CALL *KEYDOWN)(uint32_t wParam, int32_t lParam);
    typedef void(CALL *KEYUP)(uint32_t wParam, int32_t lParam);
    typedef void(CALL *CONTROLLERCOMMAND)(int32_t controller, unsigned char *command);
    typedef void(CALL *GETKEYS)(int32_t controller, core_buttons *keys);
    typedef void(CALL *SETKEYS)(int32_t controller, core_buttons keys);
    typedef void(CALL *READCONTROLLER)(int32_t controller, unsigned char *command);

    typedef void(CALL *INITIATERSP)(core_rsp_info rsp_info, uint32_t *cycles);
    typedef uint32_t(CALL *DORSPCYCLES)(uint32_t);

#if defined(PLUGIN_WITH_CALLBACKS)

    // ReSharper disable CppInconsistentNaming

#pragma region Base

    EXPORT void CALL CloseDLL(void);
    EXPORT void CALL DllAbout(void *hParent);
    EXPORT void CALL DllConfig(void *hParent);
    EXPORT void CALL GetDllInfo(core_plugin_info *PluginInfo);
    EXPORT void CALL RomClosed(void);
    EXPORT void CALL RomOpen(void);

#pragma endregion

#pragma region Video

    EXPORT void CALL CaptureScreen(const char *Directory);
    EXPORT void CALL ChangeWindow(void);
    EXPORT int CALL InitiateGFX(core_gfx_info Gfx_Info);
    EXPORT void CALL MoveScreen(int xpos, int ypos);
    EXPORT void CALL ProcessDList(void);
    EXPORT void CALL ProcessRDPList(void);
    EXPORT void CALL ShowCFB(void);
    EXPORT void CALL UpdateScreen(void);
    EXPORT void CALL ViStatusChanged(void);
    EXPORT void CALL ViWidthChanged(void);
    EXPORT void CALL mge_get_video_size(long *width, long *height);
    EXPORT void CALL mge_read_video2(void **);

#pragma endregion

#pragma region Audio

    EXPORT void CALL AiDacrateChanged(int32_t SystemType);
    EXPORT void CALL AiLenChanged(void);
    EXPORT uint32_t CALL AiReadLength(void);
    EXPORT void CALL AiUpdate(int32_t Wait);
    EXPORT void CALL DllTest(void *hParent);
    EXPORT int32_t CALL InitiateAudio(core_audio_info Audio_Info);
    EXPORT void CALL ProcessAList(void);

#pragma endregion

#pragma region Input

    EXPORT void CALL ControllerCommand(int32_t Control, uint8_t *Command);
    EXPORT void CALL GetKeys(int32_t Control, core_buttons *Keys);
    EXPORT void CALL SetKeys(int32_t controller, core_buttons keys);
#if defined(CORE_PLUGIN_INPUT_OLD_INITIATE_CONTROLLERS)
    EXPORT void CALL InitiateControllers(void *hwnd, core_controller controls[4]);
#else
    EXPORT void CALL InitiateControllers(core_input_info ControlInfo);
#endif
    EXPORT void CALL ReadController(int Control, uint8_t *Command);
    EXPORT void CALL WM_KeyDown(uint32_t wParam, uint32_t lParam);
    EXPORT void CALL WM_KeyUp(uint32_t wParam, uint32_t lParam);

#pragma endregion

#pragma region RSP

    EXPORT uint32_t DoRspCycles(uint32_t Cycles);
    EXPORT void InitiateRSP(core_rsp_info Rsp_Info, uint32_t *CycleCount);

#pragma endregion

    // ReSharper restore CppInconsistentNaming
#else
#undef EXPORT
#undef CALL
#endif
}

/**
 * \brief A module that provides helpers surrounding ZilmarExtSpecPlugin.
 */
namespace ZilmarExtSpecPluginHelpers
{
/**
 * @brief Gets the config path as a `std::filesystem::path` from a `core_plugin_extended_funcs`.
 */
inline std::filesystem::path get_config_path(const core_plugin_extended_funcs *ef)
{
    // get length
    size_t len = ef->config_path(nullptr, 0);
    // copy string
    std::string path_temp(len, '\0');
    ef->config_path(path_temp.data(), path_temp.size());
    // drop the terminating null
    path_temp.pop_back();
    // force UTF-8 conversion
    std::u8string_view utf8_path_temp{(char8_t *)path_temp.data(), path_temp.size()};
    return std::filesystem::absolute(utf8_path_temp);
}

} // namespace ZilmarExtSpecPluginHelpers
