/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Views.Win32/ZESpec.h>
#include <m64rr/Plugin.hpp>

struct ZESpecFuncs
{
    M64RRSpec::PluginInit video_init;
    ZESpec::ROMOPEN video_rom_open;
    ZESpec::ROMCLOSED video_rom_closed;
    ZESpec::CLOSEDLL video_close_dll;
    ZESpec::PROCESSDLIST video_process_dlist;
    ZESpec::PROCESSRDPLIST video_process_rdp_list;
    ZESpec::SHOWCFB video_show_cfb;
    ZESpec::VISTATUSCHANGED video_vi_status_changed;
    ZESpec::VIWIDTHCHANGED video_vi_width_changed;
    ZESpec::GETVIDEOSIZE video_get_video_size;
    ZESpec::FBREAD video_fb_read;
    ZESpec::FBWRITE video_fb_write;
    ZESpec::FBGETFRAMEBUFFERINFO video_fb_get_frame_buffer_info;
    ZESpec::CHANGEWINDOW video_change_window;
    ZESpec::UPDATESCREEN video_update_screen;
    ZESpec::READSCREEN video_read_screen;
    ZESpec::DLLCRTFREE video_dll_crt_free;
    ZESpec::MOVESCREEN video_move_screen;
    ZESpec::CAPTURESCREEN video_capture_screen;
    ZESpec::READVIDEO video_read_video;

    M64RRSpec::PluginInit audio_init;
    ZESpec::ROMOPEN audio_rom_open;
    ZESpec::ROMCLOSED audio_rom_closed;
    ZESpec::CLOSEDLL audio_close_dll_audio;
    std::function<void(CoreSystemType system_type)> audio_ai_dacrate_changed;
    ZESpec::AILENCHANGED audio_ai_len_changed;
    ZESpec::AIREADLENGTH audio_ai_read_length;
    ZESpec::PROCESSALIST audio_process_alist;
    ZESpec::AIUPDATE audio_ai_update;
    ZESpec::CLOSEDLL input_close_dll;
    ZESpec::ROMCLOSED input_rom_closed;
    ZESpec::ROMOPEN input_rom_open;

    M64RRSpec::PluginInit input_init;
    ZESpec::CONTROLLERCOMMAND input_controller_command;
    ZESpec::GETKEYS input_get_keys;
    ZESpec::SETKEYS input_set_keys;
    ZESpec::READCONTROLLER input_read_controller;
    ZESpec::KEYDOWN input_key_down;
    ZESpec::KEYUP input_key_up;

    M64RRSpec::PluginInit rsp_init;
    ZESpec::CLOSEDLL rsp_close_dll;
    ZESpec::ROMCLOSED rsp_rom_closed;
    ZESpec::DORSPCYCLES rsp_do_rsp_cycles;
};

class ZEPlugin;
class M64RRPlugin;

extern ZESpecFuncs g_plugin_funcs;
extern ZESpec::VideoPluginInfo gfx_info;
extern ZESpec::AudioPluginInfo audio_info;
extern ZESpec::InputPluginInfo control_info;
extern ZESpec::RSPPluginInfo rsp_info;
extern ZESpec::DLLABOUT dll_about;
extern ZESpec::DLLCONFIG dll_config;
extern ZESpec::DLLTEST dll_test;

#define FUNC(target, type, fallback, name)                                                                             \
    target = (type)GetProcAddress((HMODULE)m_module, name);                                                            \
    if (!target)                                                                                                       \
    {                                                                                                                  \
        target = fallback;                                                                                             \
        g_view_logger->info("Substituting dummy function for {}", name);                                               \
    }

class Plugin
{
  public:
    enum class Type
    {
        Video,
        Audio,
        Input,
        RSP,
    };

    /**
     * \brief Tries to create a plugin from the given path
     * \param path The path to a plugin
     * \return The operation status along with a pointer to the plugin. The pointer will be invalid if the first pair
     * element isn't an empty string.
     */
    static std::pair<std::wstring, std::unique_ptr<Plugin>> create(std::filesystem::path path);

    Plugin() = default;
    virtual ~Plugin();

    /**
     * \brief Opens the plugin configuration dialog.
     * \param hwnd The parent window handle.
     */
    virtual void config(HWND hwnd);

    /**
     * \brief Opens the plugin test dialog
     * \param hwnd The parent window handle.
     */
    virtual void test(HWND hwnd);

    /**
     * \brief Opens the plugin about dialog
     * \param hwnd The parent window handle.
     */
    virtual void about(HWND hwnd);

    /**
     * \brief Loads the plugin's exported functions into `funcs` and calls the initiate function.
     * \param funcs The function table to load the plugin's exported functions into.
     */
    virtual void initiate(ZESpecFuncs &funcs);

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

  protected:
    /**
     * \brief Initiates the plugin for being called ephemerally (e.g. via `config()`, `test()`, `about()`)
     */
    virtual void initiate_dummy();
    virtual void deinitiate_dummy();

    std::filesystem::path m_path;
    std::string m_name;
    Type m_type;
    HMODULE m_module;
};

/**
 * Represents the result of a plugin discovery operation.
 */
typedef struct
{
    /**
     * The discovered plugins matching the plugin API surface.
     */
    std::vector<std::unique_ptr<Plugin>> plugins;

    /**
     * Vector of discovered plugins and their results.
     */
    std::vector<std::pair<std::filesystem::path, std::string>> results;

} t_plugin_discovery_result;

/**
 * \brief A module providing utility functions related to plugins.
 */
namespace PluginUtil
{
/**
 * \brief Initializes the plugin utility module.
 */
void init();

/**
 * \brief Discovers plugins in the given directory.
 * \param directory The directory to search for plugins in.
 * \return The plugin discovery result.
 */
t_plugin_discovery_result discover_plugins(const std::filesystem::path &directory);

/**
 * \return Whether MGE functionality is currently available.
 */
bool mge_available();

/**
 * \brief Prepares and starts the currently loaded plugins to be used by the core.
 */
void start_plugins();

/**
 * \brief Stops and unloads the currently loaded plugins.
 */
void stop_plugins();

/**
 * \brief Loads the plugins specified in the configuration, filling out the global plugin function registry.
 * \return Whether the operation succeeded.
 */
bool load_plugins();

/**
 * \brief Initiates the currently loaded plugins.
 */
void initiate_plugins();

/**
 * \brief Copies the names of the currently loaded plugins into the provided buffers.
 * \param video The video plugin name buffer of size `64` (including NUL terminator).
 * \param audio The audio plugin name buffer of size `64` (including NUL terminator).
 * \param input The input plugin name buffer of size `64` (including NUL terminator).
 * \param rsp The RSP plugin name buffer of size `64` (including NUL terminator).
 */
void get_plugin_names(char *video, char *audio, char *input, char *rsp);

/**
 * \brief Takes a screenshot to the specified directory.
 * \param path The directory to save the screenshot to. If a file path, the screenshot will be saved to the directory
 * containing that file.
 */
void screenshot(const std::filesystem::path &path);

/**
 * \brief Tries to find the free function exported by the CRT in the specified module.
 * \param module The module to search in.
 * \return A pointer to the free function, or the CRT's free if not found via DllCrtFree.
 */
ZESpec::DLLCRTFREE get_free_function_in_module(HMODULE module);

void get_video_size(int32_t *width, int32_t *height);
void read_video(void *buffer);
void update_screen();
void key_down(uint32_t wParam, int32_t lParam);
void key_up(uint32_t wParam, int32_t lParam);
void move_screen(uint32_t wParam, int32_t lParam);

} // namespace PluginUtil
