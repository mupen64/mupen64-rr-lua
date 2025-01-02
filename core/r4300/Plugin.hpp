#pragma once
/**
 * Mupen64 - Plugin.hpp
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 **/

#include <functional>
#include <memory>
#include <shared/helpers/IOHelpers.h>
#include <shared/types/CoreTypes.h>
#include <stdlib.h>
#include <string>
#include <vector>

// ReSharper disable CppInconsistentNaming

static uint32_t __cdecl dummy_doRspCycles(uint32_t Cycles) { return Cycles; };

extern CONTROL Controls[4];

#pragma region Base Functions

typedef void(__cdecl* GETDLLINFO)(PLUGIN_INFO*);
typedef void(__cdecl* DLLCONFIG)(void*);
typedef void(__cdecl* DLLTEST)(void*);
typedef void(__cdecl* DLLABOUT)(void*);
extern GETDLLINFO getDllInfo;
extern DLLCONFIG dllConfig;
extern DLLTEST dllTest;
extern DLLABOUT dllAbout;

#pragma endregion

#pragma region Video Functions

typedef void(__cdecl* CHANGEWINDOW)();
typedef void(__cdecl* CLOSEDLL_GFX)();
typedef int32_t(__cdecl* INITIATEGFX)(GFX_INFO);
typedef void(__cdecl* PROCESSDLIST)();
typedef void(__cdecl* PROCESSRDPLIST)();
typedef void(__cdecl* ROMCLOSED_GFX)();
typedef void(__cdecl* ROMOPEN_GFX)();
typedef void(__cdecl* SHOWCFB)();
typedef void(__cdecl* UPDATESCREEN)();
typedef void(__cdecl* VISTATUSCHANGED)();
typedef void(__cdecl* VIWIDTHCHANGED)();
typedef void(__cdecl* READSCREEN)(void**, int32_t*, int32_t*);
typedef void(__cdecl* DLLCRTFREE)(void*);
typedef void(__cdecl* MOVESCREEN)(int32_t, int32_t);
typedef void(__cdecl* CAPTURESCREEN)(char*);
typedef void(__cdecl* GETVIDEOSIZE)(int32_t*, int32_t*);
typedef void(__cdecl* READVIDEO)(void**);
typedef void(__cdecl* FBREAD)(uint32_t);
typedef void(__cdecl* FBWRITE)(uint32_t addr, uint32_t size);
typedef void(__cdecl* FBGETFRAMEBUFFERINFO)(void*);
extern CHANGEWINDOW changeWindow;
extern CLOSEDLL_GFX closeDLL_gfx;
extern INITIATEGFX initiateGFX;
extern PROCESSDLIST processDList;
extern PROCESSRDPLIST processRDPList;
extern ROMCLOSED_GFX romClosed_gfx;
extern ROMOPEN_GFX romOpen_gfx;
extern SHOWCFB showCFB;
extern UPDATESCREEN updateScreen;
extern VISTATUSCHANGED viStatusChanged;
extern VIWIDTHCHANGED viWidthChanged;
extern READSCREEN readScreen;
extern DLLCRTFREE DllCrtFree;
extern MOVESCREEN moveScreen;
extern CAPTURESCREEN CaptureScreen;
// Mupen Graphics Extension, currently only implemented by bettergln64/ngl64
extern GETVIDEOSIZE get_video_size;
extern READVIDEO read_video;
// Framebuffer Extension
extern FBREAD fBRead;
extern FBWRITE fBWrite;
extern FBGETFRAMEBUFFERINFO fBGetFrameBufferInfo;

#pragma endregion

#pragma region Audio Functions

typedef void(__cdecl* AIDACRATECHANGED)(int32_t system_type);
typedef void(__cdecl* AILENCHANGED)();
typedef uint32_t(__cdecl* AIREADLENGTH)();
typedef void(__cdecl* CLOSEDLL_AUDIO)();
typedef int32_t(__cdecl* INITIATEAUDIO)(AUDIO_INFO);
typedef void(__cdecl* PROCESSALIST)();
typedef void(__cdecl* ROMCLOSED_AUDIO)();
typedef void(__cdecl* ROMOPEN_AUDIO)();
typedef void(__cdecl* AIUPDATE)(int32_t wait);
extern AIDACRATECHANGED aiDacrateChanged;
extern AILENCHANGED aiLenChanged;
extern AIREADLENGTH aiReadLength;
extern CLOSEDLL_AUDIO closeDLL_audio;
extern INITIATEAUDIO initiateAudio;
extern PROCESSALIST processAList;
extern ROMCLOSED_AUDIO romClosed_audio;
extern ROMOPEN_AUDIO romOpen_audio;
extern AIUPDATE aiUpdate;

