/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <variant>

#include <decan.hpp>
#include <m64rr/API.hpp>
#include <m64rr/Plugin.hpp>
#include "BuiltinTAS.hpp"

class Plugin
{
  public:
    Plugin(const std::filesystem::path &path);
    Plugin(BuiltinTAS::PluginID id);

    /**
     * @brief Triggers the `Initiate` event and sets up necessary initialization data.
     */
    void initiate(core_ctx *core_ctx, core_params &core_params);

    /**
     * @brief Binds the needed functions from this plugin to the core.
     */
    void bind_functions(core_params &core_params);

    /**
     * @brief Triggers an arbitrary lifecycle event.
     *
     * @param event The event
     */
    void send_event(M64RRSpec::Event event);

    const std::string &name() const { return m_name; }

    /**
     * @brief Tries to load a symbol from the DLL.
     *
     * @return the symbol's address, or nullptr if it doesn't exist
     */
    void *load_symbol(const char *symbol);

  private:
    void init_common();

    std::variant<decan::library, BuiltinTAS::PluginID> m_lib;

    std::filesystem::path m_path;
    std::string m_name;
    M64RRSpec::PluginType m_type;
    M64RRSpec::PtrProcessEvent m_process_event;
    std::unique_ptr<M64RRSpec::PluginInit> m_init_data;
};

namespace PluginUtil
{
void start_plugins(core_params &core_params);
void stop_plugins();
void get_plugin_names(char *video, char *audio, char *input, char *rsp);
void send_event(M64RRSpec::Event event);
} // namespace PluginUtil