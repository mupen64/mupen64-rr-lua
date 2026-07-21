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

#include "core_types.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALL __cdecl
#else
#error "Unsupported platform"
#endif

namespace ZilmarExtSpec
{

extern "C"
{
    /**
     * \brief Represents a plugin type.
     */
    enum class PluginType : uint16_t
    {
        Video = 2,
        Audio = 3,
        Input = 4,
        RSP = 1,
    };

    /**
     * \brief Represents an extension for a controller.
     */
    enum class ControllerExtension : int32_t
    {
        None = 1,
        Mempak = 2,
        Rumblepak = 3,
        Transferpak = 4,
        Raw = 5
    };

    /**
     * \brief Describes a controller.
     */
    struct Controller
    {
        int32_t present;
        int32_t raw;
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

    /**
     * \brief Exposes an extended set of functions to plugins.
     */
    struct ExtendedFuncs
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
    struct PluginInfo
    {
        /**
         * \brief <c>0x0100</c> (old)
         * <c>0x0101</c> (new).
         * If <c>0x0101</c> is specified and the plugin is an input plugin, <c>InitiateControllers</c> will be called
         * with the <c>INITIATECONTROLLERS</c> signature instead of <c>OLD_INITIATECONTROLLERS</c>.
         */
        uint16_t ver;

        PluginType type;

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
        char target_version[32];
    };

    struct VideoPluginInfo
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
        ExtendedFuncs *extended_funcs;
    };

    struct AudioPluginInfo
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
        ExtendedFuncs *extended_funcs;
    };

    struct InputPluginInfo
    {
        void *main_hwnd;
        void *hinst;
        int32_t byteswapped;
        uint8_t *header;
        Controller *controllers;

        // --- Zilmar spec struct ends here

        /**
         * \brief Pointer to extended functions provided by Mupen.
         * \remarks **Unstable API** might change frequently.
         */
        ExtendedFuncs *extended_funcs;
    };

    struct RSPPluginInfo
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
        ExtendedFuncs *extended_funcs;
    };

    typedef void(CALL *CLOSEDLL)();
    typedef void(CALL *DLLABOUT)(void *);
    typedef void(CALL *DLLCONFIG)(void *);
    typedef void(CALL *DLLTEST)(void *);
    typedef void(CALL *GETDLLINFO)(PluginInfo *);
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
    typedef void(CALL *FBGETFRAMEBUFFERINFO)(ZilmarExtSpec::FBInfo *);
    typedef void(CALL *CHANGEWINDOW)();
    typedef int32_t(CALL *INITIATEGFX)(VideoPluginInfo);
    typedef void(CALL *UPDATESCREEN)();
    typedef void(CALL *READSCREEN)(void **, int32_t *, int32_t *);
    typedef void(CALL *DLLCRTFREE)(void *);
    typedef void(CALL *MOVESCREEN)(int32_t, int32_t);
    typedef void(CALL *CAPTURESCREEN)(char *);
    typedef void(CALL *READVIDEO)(void **);

    typedef int32_t(CALL *INITIATEAUDIO)(AudioPluginInfo);
    typedef void(CALL *AIUPDATE)(int32_t wait);
    typedef void(CALL *AIDACRATECHANGED)(int32_t system_type);
    typedef void(CALL *AILENCHANGED)();
    typedef uint32_t(CALL *AIREADLENGTH)();
    typedef void(CALL *PROCESSALIST)();

    typedef void(CALL *OLD_INITIATECONTROLLERS)(void *hwnd, Controller controls[4]);
    typedef void(CALL *INITIATECONTROLLERS)(InputPluginInfo control_info);
    typedef void(CALL *KEYDOWN)(uint32_t wParam, int32_t lParam);
    typedef void(CALL *KEYUP)(uint32_t wParam, int32_t lParam);
    typedef void(CALL *CONTROLLERCOMMAND)(int32_t controller, unsigned char *command);
    typedef void(CALL *GETKEYS)(int32_t controller, Buttons *keys);
    typedef void(CALL *SETKEYS)(int32_t controller, Buttons keys);
    typedef void(CALL *READCONTROLLER)(int32_t controller, unsigned char *command);

    typedef void(CALL *INITIATERSP)(RSPPluginInfo rsp_info, uint32_t *cycles);
    typedef uint32_t(CALL *DORSPCYCLES)(uint32_t);
}

/**
 * \brief Gets the config path from a `core_plugin_extended_funcs`.
 */
inline std::filesystem::path get_config_path(const ExtendedFuncs *ef)
{
    size_t len = ef->config_path(nullptr, 0);

    std::string path_temp(len, '\0');
    ef->config_path(path_temp.data(), path_temp.size());
    path_temp.pop_back();

    std::u8string_view utf8_path_temp{(char8_t *)path_temp.data(), path_temp.size()};
    return std::filesystem::absolute(utf8_path_temp);
}

}; // namespace ZilmarExtSpec

#if defined(PLUGIN_WITH_CALLBACKS)

extern "C"
{
    // ReSharper disable CppInconsistentNaming

#pragma region Base

    EXPORT void CALL CloseDLL(void);
    EXPORT void CALL DllAbout(void *hParent);
    EXPORT void CALL DllConfig(void *hParent);
    EXPORT void CALL GetDllInfo(ZilmarExtSpec::PluginInfo *PluginInfo);
    EXPORT void CALL RomClosed(void);
    EXPORT void CALL RomOpen(void);

#pragma endregion

#pragma region Video

    EXPORT void CALL CaptureScreen(const char *Directory);
    EXPORT void CALL ChangeWindow(void);
    EXPORT int CALL InitiateGFX(ZilmarExtSpec::VideoPluginInfo Gfx_Info);
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
    EXPORT int32_t CALL InitiateAudio(ZilmarExtSpec::AudioPluginInfo Audio_Info);
    EXPORT void CALL ProcessAList(void);

#pragma endregion

#pragma region Input

    EXPORT void CALL ControllerCommand(int32_t Control, uint8_t *Command);
    EXPORT void CALL GetKeys(int32_t Control, ZilmarExtSpec::Buttons *Keys);
    EXPORT void CALL SetKeys(int32_t controller, ZilmarExtSpec::Buttons keys);
    EXPORT void CALL InitiateControllers(ZilmarExtSpec::InputPluginInfo ControlInfo);
    EXPORT void CALL ReadController(int Control, uint8_t *Command);
    EXPORT void CALL WM_KeyDown(uint32_t wParam, uint32_t lParam);
    EXPORT void CALL WM_KeyUp(uint32_t wParam, uint32_t lParam);

#pragma endregion

#pragma region RSP

    EXPORT uint32_t DoRspCycles(uint32_t Cycles);
    EXPORT void InitiateRSP(ZilmarExtSpec::RSPPluginInfo Rsp_Info, uint32_t *CycleCount);

#pragma endregion

    // ReSharper restore CppInconsistentNaming
}

#endif

// #undef EXPORT
// #undef CALL
