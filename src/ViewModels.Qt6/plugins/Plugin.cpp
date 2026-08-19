/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ViewModels.Qt6/plugins/Plugin.hpp"
#include "VersionNameHelpers.hpp"
#include <CommonPCH.hpp>

#include <print>
#include <variant>
#include <decan.hpp>

#include <ViewModels.Qt6/Core.hpp>

template <class T>
static inline void load_core_function(Plugin &plugin, const char *symbol, std::function<std::remove_pointer_t<T>> &func)
{
    auto pointer = (T)plugin.load_symbol(symbol);
    if (pointer != nullptr) func = pointer;
}

static size_t get_config_path(char *data, size_t size)
{
    static const std::u8string config_path = std::filesystem::absolute(IOUtils::config_path()).u8string();

    if (data == nullptr) return config_path.size() + 1;
    if (size < config_path.size() + 1) return 0;

    memcpy(data, config_path.c_str(), config_path.size() + 1);
    return size + 1;
}

namespace
{
struct PluginSet
{
    Plugin video;
    Plugin audio;
    Plugin input;
    Plugin rsp;
};
} // namespace

static std::optional<PluginSet> g_plugins = std::nullopt;
static std::mutex g_plugin_lock;

Plugin::Plugin(const std::filesystem::path &path) : m_lib(std::in_place_type<decan::library>, path), m_path(path)
{
    init_common();
}
Plugin::Plugin(BuiltinTAS::PluginID id) : m_lib(id), m_path()
{
    init_common();
}
void Plugin::init_common()
{
    // metadata (required)
    auto get_metadata = (M64RRSpec::PtrGetMetadata)load_symbol("M64RRGetMetadata");
    if (get_metadata == nullptr) throw std::invalid_argument("Provided plugin does not define M64RRGetMetadata");

    M64RRSpec::PluginMetadata metadata;
    get_metadata(&metadata);

    // Plugins are tied to one version of Mupen
    std::string_view target_version{metadata.target_version,
                                    strnlen(metadata.target_version, sizeof(metadata.target_version))};
    if (!target_version.empty() && target_version != CURRENT_VERSION)
        throw PluginLoadFailed("Expected target version " CURRENT_VERSION);

    std::string_view name = {metadata.name, strnlen(metadata.name, sizeof(metadata.name))};
    m_name = name;

    m_type = metadata.type;

    m_process_event = (M64RRSpec::PtrProcessEvent)load_symbol("M64RRProcessEvent");
}

void Plugin::initiate(core_ctx *ctx, core_params &params)
{
    if (!m_init_data)
    {
        m_init_data.reset(new M64RRSpec::PluginInit);

        m_init_data->rom = ctx->rom;
        m_init_data->rdram = (uint8_t *)ctx->rdram;
        m_init_data->dmem = (uint8_t *)ctx->sp_dmem;
        m_init_data->imem = (uint8_t *)ctx->sp_imem;

        m_init_data->rdram_register = ctx->rdram_register;
        m_init_data->mi_register = ctx->mi_register;
        m_init_data->pi_register = ctx->pi_register;
        m_init_data->sp_register = ctx->sp_register;
        m_init_data->rsp_register = ctx->rsp_register;
        m_init_data->si_register = ctx->si_register;
        m_init_data->vi_register = ctx->vi_register;
        m_init_data->ri_register = ctx->ri_register;
        m_init_data->ai_register = ctx->ai_register;
        m_init_data->dpc_register = ctx->dpc_register;
        m_init_data->dps_register = ctx->dps_register;

        m_init_data->rcp_counter = ctx->rcp_counter;

        auto *video_process_dlist_ptr = (M64RRSpec::PtrProcessDList)load_symbol("M64RRProcessDList");
        m_init_data->process_dlist = video_process_dlist_ptr;

        m_init_data->log_error = [](const char *msg) { std::println(stderr, "[ERROR] {}", msg); };
        m_init_data->log_warn = [](const char *msg) { std::println(stderr, "[WARN]  {}", msg); };
        m_init_data->log_info = [](const char *msg) { std::println(stderr, "[INFO]  {}", msg); };
        m_init_data->log_trace = [](const char *msg) { std::println(stderr, "[TRACE] {}", msg); };

        m_init_data->get_effective_speed_mode = []() { return Core::context()->vr_get_effective_speed_mode(); };
        m_init_data->frame_skipped = []() { return Core::context()->vr_get_frame_skipped(); };
        m_init_data->config_path = get_config_path;

        m_init_data->controllers = params.controls;
    }

    M64RRSpec::Event init_event{.initiate = {.type = M64RRSpec::Event::Type::Initiate, .init = m_init_data.get()}};

    if (m_process_event) m_process_event(init_event);
}

