/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.hpp>
#include <DialogService.hpp>
#include <components/Statusbar.hpp>
#include <plugin/M64RRPlugin.hpp>
#include <plugin/Plugin.hpp>

static M64RRSpec::PtrProcessEvent s_mupenrr_video_event_fn = nullptr;
static M64RRSpec::PtrProcessDList s_mupenrr_process_dlist_fn = nullptr;
static M64RRSpec::PtrReadVideo s_mupenrr_read_video_fn = nullptr;

static M64RRSpec::PtrProcessEvent s_mupenrr_audio_event_fn = nullptr;
static M64RRSpec::PtrAIDacrateChanged s_mupenrr_ai_dacrate_changed_fn = nullptr;
static M64RRSpec::PtrAILenChanged s_mupenrr_ai_len_changed_fn = nullptr;

static M64RRSpec::PtrProcessEvent s_mupenrr_input_event_fn = nullptr;
static M64RRSpec::PtrGetKeys s_mupenrr_get_keys_fn = nullptr;
static M64RRSpec::PtrSetKeys s_mupenrr_set_keys_fn = nullptr;
static M64RRSpec::PtrReadController s_mupenrr_read_controller_fn = nullptr;

static M64RRSpec::PtrProcessEvent s_mupenrr_rsp_event_fn = nullptr;
static M64RRSpec::PtrDoRSPCycles s_mupenrr_do_rsp_cycles_fn = nullptr;

#define LOOKUP_MUPENRR_FN(mupenrr_ptr, mupenrr_type, export_name)                                                      \
    mupenrr_ptr = (mupenrr_type)GetProcAddress(m_module, export_name);

static CoreController controller_to_core_controller(const M64RRSpec::Controller &controller)
{
    CoreControllerExtension extension;
    switch (controller.plugin)
    {
    case M64RRSpec::ControllerExtension::None:
        extension = CoreControllerExtension::None;
        break;
    case M64RRSpec::ControllerExtension::Mempak:
        extension = CoreControllerExtension::Mempak;
        break;
    case M64RRSpec::ControllerExtension::Rumblepak:
        extension = CoreControllerExtension::Rumblepak;
        break;
    case M64RRSpec::ControllerExtension::Transferpak:
        extension = CoreControllerExtension::Transferpak;
        break;
    case M64RRSpec::ControllerExtension::Raw:
        extension = CoreControllerExtension::Raw;
        break;
    default:
        RT_ASSERT(false, L"Unknown controller extension");
        break;
    }

    return CoreController{
        .Present = controller.present ? 1 : 0,
        .RawData = controller.raw ? 1 : 0,
        .Plugin = extension,
    };
}

static size_t get_config_path(char *data, size_t size)
{
    static const std::u8string config_path = std::filesystem::absolute(IOUtils::config_path()).u8string();

    if (data == nullptr) return config_path.size() + 1;
    if (size < config_path.size() + 1) return 0;

    memcpy(data, config_path.c_str(), config_path.size() + 1);
    return size + 1;
}

std::pair<std::wstring, std::unique_ptr<Plugin>> M64RRPlugin::create(HMODULE module, std::filesystem::path path)
{
    const auto get_metadata = (M64RRSpec::PtrGetMetadata)GetProcAddress(module, "M64RRGetMetadata");

    if (!get_metadata)
    {
        return std::make_pair(L"M64RRGetMetadata missing", nullptr);
    }

    M64RRSpec::PluginMetadata metadata{};
    get_metadata(&metadata);

    const size_t target_version_len = strnlen(metadata.target_version, std::size(metadata.target_version));
    if (target_version_len > 0)
    {
        // Plugin is tied to one version of mupen
        const auto current_version = IOUtils::to_utf8_string(CURRENT_VERSION);
        const std::string target_version(metadata.target_version, target_version_len);
        if (current_version != target_version)
        {
            return std::make_pair(L"Incompatible with this version of Mupen64", nullptr);
        }
    }

    const size_t plugin_name_len = strlen(metadata.name);
    while (plugin_name_len > 0 && metadata.name[plugin_name_len - 1] == ' ')
    {
        metadata.name[plugin_name_len - 1] = '\0';
    }

    auto plugin = std::make_unique<M64RRPlugin>();

    plugin->m_path = path;
    plugin->m_name = metadata.name;
    switch (metadata.type)
    {
    case M64RRSpec::PluginType::Video:
        plugin->m_type = Plugin::Type::Video;
        break;
    case M64RRSpec::PluginType::Audio:
        plugin->m_type = Plugin::Type::Audio;
        break;
    case M64RRSpec::PluginType::Input:
        plugin->m_type = Plugin::Type::Input;
        break;
    case M64RRSpec::PluginType::RSP:
        plugin->m_type = Plugin::Type::RSP;
        break;
    }
    plugin->m_module = module;
    plugin->m_meta = metadata;

    g_view_logger->info("[Plugin] Created plugin {}", plugin->m_name);
    return std::make_pair(L"", std::move(plugin));
}

