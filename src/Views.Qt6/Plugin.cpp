/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Plugin.hpp"
#include "Main.hpp"

#include <print>

extern "C"
{
    void CALL M64RRBuiltinTASVideoGetMetadata(M64RRSpec::PluginMetadata *metadata);
    void CALL M64RRBuiltinTASVideoProcessEvent(M64RRSpec::Event event);
    void CALL M64RRBuiltinTASVideoProcessDList();
    void CALL M64RRBuiltinTASVideoShowConfig(M64RRSpec::WindowHandle parent_window);
    void CALL M64RRBuiltinTASVideoReadVideo(void *buffer, int32_t *width, int32_t *height);

    void CALL M64RRBuiltinTASAudioGetMetadata(M64RRSpec::PluginMetadata *metadata);
    void CALL M64RRBuiltinTASAudioProcessEvent(M64RRSpec::Event event);
    void CALL M64RRBuiltinTASAudioAIDacrateChanged(CoreSystemType system_type);
    void CALL M64RRBuiltinTASAudioAILenChanged();

    void CALL M64RRBuiltinNoInputGetMetadata(M64RRSpec::PluginMetadata *metadata);
    void CALL M64RRBuiltinNoInputProcessEvent(M64RRSpec::Event event);

    void CALL M64RRBuiltinTASRSPGetMetadata(M64RRSpec::PluginMetadata *metadata);
    void CALL M64RRBuiltinTASRSPProcessEvent(M64RRSpec::Event event);
    uint32_t CALL M64RRBuiltinTASRSPDoRSPCycles(uint32_t cycles);
}

static void dummy_process_event(M64RRSpec::Event)
{
}

static void dummy_process_dlist()
{
}

static size_t get_config_path(char *data, size_t size)
{
    static const std::u8string config_path = std::filesystem::absolute(IOUtils::config_path()).u8string();

    if (data == nullptr) return config_path.size() + 1;
    if (size < config_path.size() + 1) return 0;

    memcpy(data, config_path.c_str(), config_path.size() + 1);
    return size + 1;
}

struct PluginSet
{
    Plugin video;
    Plugin audio;
    Plugin input;
    Plugin rsp;

    PluginSet()
        : video(M64RRBuiltinTASVideoGetMetadata, M64RRBuiltinTASVideoProcessEvent, M64RRBuiltinTASVideoProcessDList,
                M64RRBuiltinTASVideoShowConfig, M64RRBuiltinTASVideoReadVideo),
          audio(M64RRBuiltinTASAudioGetMetadata, M64RRBuiltinTASAudioProcessEvent),
          input(M64RRBuiltinNoInputGetMetadata, M64RRBuiltinNoInputProcessEvent),
          rsp(M64RRBuiltinTASRSPGetMetadata, M64RRBuiltinTASRSPProcessEvent)
    {
    }
};

static std::optional<PluginSet> g_plugins;
static std::mutex g_plugin_lock;

Plugin::Plugin(M64RRSpec::PtrGetMetadata get_metadata, M64RRSpec::PtrProcessEvent process_event,
               M64RRSpec::PtrProcessDList process_dlist, M64RRSpec::PtrShowConfig show_config,
               M64RRSpec::PtrReadVideo read_video)
    : m_process_event(process_event), m_process_dlist(process_dlist), m_show_config(show_config),
      m_read_video(read_video)
{
    M64RRSpec::PluginMetadata metadata{};
    get_metadata(&metadata);

    m_name = {metadata.name, strnlen(metadata.name, sizeof(metadata.name))};
    m_type = metadata.type;
}