#pragma endregion

#pragma region Input Functions

typedef void(__cdecl* CLOSEDLL_INPUT)();
typedef void(__cdecl* CONTROLLERCOMMAND)(int32_t controller, unsigned char* command);
typedef void(__cdecl* GETKEYS)(int32_t controller, BUTTONS* keys);
typedef void(__cdecl* SETKEYS)(int32_t controller, BUTTONS keys);
typedef void(__cdecl* OLD_INITIATECONTROLLERS)(void* hwnd, CONTROL controls[4]);
typedef void(__cdecl* INITIATECONTROLLERS)(CONTROL_INFO control_info);
typedef void(__cdecl* READCONTROLLER)(int32_t controller, unsigned char* command);
typedef void(__cdecl* ROMCLOSED_INPUT)();
typedef void(__cdecl* ROMOPEN_INPUT)();
typedef void(__cdecl* KEYDOWN)(uint32_t wParam, int32_t lParam);
typedef void(__cdecl* KEYUP)(uint32_t wParam, int32_t lParam);
extern CLOSEDLL_INPUT closeDLL_input;
extern CONTROLLERCOMMAND controllerCommand;
extern GETKEYS getKeys;
extern SETKEYS setKeys;
extern OLD_INITIATECONTROLLERS old_initiateControllers;
extern INITIATECONTROLLERS initiateControllers;
extern READCONTROLLER readController;
extern ROMCLOSED_INPUT romClosed_input;
extern ROMOPEN_INPUT romOpen_input;
extern KEYDOWN keyDown;
extern KEYUP keyUp;

#pragma endregion

#pragma region RSP Functions

typedef void(__cdecl* CLOSEDLL_RSP)();
typedef uint32_t(__cdecl* DORSPCYCLES)(uint32_t);
typedef void(__cdecl* INITIATERSP)(RSP_INFO rsp_info, uint32_t* cycles);
typedef void(__cdecl* ROMCLOSED_RSP)();
extern CLOSEDLL_RSP closeDLL_RSP;
extern DORSPCYCLES doRspCycles;
extern INITIATERSP initiateRSP;
extern ROMCLOSED_RSP romClosed_RSP;

#pragma endregion

// ReSharper restore CppInconsistentNaming

// frame buffer plugin spec extension
typedef struct {
    uint32_t addr;
    uint32_t size;
    uint32_t width;
    uint32_t height;
} FrameBufferInfo;

class Plugin {
public:
    /**
     * \brief Tries to create a plugin from the given path
     * \param path The path to a plugin
     * \return The operation status along with a pointer to the plugin. The
     * pointer will be invalid if the first pair element isn't an empty string.
     */
    static std::pair<std::wstring, std::unique_ptr<Plugin>> create(std::filesystem::path path);

    Plugin() = default;
    ~Plugin();

    /**
     * \brief Opens the plugin configuration dialog
     */
    void config();

    /**
     * \brief Opens the plugin test dialog
     */
    void test();

    /**
     * \brief Opens the plugin about dialog
     */
    void about();

    /**
     * \brief Loads the plugin's exported functions into the globals
     */
    void load_into_globals();

    /**
     * \brief Gets the plugin's path
     */
    auto path() const { return m_path; }

    /**
     * \brief Gets the plugin's name
     */
    auto name() const { return m_name; }

    /**
     * \brief Gets the plugin's type
     */
    auto type() const { return m_type; }

    /**
     * \brief Gets the plugin's version
     */
    auto version() const { return m_version; }

private:
    std::filesystem::path m_path;
    std::string m_name;
    PluginType m_type;
    uint16_t m_version;
    void* m_module;
};

/// <summary>
/// Initializes dummy info used by per-plugin functions
/// </summary>
void setup_dummy_info();

/**
 * \brief Whether the MGE compositor is available
 */
inline bool is_mge_available() { return get_video_size && read_video; }
