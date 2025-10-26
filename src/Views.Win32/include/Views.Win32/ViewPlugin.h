/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Describes the Mupen64 view-side Plugin API.
 *
 * This header can be used standalone by Mupen64 plugins, just make sure to define PLUGIN_WITH_CALLBACKS first.
 *
 */

#pragma once

#include "core_plugin.h"

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
     * \brief Exposes an extended set of functions to plugins.
     */
    struct core_plugin_extended_funcs
    {
        /**
         * \brief Size of the structure in bytes.
         */
        uint32_t size;

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
    };

    typedef void(CALL *CLOSEDLL)();
    typedef void(CALL *DLLABOUT)(void *);
    typedef void(CALL *DLLCONFIG)(void *);
    typedef void(CALL *DLLTEST)(void *);
    typedef void(CALL *GETDLLINFO)(core_plugin_info *);
    typedef void(CALL *RECEIVEEXTENDEDFUNCS)(core_plugin_extended_funcs *);

    typedef void(CALL *CHANGEWINDOW)();
    typedef int32_t(CALL *INITIATEGFX)(core_gfx_info);
    typedef void(CALL *UPDATESCREEN)();
    typedef void(CALL *READSCREEN)(void **, int32_t *, int32_t *);
    typedef void(CALL *DLLCRTFREE)(void *);
    typedef void(CALL *MOVESCREEN)(int32_t, int32_t);
    typedef void(CALL *CAPTURESCREEN)(char *);
    typedef void(CALL *READVIDEO)(void **);

    typedef int32_t(CALL *INITIATEAUDIO)(core_audio_info);

    typedef void(CALL *OLD_INITIATECONTROLLERS)(void *hwnd, core_controller controls[4]);
    typedef void(CALL *INITIATECONTROLLERS)(core_input_info control_info);
    typedef void(CALL *KEYDOWN)(uint32_t wParam, int32_t lParam);
    typedef void(CALL *KEYUP)(uint32_t wParam, int32_t lParam);

    typedef void(CALL *INITIATERSP)(core_rsp_info rsp_info, uint32_t *cycles);

#if defined(PLUGIN_WITH_CALLBACKS)

    // ReSharper disable CppInconsistentNaming


#pragma region Base

    EXPORT void CALL CloseDLL(void);
    EXPORT void CALL DllAbout(void *hParent);
    EXPORT void CALL DllConfig(void *hParent);
    EXPORT void CALL GetDllInfo(core_plugin_info *PluginInfo);
    EXPORT void CALL RomClosed(void);
    EXPORT void CALL RomOpen(void);
    /**
     * Called by the core to provide the plugin with a set of extended functions.
     * The plugin can store this pointer for use throughout its lifetime.
     * This function is called before the plugin-specific InitiateXXX function.
     */
    EXPORT void CALL ReceiveExtendedFuncs(core_plugin_extended_funcs *);

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
    EXPORT void CALL mge_read_video(void **);

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