void M64RRPlugin::config(HWND hwnd)
{
    const bool prev_initiated = m_initialized;
    initiate(g_plugin_funcs);
    const bool newly_initiated = m_initialized && !prev_initiated;

    const auto show_config = (M64RRSpec::PtrShowConfig)GetProcAddress(m_module, "M64RRRShowConfig");

    if (show_config)
        show_config(hwnd);
    else
    {
        DialogService::show_dialog(
            std::format(L"'{}' has no configuration.", IOUtils::to_wide_string(this->name())).c_str(), L"Plugin",
            fsvc_error, hwnd);
    }

    if (newly_initiated)
    {
        auto event_fn = (M64RRSpec::PtrProcessEvent)GetProcAddress(m_module, "M64RRProcessEvent");
        if (!event_fn) event_fn = [](auto) {};

        event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown});
    }
}

void M64RRPlugin::test(HWND hwnd)
{
}

void M64RRPlugin::about(HWND hwnd)
{
    MessageBox(hwnd, IOUtils::to_wide_string(m_meta.description).c_str(), L"About", MB_ICONINFORMATION | MB_OK);
}

void M64RRPlugin::initiate(ZESpecFuncs &funcs)
{
    auto event_fn = (M64RRSpec::PtrProcessEvent)GetProcAddress(m_module, "M64RRProcessEvent");
    if (!event_fn) event_fn = [](auto) {};

    M64RRSpec::PluginInit *init;
    switch (m_type)
    {

    case Plugin::Type::Video:
        init = &funcs.video_init;
        break;
    case Plugin::Type::Audio:
        init = &funcs.audio_init;
        break;
    case Plugin::Type::Input:
        init = &funcs.input_init;
        break;
    case Plugin::Type::RSP:
        init = &funcs.rsp_init;
        break;
    }
    init->platform = M64RRSpec::Platform::Windows;
    init->main_window = M64RRSpec::WindowHandle(g_main_ctx.hwnd);
    init->rom = g_main_ctx.core_ctx->rom;
    init->rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    init->dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    init->imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;

    init->rdram_register = g_main_ctx.core_ctx->rdram_register;
    init->mi_register = g_main_ctx.core_ctx->MI_register;
    init->pi_register = g_main_ctx.core_ctx->pi_register;
    init->sp_register = g_main_ctx.core_ctx->sp_register;
    init->rsp_register = g_main_ctx.core_ctx->rsp_register;
    init->si_register = g_main_ctx.core_ctx->si_register;
    init->vi_register = g_main_ctx.core_ctx->vi_register;
    init->ri_register = g_main_ctx.core_ctx->ri_register;
    init->ai_register = g_main_ctx.core_ctx->ai_register;
    init->dpc_register = g_main_ctx.core_ctx->dpc_register;
    init->dps_register = g_main_ctx.core_ctx->dps_register;

    init->process_dlist = funcs.video_process_dlist;

    std::array<M64RRSpec::Controller, 4> tmp_controllers{};
    init->controllers = tmp_controllers.data();

    init->get_effective_speed_mode = [](void) { return g_main_ctx.core_ctx->vr_get_effective_speed_mode(); };
    init->frame_skipped = [](void) { return g_main_ctx.core_ctx->vr_get_frame_skipped(); };
    init->config_path = get_config_path;
    init->rcp_counter = g_main_ctx.core_ctx->rcp_counter;
    init->request_size = Main::request_size;

    switch (m_type)
    {
    case Plugin::Type::Video:
        init->log_trace = [](const wchar_t *str) { g_video_logger->trace(str); };
        init->log_info = [](const wchar_t *str) { g_video_logger->info(str); };
        init->log_warn = [](const wchar_t *str) { g_video_logger->warn(str); };
        init->log_error = [](const wchar_t *str) { g_video_logger->error(str); };
        break;
    case Plugin::Type::Audio:
        init->log_trace = [](const wchar_t *str) { g_audio_logger->trace(str); };
        init->log_info = [](const wchar_t *str) { g_audio_logger->info(str); };
        init->log_warn = [](const wchar_t *str) { g_audio_logger->warn(str); };
        init->log_error = [](const wchar_t *str) { g_audio_logger->error(str); };
        break;
    case Plugin::Type::Input:
        init->log_trace = [](const wchar_t *str) { g_input_logger->trace(str); };
        init->log_info = [](const wchar_t *str) { g_input_logger->info(str); };
        init->log_warn = [](const wchar_t *str) { g_input_logger->warn(str); };
        init->log_error = [](const wchar_t *str) { g_input_logger->error(str); };
        break;
    case Plugin::Type::RSP:
        init->log_trace = [](const wchar_t *str) { g_rsp_logger->trace(str); };
        init->log_info = [](const wchar_t *str) { g_rsp_logger->info(str); };
        init->log_warn = [](const wchar_t *str) { g_rsp_logger->warn(str); };
        init->log_error = [](const wchar_t *str) { g_rsp_logger->error(str); };
        break;
    }

    event_fn(M64RRSpec::Event{.initiate = {
                                  .type = M64RRSpec::Event::Type::Initiate,
                                  .init = init,
                              }});

    switch (m_type)
    {
    case Plugin::Type::Video: {
        g_view_logger->trace("Initiating video plugin (MupenRR)...");

        LOOKUP_MUPENRR_FN(s_mupenrr_video_event_fn, M64RRSpec::PtrProcessEvent, "M64RRProcessEvent");
        LOOKUP_MUPENRR_FN(s_mupenrr_process_dlist_fn, M64RRSpec::PtrProcessDList, "M64RRProcessDList");
        LOOKUP_MUPENRR_FN(s_mupenrr_read_video_fn, M64RRSpec::PtrReadVideo, "M64RRReadVideo");

        funcs.video_rom_open = []() {
            if (s_mupenrr_video_event_fn)
                s_mupenrr_video_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomOpened});
        };
        funcs.video_rom_closed = []() {
            if (s_mupenrr_video_event_fn)
                s_mupenrr_video_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomClosed});
        };
        funcs.video_close_dll = []() {
            if (s_mupenrr_video_event_fn)
                s_mupenrr_video_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown});
        };
        funcs.video_process_dlist = []() {
            if (s_mupenrr_process_dlist_fn) s_mupenrr_process_dlist_fn();
        };
        funcs.video_process_rdp_list = [](const auto &...) {};
        funcs.video_show_cfb = [](const auto &...) {};
        funcs.video_vi_status_changed = [](const auto &...) {};
        funcs.video_vi_width_changed = [](const auto &...) {};
        if (s_mupenrr_read_video_fn)
        {
            funcs.video_get_video_size = [](int32_t *width, int32_t *height) {
                s_mupenrr_read_video_fn(nullptr, width, height);
            };
            funcs.video_read_video = [](void **buffer) { s_mupenrr_read_video_fn(*buffer, nullptr, nullptr); };
        }
        funcs.video_change_window = [](const auto &...) {};
        funcs.video_update_screen = [](const auto &...) {};
        funcs.video_move_screen = [](int32_t, int32_t) {};
        funcs.video_capture_screen = [](char *) {};
        funcs.video_read_screen = nullptr;
        funcs.video_fb_read = [](uint32_t) {};
        funcs.video_fb_write = [](uint32_t, uint32_t) {};
        funcs.video_fb_get_frame_buffer_info = [](ZESpec::FBInfo *) {};
        funcs.video_dll_crt_free = PluginUtil::get_free_function_in_module(m_module);

        break;
    }
    case Plugin::Type::Audio: {
        g_view_logger->trace("Initiating audio plugin (MupenRR)...");

        LOOKUP_MUPENRR_FN(s_mupenrr_audio_event_fn, M64RRSpec::PtrProcessEvent, "M64RRProcessEvent");
        LOOKUP_MUPENRR_FN(s_mupenrr_ai_dacrate_changed_fn, M64RRSpec::PtrAIDacrateChanged, "M64RRAIDacrateChanged");
        LOOKUP_MUPENRR_FN(s_mupenrr_ai_len_changed_fn, M64RRSpec::PtrAILenChanged, "M64RRAILenChanged");

        funcs.audio_rom_open = []() {
            if (s_mupenrr_audio_event_fn)
                s_mupenrr_audio_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomOpened});
        };
        funcs.audio_rom_closed = []() {
            if (s_mupenrr_audio_event_fn)
                s_mupenrr_audio_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomClosed});
        };
        funcs.audio_close_dll_audio = []() {
            if (s_mupenrr_audio_event_fn)
                s_mupenrr_audio_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown});
        };
        funcs.audio_ai_dacrate_changed = [](CoreSystemType system_type) {
            if (s_mupenrr_ai_dacrate_changed_fn) s_mupenrr_ai_dacrate_changed_fn(system_type);
        };
        funcs.audio_ai_len_changed = []() {
            if (s_mupenrr_ai_len_changed_fn) s_mupenrr_ai_len_changed_fn();
        };
        funcs.audio_ai_read_length = []() { return 0u; };
        funcs.audio_process_alist = [](const auto &...) {};
        funcs.audio_ai_update = nullptr;

        break;
    }
    case Plugin::Type::Input: {
        g_view_logger->trace("Initiating input plugin (MupenRR)...");

        LOOKUP_MUPENRR_FN(s_mupenrr_input_event_fn, M64RRSpec::PtrProcessEvent, "M64RRProcessEvent");
        LOOKUP_MUPENRR_FN(s_mupenrr_get_keys_fn, M64RRSpec::PtrGetKeys, "M64RRGetKeys");
        LOOKUP_MUPENRR_FN(s_mupenrr_set_keys_fn, M64RRSpec::PtrSetKeys, "M64RRSetKeys");
        LOOKUP_MUPENRR_FN(s_mupenrr_read_controller_fn, M64RRSpec::PtrReadController, "M64RRReadController");

        funcs.input_rom_open = []() {
            if (s_mupenrr_input_event_fn)
                s_mupenrr_input_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomOpened});
        };
        funcs.input_rom_closed = []() {
            if (s_mupenrr_input_event_fn)
                s_mupenrr_input_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomClosed});
        };
        funcs.input_close_dll = []() {
            if (s_mupenrr_input_event_fn)
                s_mupenrr_input_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown});
        };
        funcs.input_controller_command = [](int32_t, uint8_t *) {};
        funcs.input_get_keys = [](int32_t controller, ZESpec::Buttons *keys) {
            M64RRSpec::Buttons buttons{keys->value};
            if (s_mupenrr_get_keys_fn) s_mupenrr_get_keys_fn(controller, &buttons);
            keys->value = buttons.value;
        };
        funcs.input_set_keys = [](int32_t controller, ZESpec::Buttons keys) {
            M64RRSpec::Buttons buttons{keys.value};
            if (s_mupenrr_set_keys_fn) s_mupenrr_set_keys_fn(controller, &buttons);
        };
        funcs.input_read_controller = [](int32_t controller, unsigned char *command) {
            if (s_mupenrr_read_controller_fn) s_mupenrr_read_controller_fn(controller, command);
        };
        funcs.input_key_down = [](uint32_t, int32_t) {};
        funcs.input_key_up = [](uint32_t, int32_t) {};

        for (size_t i = 0; i < std::size(tmp_controllers); ++i)
        {
            g_main_ctx.core.controls[i] = controller_to_core_controller(tmp_controllers[i]);
        }
        break;
    }
    case Plugin::Type::RSP: {
        g_view_logger->trace("Initiating RSP plugin (MupenRR)...");

        LOOKUP_MUPENRR_FN(s_mupenrr_rsp_event_fn, M64RRSpec::PtrProcessEvent, "M64RRProcessEvent");
        LOOKUP_MUPENRR_FN(s_mupenrr_do_rsp_cycles_fn, M64RRSpec::PtrDoRSPCycles, "M64RRDoRSPCycles");

        // FIXME: add rsp_rom_opened
        funcs.rsp_rom_closed = []() {
            if (s_mupenrr_rsp_event_fn)
                s_mupenrr_rsp_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomClosed});
        };
        funcs.rsp_close_dll = []() {
            if (s_mupenrr_rsp_event_fn)
                s_mupenrr_rsp_event_fn(M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown});
        };
        funcs.rsp_do_rsp_cycles = [](uint32_t cycles) {
            if (s_mupenrr_do_rsp_cycles_fn)
            {
                s_mupenrr_do_rsp_cycles_fn((uint8_t)cycles);
                return 0u;
            }
            return cycles;
        };

        break;
    }
    default:
        RT_ASSERT(false, L"Unsupported plugin type");
        break;
    }

    m_initialized = true;
}

#undef LOOKUP_MUPENRR_FN