void Plugin::bind_functions(core_params &params)
{
    switch (m_type)
    {
    case M64RRSpec::PluginType::Video:
        load_core_function<M64RRSpec::PtrProcessDList>(*this, "M64RRProcessDList", params.video_process_dlist);
        break;
    case M64RRSpec::PluginType::Audio:
        load_core_function<M64RRSpec::PtrAIDacrateChanged>(*this, "M64RRAIDacrateChanged",
                                                           params.audio_ai_dacrate_changed);
        load_core_function<M64RRSpec::PtrAILenChanged>(*this, "M64RRAILenChanged", params.audio_ai_len_changed);
        break;
    case M64RRSpec::PluginType::Input:
        load_core_function<M64RRSpec::PtrGetKeys>(*this, "M64RRGetKeys", params.input_get_keys);
        load_core_function<M64RRSpec::PtrSetKeys>(*this, "M64RRSetKeys", params.input_set_keys);
        load_core_function<M64RRSpec::PtrReadController>(*this, "M64RRReadController", params.input_read_controller);
        break;
    case M64RRSpec::PluginType::RSP:
        load_core_function<M64RRSpec::PtrDoRSPCycles>(*this, "M64RRDoRSPCycles", params.rsp_do_rsp_cycles);
        break;
    }
}

void Plugin::send_event(M64RRSpec::Event event)
{
    if (m_process_event) m_process_event(event);
}

void *Plugin::load_symbol(const char *symbol)
{
    const auto visitor = MiscHelpers::Overload{
        [=](const decan::library &lib) -> void * {
            try
            {
                return lib.get(symbol);
            }
            catch (const decan::dll_error &)
            {
                return nullptr;
            }
        },
        [=](BuiltinTAS::PluginID id) -> void * { return BuiltinTAS::builtin_dlsym(id, symbol); },
    };
    return std::visit(visitor, m_lib);
}

bool PluginUtil::load_plugins()
{
    try
    {
        std::scoped_lock lock(g_plugin_lock);
        auto video_plugin = Plugin(BuiltinTAS::PluginID::DummyVideo);
        auto audio_plugin = Plugin(BuiltinTAS::PluginID::TASAudio);
        auto input_plugin = Plugin(BuiltinTAS::PluginID::DummyInput);
        auto rsp_plugin = Plugin(BuiltinTAS::PluginID::TASRSP);
        g_plugins.emplace(std::move(video_plugin), std::move(audio_plugin), std::move(input_plugin),
                          std::move(rsp_plugin));
        return true;
    }
    catch (const std::exception &err)
    {
        std::println(stderr, "[ERROR] Plugin load failed: {}", err.what());
        return false;
    }
}
void PluginUtil::initiate_plugins(core_ctx *ctx, core_params &params)
{
    std::scoped_lock lock(g_plugin_lock);
    Core::clear_plugin_funcs(params);

    if (!g_plugins.has_value()) abort();

    g_plugins->video.initiate(ctx, params);
    g_plugins->audio.initiate(ctx, params);
    g_plugins->input.initiate(ctx, params);
    g_plugins->rsp.initiate(ctx, params);
}
void PluginUtil::start_plugins(core_params &params)
{

    g_plugins->video.bind_functions(params);
    g_plugins->audio.bind_functions(params);
    g_plugins->input.bind_functions(params);
    g_plugins->rsp.bind_functions(params);
    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomOpened});
}
void PluginUtil::stop_plugins()
{
    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomClosed});
    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown});
    Core::clear_plugin_funcs(Core::params());
}
void PluginUtil::get_plugin_names(char *video, char *audio, char *input, char *rsp)
{
    std::scoped_lock lock(g_plugin_lock);
    if (video) strncpy_s(video, 64, g_plugins->video.name().c_str(), 64);
    if (audio) strncpy_s(audio, 64, g_plugins->audio.name().c_str(), 64);
    if (input) strncpy_s(input, 64, g_plugins->input.name().c_str(), 64);
    if (rsp) strncpy_s(rsp, 64, g_plugins->rsp.name().c_str(), 64);
}
void PluginUtil::send_event(M64RRSpec::Event event)
{
    std::scoped_lock lock(g_plugin_lock);

    if (!g_plugins.has_value()) abort();

    g_plugins->video.send_event(event);
    g_plugins->audio.send_event(event);
    g_plugins->input.send_event(event);
    g_plugins->rsp.send_event(event);
}