void Plugin::initiate()
{
    if (!m_init_data)
    {
        m_init_data.reset(new M64RRSpec::PluginInit);

        m_init_data->rom = g_core_ctx->rom;
        m_init_data->rdram = (uint8_t *)g_core_ctx->rdram;
        m_init_data->dmem = (uint8_t *)g_core_ctx->sp_dmem;
        m_init_data->imem = (uint8_t *)g_core_ctx->sp_imem;

        m_init_data->rdram_register = g_core_ctx->rdram_register;
        m_init_data->mi_register = g_core_ctx->mi_register;
        m_init_data->pi_register = g_core_ctx->pi_register;
        m_init_data->sp_register = g_core_ctx->sp_register;
        m_init_data->rsp_register = g_core_ctx->rsp_register;
        m_init_data->si_register = g_core_ctx->si_register;
        m_init_data->vi_register = g_core_ctx->vi_register;
        m_init_data->ri_register = g_core_ctx->ri_register;
        m_init_data->ai_register = g_core_ctx->ai_register;
        m_init_data->dpc_register = g_core_ctx->dpc_register;
        m_init_data->dps_register = g_core_ctx->dps_register;

        m_init_data->rcp_counter = g_core_ctx->rcp_counter;
        m_init_data->process_dlist = m_process_dlist;

        m_init_data->log_error = [](const char *msg) { std::println(stderr, "[ERROR] {}", msg); };
        m_init_data->log_warn = [](const char *msg) { std::println(stderr, "[WARN]  {}", msg); };
        m_init_data->log_info = [](const char *msg) { std::println(stderr, "[INFO]  {}", msg); };
        m_init_data->log_trace = [](const char *msg) { std::println(stderr, "[TRACE] {}", msg); };

        m_init_data->get_effective_speed_mode = []() { return g_core_ctx->vr_get_effective_speed_mode(); };
        m_init_data->frame_skipped = []() { return g_core_ctx->vr_get_frame_skipped(); };
        m_init_data->config_path = get_config_path;
        m_init_data->controllers = g_core_params.controls;

        m_init_data->request_size = [](uint32_t, uint32_t) {};
    }

    if (m_process_event)
        m_process_event(
            M64RRSpec::Event{.initiate = {.type = M64RRSpec::Event::Type::Initiate, .init = m_init_data.get()}});
}

void Plugin::bind_functions()
{
    switch (m_type)
    {
    case M64RRSpec::PluginType::Video:
        g_core_params.video_process_dlist = m_process_dlist ? m_process_dlist : dummy_process_dlist;
        g_core_params.video_get_video_size = [this](int32_t *width, int32_t *height) {
            read_video(nullptr, width, height);
        };
        break;
    case M64RRSpec::PluginType::Audio:
        g_core_params.audio_ai_dacrate_changed = M64RRBuiltinTASAudioAIDacrateChanged;
        g_core_params.audio_ai_len_changed = M64RRBuiltinTASAudioAILenChanged;
        break;
    case M64RRSpec::PluginType::Input:
        break;
    case M64RRSpec::PluginType::RSP:
        g_core_params.rsp_do_rsp_cycles = M64RRBuiltinTASRSPDoRSPCycles;
        break;
    }
}

void Plugin::send_event(M64RRSpec::Event event)
{
    if (m_process_event) m_process_event(event);
}

void Plugin::show_config(M64RRSpec::WindowHandle parent_window)
{
    if (m_show_config) m_show_config(parent_window);
}

void Plugin::read_video(void *buffer, int32_t *width, int32_t *height)
{
    if (m_read_video)
        m_read_video(buffer, width, height);
    else
    {
        if (width) *width = 0;
        if (height) *height = 0;
    }
}

bool PluginUtil::load_plugins()
{
    std::scoped_lock lock(g_plugin_lock);
    g_plugins.emplace();
    return true;
}

void PluginUtil::initiate_plugins()
{
    std::scoped_lock lock(g_plugin_lock);
    clear_plugin_funcs();

    if (!g_plugins.has_value()) abort();

    g_plugins->video.initiate();
    g_plugins->audio.initiate();
    g_plugins->input.initiate();
    g_plugins->rsp.initiate();
}

void PluginUtil::start_plugins()
{
    std::scoped_lock lock(g_plugin_lock);
    if (!g_plugins.has_value()) abort();

    g_plugins->video.bind_functions();
    g_plugins->audio.bind_functions();
    g_plugins->input.bind_functions();
    g_plugins->rsp.bind_functions();
    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomOpened});
}

void PluginUtil::stop_plugins()
{
    std::scoped_lock lock(g_plugin_lock);
    if (!g_plugins.has_value()) return;

    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomClosed});
    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown});
}

void PluginUtil::get_plugin_names(char *video, char *audio, char *input, char *rsp)
{
    std::scoped_lock lock(g_plugin_lock);
    if (!g_plugins.has_value()) return;

    if (video) strncpy_s(video, 64, g_plugins->video.name().c_str(), 64);
    if (audio) strncpy_s(audio, 64, g_plugins->audio.name().c_str(), 64);
    if (input) strncpy_s(input, 64, g_plugins->input.name().c_str(), 64);
    if (rsp) strncpy_s(rsp, 64, g_plugins->rsp.name().c_str(), 64);
}

void PluginUtil::send_event(M64RRSpec::Event event)
{
    if (!g_plugins.has_value()) abort();

    g_plugins->video.send_event(event);
    g_plugins->audio.send_event(event);
    g_plugins->input.send_event(event);
    g_plugins->rsp.send_event(event);
}
