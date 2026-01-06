/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Magnificent Unified Plugin API
// ==============================
// The one to rule them all; designed to work on any platform.

#ifndef MUPEN64RR_MUPAPI_H_INCUDED
#define MUPEN64RR_MUPAPI_H_INCUDED

#include "core_types.h"
#include <core_plugin.h>
#include <cstdint>
#include <type_traits>

#if defined(_WIN32)
#include <windows.h>

#define EXPORT __declspec(dllexport)
#define CALL __cdecl
#elif defined(__linux__)
#include <xcb/xcb.h>
#include <wayland-client-protocol.h>

#define EXPORT __attribute__ ((visibility ("default")))
#define CALL
#else
#error Unsupported platform!
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MUPAPI_PLUGIN_DECLS
#define MUPAPI_DEFINE_FN(rt_type, name, ...)                                                                           \
    extern "C" rt_type name(__VA_ARGS__);                                                                                         \
    typedef rt_type (CALL *fp_##name)(__VA_ARGS__);
#else
#define MUPAPI_DEFINE_FN(rt_type, name, ...) typedef rt_type (*fp_##name)(__VA_ARGS__);
#endif

    // types
    // =============================

    typedef struct _XDisplay Display;

    /**
     * @brief A list of forwarded functions that can be passed to child plugins.
     *
     */
    typedef struct
    {
        /**
         * @brief Size of the structure in bytes.
         */
        uint32_t size;

        /**
         * @brief Logs the specified message at the trace level.
         */
        void (CALL *log_trace)(const char *);

        /**
         * @brief Logs the specified message at the info level.
         */
        void (CALL *log_info)(const char *);

        /**
         * @brief Logs the specified message at the warning level.
         */
        void (CALL *log_warn)(const char *);

        /**
         * @brief Logs the specified message at the error level.
         */
        void (CALL *log_error)(const char *);
    } core_plugin_extended_funcs;

    /**
     * \brief Describes information about a video plugin.
     */
    typedef struct
    {
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
    } core_gfx_info;

    /**
     * \brief Describes information about an audio plugin.
     */
    typedef struct
    {
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
    } core_audio_info;

    /**
     * \brief Describes information about an input plugin.
     */
    typedef struct
    {
        int32_t byteswapped;
        uint8_t *header;
        core_controller *controllers;
    } core_input_info;

    /**
     * \brief Describes information about an RSP plugin.
     */
    typedef struct
    {
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
        void (CALL *check_interrupts)(void);
        void (CALL *process_dlist_list)(void);
        void (CALL *process_alist_list)(void);
        void (CALL *process_rdp_list)(void);
        void (CALL *show_cfb)(void);
    } core_rsp_info;

    // window handles and whatnot
    // =============================

    /**
     * @brief Marks a window system, that is, an interface by which graphical apps can create and display windows.
     *
     */
    enum mup_wm_platform
    {
        /**
         * @brief The Windows API.
         */
        MUP_WM_WIN32 = 0,
        /**
         * @brief X11.
         */
        MUP_WM_X11,
        /**
         * @brief Wayland.
         * @note This API uses libwayland's types.
         */
        MUP_WM_WAYLAND,
    };

    /**
     * @brief A window handle, usually to initialize and display graphics.
     */
    typedef struct
    {
        mup_wm_platform platform;
        union {
#if defined(_WIN32)
            HWND win32_hwnd;
#elif defined(__linux__)
        xcb_window_t x11_xid;
        wl_surface *wl_surface;
#else
#error Unsupported platform!
#endif
        } handle;
    } mup_wm_handle;

    /**
     * @brief A display handle, usually needed to initialize graphics contexts.
     */
    typedef struct
    {
        mup_wm_platform platform;
        union {
#if defined(_WIN32)
            char dummy;
#elif defined(__linux__)
        struct
        {
            Display *x11_xlib_display;
            xcb_connection_t *x11_xcb_connection;
        };
        wl_display *wl_display;
#else
#error Unsupported platform!
#endif
        } handle;
    } mup_display_handle;

    /**
     * @brief An exported window handle, usually to pass to a child process.
     * @note On Wayland, this struct retains ownership of the string containing the ID handle. The handle should be freed using `free()`.
     */
    typedef struct
    {
        mup_wm_platform platform;
        union {
#ifdef _WIN32
            HWND win32_hwnd;
#endif
#ifdef __linux__
            xcb_window_t x11_xid;
            const char *wl_foreign_id;
#endif
        } handle;
    } mup_wm_export_handle;

    // managed graphics API
    // =============================

    typedef void (*mupv_fptr)(void);

    enum mupv_gl_buffer_attr {
        MUPV_GL_RED_BITS = 0,
        MUPV_GL_GREEN_BITS,
        MUPV_GL_BLUE_BITS,
        MUPV_GL_ALPHA_BITS,
        MUPV_GL_SAMPLES,
        MUPV_GL_BUFFER_ATTRS_COUNT,
    };

    enum mupv_gl_profile {
        MUPV_GL_COMPATIBILITY = 0,
        MUPV_GL_CORE,
        MUPV_GL_ES
    };

    typedef struct {
        mupv_fptr (*get_proc_address)(void* p_self, const char* sym);

        core_result (*request_attrs)(void* p_self, const mupv_gl_buffer_attr* attrs, const int32_t* vals, size_t len);
        core_result (*request_version)(void* p_self, mupv_gl_profile profile, uint32_t major, uint32_t minor);

        core_result (*query_attrs)(void* p_self, const mupv_gl_buffer_attr* attrs, int32_t* vals, size_t len);
        core_result (*query_version)(void* p_self, mupv_gl_profile* profile, uint32_t* major, uint32_t* minor);
        core_result (*query_default_fbo)(void* p_self, uint32_t* fbo);

        core_result (*swap_buffers)(void* p_self);
    } mupv_wm_gl_funcs;

    enum mupv_graphics_api {
        MUPV_API_OPENGL = 0
    };

    typedef struct {
        size_t size;
        void* p_self;

        core_result (*init)(void** pp_self, mupv_graphics_api api, void* funcs);
        core_result (*drop)(void** pp_self);

        core_result (*open_window)(void* p_self, uint32_t width, uint32_t height);
        core_result (*close_window)(void* p_self);
    } mupv_wm_funcs;



    // common definitions
    // =============================

    /**
     * @brief Initializes the plugin. Should be called immediately after loading.
     *
     * @param plugin_dir The directory where plugins are stored.
     * @param fwd_funcs A table with functions passed from the core.
     */
    MUPAPI_DEFINE_FN(core_result, mup_init, const char *plugin_dir, const core_plugin_extended_funcs *fwd_funcs);

    /**
     * @brief Cleans up all plugin resources. Should be called immediately before unloading.
     */
    MUPAPI_DEFINE_FN(void, mup_drop);

    /**
     * @brief Displays the configuration GUI for this plugin.
     *
     * Plugins should create a subprocess and parent their window to `parent_window`, though platforms which 
     * allow multiple event loops per program (e.g. Windows) may do this in-process.
     *
     * @param parent_window The *exported* plugin handle.
     */
    MUPAPI_DEFINE_FN(void, mup_show_config, mup_wm_export_handle parent_window);

    /**
     * @brief Requests information from this plugin. This may be called before `mup_init`.
     *
     * @param plugin_info A non-null pointer to a core_plugin_info struct to be filled with the needed information.
     */
    MUPAPI_DEFINE_FN(void, mup_get_info, core_plugin_info *plugin_info);

    /**
     * @brief Called just before the emulator starts execution.
     */
    MUPAPI_DEFINE_FN(void, mup_rom_opened);

    /**
     * @brief Called just before the emulator halts execution.
     */
    MUPAPI_DEFINE_FN(void, mup_rom_closed);

    // video
    // =============================

    /**
     * @brief Called to initialize the graphics plugin and request window settings.
     *
     * @param core_info Various pointers to objects inside the core.
     * @param wm_funcs Functions that can be used to initialize a window and graphics context.
     */
    MUPAPI_DEFINE_FN(void, mupv_init, core_gfx_info core_info, mupv_wm_funcs wm_funcs);

    MUPAPI_DEFINE_FN(void, mupv_process_d_list);
    MUPAPI_DEFINE_FN(void, mupv_process_rdp_list);
    MUPAPI_DEFINE_FN(void, mupv_show_cfb);
    MUPAPI_DEFINE_FN(void, mupv_vi_status_changed);
    MUPAPI_DEFINE_FN(void, mupv_vi_width_changed);
    MUPAPI_DEFINE_FN(void, mupv_get_video_size, int32_t *p_width, int32_t *p_height);
    MUPAPI_DEFINE_FN(void, mupv_fb_read, uint32_t addr);
    MUPAPI_DEFINE_FN(void, mupv_fb_write, uint32_t addr, uint32_t size);
    MUPAPI_DEFINE_FN(void, mupv_get_fb_info, void *fb_info);

#ifdef __cplusplus
    // sanity check vs core headers
    static_assert(std::is_same_v<fp_mupv_process_d_list, PROCESSDLIST>);
    static_assert(std::is_same_v<fp_mupv_process_rdp_list, PROCESSRDPLIST>);
    static_assert(std::is_same_v<fp_mupv_show_cfb, SHOWCFB>);
    static_assert(std::is_same_v<fp_mupv_vi_status_changed, VISTATUSCHANGED>);
    static_assert(std::is_same_v<fp_mupv_vi_width_changed, VIWIDTHCHANGED>);
    static_assert(std::is_same_v<fp_mupv_get_video_size, GETVIDEOSIZE>);
    static_assert(std::is_same_v<fp_mupv_fb_read, FBREAD>);
    static_assert(std::is_same_v<fp_mupv_fb_write, FBWRITE>);
    static_assert(std::is_same_v<fp_mupv_get_fb_info, FBGETFRAMEBUFFERINFO>);
#endif

    // audio
    // =============================

    /**
     * @brief Called to initialize this audio plugin.
     * Do not expect any window system information from `core_info`, these are left for compatibility with older
     * plugins.
     *
     * @param core_info Various pointers to objects inside the core.
     */
    MUPAPI_DEFINE_FN(void, mupa_init, core_audio_info core_info);

    MUPAPI_DEFINE_FN(void, mupa_ai_dacrate_changed, int32_t system_type);
    MUPAPI_DEFINE_FN(void, mupa_ai_len_changed);
    MUPAPI_DEFINE_FN(uint32_t, mupa_ai_read_length);
    MUPAPI_DEFINE_FN(void, mupa_process_a_list);
    MUPAPI_DEFINE_FN(void, mupa_ai_update, int32_t wait);

#ifdef __cplusplus
    // sanity check vs core headers
    static_assert(std::is_same_v<fp_mupa_ai_dacrate_changed, AIDACRATECHANGED>);
    static_assert(std::is_same_v<fp_mupa_ai_len_changed, AILENCHANGED>);
    static_assert(std::is_same_v<fp_mupa_ai_read_length, AIREADLENGTH>);
    static_assert(std::is_same_v<fp_mupa_process_a_list, PROCESSALIST>);
    static_assert(std::is_same_v<fp_mupa_ai_update, AIUPDATE>);
#endif

    // input
    // =============================

    /**
     * @brief Called to initialize this input plugin.
     * Do not expect any window system information from `core_info`, these are left for compatibility with older
     * plugins.
     *
     * @param core_info Various pointers to objects inside the core.
     */
    MUPAPI_DEFINE_FN(void, mupi_init, core_input_info core_info);

    MUPAPI_DEFINE_FN(void, mupi_controller_command, int32_t controller, unsigned char *command);
    MUPAPI_DEFINE_FN(void, mupi_get_keys, int32_t controller, core_buttons *keys);
    MUPAPI_DEFINE_FN(void, mupi_set_keys, int32_t controller, core_buttons keys);
    MUPAPI_DEFINE_FN(void, mupi_read_controller, int32_t controller, unsigned char *command);

#ifdef __cplusplus
    // sanity check vs core headers
    static_assert(std::is_same_v<fp_mupi_controller_command, CONTROLLERCOMMAND>);
    static_assert(std::is_same_v<fp_mupi_get_keys, GETKEYS>);
    static_assert(std::is_same_v<fp_mupi_set_keys, SETKEYS>);
    static_assert(std::is_same_v<fp_mupi_read_controller, READCONTROLLER>);
#endif

    // rsp
    // =============================

    /**
     * @brief Called to initialize this RSP plugin.
     * Do not expect any window system information from `core_info`, these are left for compatibility with older
     * plugins.
     *
     * @param core_info Various pointers to objects inside the core.
     */
    MUPAPI_DEFINE_FN(void, mupr_init, core_rsp_info core_info);

    MUPAPI_DEFINE_FN(uint32_t, mupr_do_rsp_cycles, uint32_t count);

#ifdef __cplusplus
    // sanity check vs core headers
    static_assert(std::is_same_v<fp_mupr_do_rsp_cycles, DORSPCYCLES>);
#endif
#undef MUPAPI_DEFINE_FN

#ifdef __cplusplus
}
#endif
#endif