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

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <X11/X.h>
#include <wayland-client-protocol.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define MUPAPI_PLUGIN_DECLS
#ifdef MUPAPI_PLUGIN_DECLS
#define MUPAPI_DEFINE_FN(rt_type, name, ...)                                                                           \
    rt_type name(__VA_ARGS__);                                                                                         \
    typedef rt_type (*fp_##name)(__VA_ARGS__);
#else
#define MUPAPI_DEFINE_FN(rt_type, name, ...) typedef rt_type (*fp_##name)(__VA_ARGS__);
#endif

    // types
    // =============================

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
         * @brief Opaque pointer for the core, to be passed into forwarded functions.
         */
        void *core_ptr;

        /**
         * @brief Logs the specified message at the trace level.
         */
        void (*log_trace)(const char *);

        /**
         * @brief Logs the specified message at the info level.
         */
        void (*log_info)(const char *);

        /**
         * @brief Logs the specified message at the warning level.
         */
        void (*log_warn)(const char *);

        /**
         * @brief Logs the specified message at the error level.
         */
        void (*log_error)(const char *);
    } core_forward_funcs;

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
        /**
         * @brief Cocoa, the native API on macOS.
         */
        MUP_WM_COCOA,
    };

    /**
     * @brief A window handle, usually to initialize and display graphics.
     */
    typedef struct
    {
        mup_wm_platform platform;
        union {
#ifdef _WIN32
            HWND win32_hwnd;
#endif
#ifdef __linux__
            XID x11_xid;
            wl_surface* wl_surface_ptr;
#endif
        } handle;
    } mup_wm_handle;

    /**
     * @brief An exported window handle, usually to pass to a child process.
     * @note On Wayland, this struct retains ownership of the string containing the ID handle.
     */
    typedef struct {
        mup_wm_platform platform;
        union {
#ifdef _WIN32
            HWND win32_hwnd;
#endif
#ifdef __linux__
            XID x11_xid;
            const char *wl_foreign_id;
#endif
        } handle;
    } mup_wm_export_handle;

    // common definitions
    // =============================

    /**
     * @brief Initializes the plugin. Should be called immediately after loading.
     *
     * @param exe_dir The directory in which mupen64(.exe) is located.
     * @param fwd_funcs A table with functions passed from the core.
     */
    MUPAPI_DEFINE_FN(core_result, mup_init, const char *exe_dir, const core_forward_funcs *fwd_funcs);

    /**
     * @brief Cleans up all plugin resources. Should be called immediately before unloading.
     */
    MUPAPI_DEFINE_FN(void, mup_cleanup);

    /**
     * @brief Displays the configuration GUI for this plugin.
     * 
     * Plugins are generally expected to create a subprocess and parent their window to `parent_window`.
     * 
     * @param parent_window The *exported* plugin handle.
     */
    MUPAPI_DEFINE_FN(void, mup_show_config, mup_wm_export_handle parent_window);

    /**
     * @brief Requests information from this plugin.
     * 
     * @param plugin_info A non-null pointer to a core_plugin_info struct to be filled with the needed information.
     */
    MUPAPI_DEFINE_FN(void, mup_get_info, core_plugin_info* plugin_info);

    /**
     * @brief Called just before the emulator starts execution.
     */
    MUPAPI_DEFINE_FN(void, mup_rom_opened);

    /**
     * @brief Called just before the emulator halts execution.
     */
    MUPAPI_DEFINE_FN(void, mup_rom_closed);

    



#undef MUPAPI_DEFINE_FN

#ifdef __cplusplus
}
#endif
#endif