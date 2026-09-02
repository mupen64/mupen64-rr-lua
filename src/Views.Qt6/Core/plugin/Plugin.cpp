/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Plugin.hpp"

#include <decan.hpp>

#include "EmuContext.hpp"

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

// namespace
// {
// struct PluginSet
// {
//     Plugin video;
//     Plugin audio;
//     Plugin input;
//     Plugin rsp;
// };
// } // namespace

// static std::optional<PluginSet> g_plugins = std::nullopt;
// static std::mutex g_plugin_lock;

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

    std::string_view name = {metadata.name, strnlen(metadata.name, sizeof(metadata.name))};
    m_name = name;

    m_type = metadata.type;

    m_process_event = (M64RRSpec::PtrProcessEvent)load_symbol("M64RRProcessEvent");
}

void Plugin::initiate(core_ctx *ctx, core_params &params, const std::function<void(M64RRSpec::PluginInit *)> &post_init)
{
    m_init_data.reset(new M64RRSpec::PluginInit);

    m_init_data->rom = ctx->rom;
    m_init_data->rdram = (uint8_t *)ctx->rdram;
    m_init_data->dmem = (uint8_t *)ctx->sp_dmem;
    m_init_data->imem = (uint8_t *)ctx->sp_imem;

    m_init_data->log_trace = [](const char *msg) { std::println(stderr, "[PTRACE] {}", msg); };
    m_init_data->log_info = [](const char *msg) { std::println(stderr, "[PINFO]  {}", msg); };
    m_init_data->log_warn = [](const char *msg) { std::println(stderr, "[PWARN]  {}", msg); };
    m_init_data->log_error = [](const char *msg) { std::println(stderr, "[PERROR] {}", msg); };

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

    m_init_data->get_effective_speed_mode = []() { return EmuContext::rawContext()->vr_get_effective_speed_mode(); };
    m_init_data->frame_skipped = []() { return EmuContext::rawContext()->vr_get_frame_skipped(); };
    m_init_data->config_path = get_config_path;

    // TODO: handle this!
    m_init_data->request_size = [](uint32_t, uint32_t) {};

    m_init_data->controllers = params.controls;

    if (post_init) post_init(m_init_data.get());

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
        load_core_function<M64RRSpec::PtrAIDacrateChanged>(
            *this, "M64RRAIDacrateChanged", params.audio_ai_dacrate_changed);
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

void *Plugin::load_symbol(const char *symbol) const
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

PluginSet::PluginSet(Plugin &&video, Plugin &&audio, Plugin &&input, Plugin &&rsp)
    : m_video(std::move(video)), m_audio(std::move(audio)), m_input(std::move(input)), m_rsp(std::move(rsp)),
      m_video_process_dlist((M64RRSpec::PtrProcessDList)m_video.load_symbol("M64RRProcessDList"))
{
}

void PluginSet::initiate_plugins(core_ctx *core_ctx, core_params &core_params)
{
    CoreUtil::clear_plugin_funcs(core_params);

    m_video.initiate(core_ctx, core_params, [](M64RRSpec::PluginInit *init) {
        init->request_size = [](uint32_t width, uint32_t height) {
            // must be called on GUI thread!
            QMetaObject::invokeMethod(EmuContext::instance(), &EmuContext::gfxRequestSize, width, height);
        };
    });
    m_audio.initiate(core_ctx, core_params);
    m_input.initiate(core_ctx, core_params);
    m_rsp.initiate(core_ctx, core_params, [&](M64RRSpec::PluginInit *init) {
        init->process_dlist = (M64RRSpec::PtrProcessDList)m_video.load_symbol("M64RRProcessDList");
    });
}

void PluginSet::emu_started(core_params &core_params)
{
    const auto opened_event = M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomOpened};

    m_video.bind_functions(core_params);
    m_audio.bind_functions(core_params);
    m_input.bind_functions(core_params);
    m_rsp.bind_functions(core_params);

    m_video.send_event(opened_event);
    m_audio.send_event(opened_event);
    m_input.send_event(opened_event);
    m_rsp.send_event(opened_event);
}

void PluginSet::emu_stopped(core_params &core_params)
{
    const auto closed_event = M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomClosed};
    const auto shutdown_event = M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown};

    m_video.send_event(closed_event);
    m_audio.send_event(closed_event);
    m_input.send_event(closed_event);
    m_rsp.send_event(closed_event);

    m_video.send_event(shutdown_event);
    m_audio.send_event(shutdown_event);
    m_input.send_event(shutdown_event);
    m_rsp.send_event(shutdown_event);

    CoreUtil::clear_plugin_funcs(core_params);
}

void PluginSet::get_plugin_names(char *video, char *audio, char *input, char *rsp)
{
    if (video) strncpy_s(video, 64, m_video.name().c_str(), 64);
    if (audio) strncpy_s(audio, 64, m_audio.name().c_str(), 64);
    if (input) strncpy_s(input, 64, m_input.name().c_str(), 64);
    if (rsp) strncpy_s(rsp, 64, m_rsp.name().c_str(), 64);
}
