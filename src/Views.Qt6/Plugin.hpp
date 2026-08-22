/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <m64rr/API.hpp>
#include <m64rr/Plugin.hpp>

class Plugin
{
  public:
    Plugin(M64RRSpec::PtrGetMetadata get_metadata, M64RRSpec::PtrProcessEvent process_event,
           M64RRSpec::PtrProcessDList process_dlist = nullptr, M64RRSpec::PtrShowConfig show_config = nullptr,
           M64RRSpec::PtrReadVideo read_video = nullptr);

    void initiate();
    void bind_functions();
    void send_event(M64RRSpec::Event event);
    void show_config(M64RRSpec::WindowHandle parent_window = {});
    void read_video(void *buffer, int32_t *width, int32_t *height);

    const std::string &name() const { return m_name; }

  private:
    std::string m_name;
    M64RRSpec::PluginType m_type;
    M64RRSpec::PtrProcessEvent m_process_event;
    M64RRSpec::PtrProcessDList m_process_dlist;
    M64RRSpec::PtrShowConfig m_show_config;
    M64RRSpec::PtrReadVideo m_read_video;
    std::unique_ptr<M64RRSpec::PluginInit> m_init_data;
};

namespace PluginUtil
{
bool load_plugins();
void initiate_plugins();
void start_plugins();
void stop_plugins();
void get_plugin_names(char *video, char *audio, char *input, char *rsp);
void send_event(M64RRSpec::Event event);
} // namespace PluginUtil