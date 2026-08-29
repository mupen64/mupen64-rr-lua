/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <variant>

#include <decan.hpp>
#include <Core/API.hpp>
#include <Core/Plugin.hpp>
#include "BuiltinTAS.hpp"

class PluginLoadFailed : std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

class Plugin
{
  public:
    Plugin(const std::filesystem::path &path);
    Plugin(BuiltinTAS::PluginID id);

    /**
     * @brief Triggers the `Initiate` event and sets up necessary initialization data.
     */
    void initiate(core_ctx *core_ctx, CoreParams &core_params,
        const std::function<void(M64RRSpec::PluginInit *)> &post_init = {});

    /**
     * @brief Binds the needed functions from this plugin to the core.
     */
    void bind_functions(CoreParams &core_params);

    /**
     * @brief Triggers an arbitrary lifecycle event.
     *
     * @param event The event
     */
    void send_event(M64RRSpec::Event event);

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
     * @brief Tries to load a symbol from the DLL.
     *
     * @return the symbol's address, or nullptr if it doesn't exist
     */
    void *load_symbol(const char *symbol) const;

  private:
    void init_common();

    std::variant<decan::library, BuiltinTAS::PluginID> m_lib;

    std::filesystem::path m_path;
    std::string m_name;
    M64RRSpec::PluginType m_type;

    M64RRSpec::PtrProcessEvent m_process_event;

    std::unique_ptr<M64RRSpec::PluginInit> m_init_data;
};

class PluginSet
{
  public:
    PluginSet(Plugin &&video, Plugin &&audio, Plugin &&input, Plugin &&rsp);

    const Plugin &video() const { return m_video; }
    const Plugin &audio() const { return m_audio; }
    const Plugin &input() const { return m_input; }
    const Plugin &rsp() const { return m_rsp; }

    void initiate_plugins(core_ctx *core_ctx, CoreParams &core_params);
    void emu_started(CoreParams &core_params);
    void emu_stopped(CoreParams &core_params);

    void get_plugin_names(char *video, char *audio, char *input, char *rsp);

  private:
    Plugin m_video;
    Plugin m_audio;
    Plugin m_input;
    Plugin m_rsp;

    M64RRSpec::PtrProcessDList m_video_process_dlist;
};

// namespace PluginUtil
// {
// bool load_plugins();
// void initiate_plugins(core_ctx *core_ctx, core_params &core_params);
// void start_plugins(core_params &core_params);
// void stop_plugins();
// void get_plugin_names(char *video, char *audio, char *input, char *rsp);
// void send_event(M64RRSpec::Event event);
// } // namespace PluginUtil
