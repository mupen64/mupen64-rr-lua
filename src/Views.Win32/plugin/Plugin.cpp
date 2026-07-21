/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// ReSharper disable CppCStyleCast

#include "stdafx.h"
#include <Config.hpp>
#include <DialogService.hpp>
#include <plugin/Plugin.hpp>
#include <plugin/M64RRPlugin.hpp>
#include <plugin/ZEPlugin.hpp>
#include <components/ConfigDialog.hpp>
#include <components/Statusbar.hpp>
#include <components/MGECompositor.hpp>
#include <ThreadPool.hpp>
#include <Messenger.hpp>

ZESpec::VideoPluginInfo dummy_video_info{};
ZESpec::AudioPluginInfo dummy_audio_info{};
ZESpec::InputPluginInfo dummy_control_info{};
ZESpec::RSPPluginInfo dummy_rsp_info{};
ZESpec::Controller dummy_controllers[4]{};
uint8_t dummy_header[0x40]{};
uint32_t dummy_dw{};

ZESpec::VideoPluginInfo gfx_info{};
ZESpec::AudioPluginInfo audio_info{};
ZESpec::InputPluginInfo control_info{};
ZESpec::RSPPluginInfo rsp_info{};

ZESpec::DLLABOUT dll_about{};
ZESpec::DLLCONFIG dll_config{};
ZESpec::DLLTEST dll_test{};

static std::shared_ptr<Plugin> video_plugin;
static std::shared_ptr<Plugin> audio_plugin;
static std::shared_ptr<Plugin> input_plugin;
static std::shared_ptr<Plugin> rsp_plugin;

static std::jthread s_audio_thread;

ZESpecFuncs s_funcs{};

static void audio_thread_proc(std::stop_token st)
{
    while (!st.stop_requested())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        s_funcs.audio_ai_update(0);
    }
}

static void stop_audio_thread()
{
    if (!s_audio_thread.joinable()) return;
    s_audio_thread.request_stop();
    s_audio_thread = {};
}

static void start_audio_thread()
{
    // We can forego thread creation for plugins with no AiUpdate implementation because they do audio heartbeat
    // themselves
    if (!s_funcs.audio_ai_update)
    {
        g_view_logger->info("Skipping audio thread creation");
        return;
    }

    g_view_logger->info("Starting audio thread...");
    if (s_audio_thread.joinable()) stop_audio_thread();
    s_audio_thread = std::jthread(audio_thread_proc);
}

#define GEN_EXTENDED_FUNCS(logger)                                                                                     \
    ZESpec::ExtendedFuncs                                                                                              \
    {                                                                                                                  \
        .log_trace = [](const wchar_t *str) { logger->trace(str); },                                                   \
        .log_info = [](const wchar_t *str) { logger->info(str); },                                                     \
        .log_warn = [](const wchar_t *str) { logger->warn(str); },                                                     \
        .log_error = [](const wchar_t *str) { logger->error(str); },                                                   \
        .get_effective_speed_mode = [](void) { return g_main_ctx.core_ctx->vr_get_effective_speed_mode(); },           \
        .frame_skipped = [](void) { return g_main_ctx.core_ctx->vr_get_frame_skipped(); },                             \
        .config_path = ext_fn_config_path, .rcp_counter = g_main_ctx.core_ctx->rcp_counter                             \
    }

ZESpec::DLLCRTFREE PluginUtil::get_free_function_in_module(HMODULE module)
{
    auto dll_crt_free = (ZESpec::DLLCRTFREE)GetProcAddress(module, "DllCrtFree");
    if (dll_crt_free) return dll_crt_free;

    ULONG size;
    auto import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToDataEx(
        module, true, IMAGE_DIRECTORY_ENTRY_IMPORT, &size, nullptr);
    if (import_descriptor != nullptr)
    {
        while (import_descriptor->Characteristics && import_descriptor->Name)
        {
            auto importDllName = (LPCSTR)((PBYTE)module + import_descriptor->Name);
            auto importDllHandle = GetModuleHandleA(importDllName);
            if (importDllHandle != nullptr)
            {
                dll_crt_free = (ZESpec::DLLCRTFREE)GetProcAddress(importDllHandle, "free");
                if (dll_crt_free != nullptr) return dll_crt_free;
            }

            import_descriptor++;
        }
    }

    return free;
}

void PluginUtil::get_video_size(int32_t *width, int32_t *height)
{
    s_funcs.video_get_video_size(width, height);
}

void PluginUtil::read_video(void *buffer)
{
    s_funcs.video_read_video(&buffer);
}

void PluginUtil::update_screen()
{
    if (PluginUtil::mge_available())
        MGECompositor::update_screen();
    else
        s_funcs.video_update_screen();
}

void PluginUtil::key_down(uint32_t wParam, int32_t lParam)
{
    if (s_funcs.input_key_down && g_main_ctx.core_ctx->vr_get_launched()) s_funcs.input_key_down(wParam, lParam);
}

void PluginUtil::key_up(uint32_t wParam, int32_t lParam)
{
    if (s_funcs.input_key_up && g_main_ctx.core_ctx->vr_get_launched()) s_funcs.input_key_up(wParam, lParam);
}

void PluginUtil::move_screen(uint32_t wParam, int32_t lParam)
{
    if (g_main_ctx.core_ctx->vr_get_launched()) s_funcs.video_move_screen((int)wParam, lParam);
}

std::pair<std::wstring, std::unique_ptr<Plugin>> Plugin::create(std::filesystem::path path)
{
    Main::init_sdl();

    const auto module = LoadLibrary(path.wstring().c_str());
    uint64_t last_error = GetLastError();

    if (module == nullptr)
    {
        return std::make_pair(std::format(L"LoadLibrary (code {})", last_error), nullptr);
    }

    auto result1 = ZEPlugin::create(module, path);
    auto result2 = M64RRPlugin::create(module, path);

    if (result1.first.empty()) return result1;
    if (result2.first.empty()) return result2;

    FreeLibrary(module);
    return std::make_pair(L"Incompatible with this version of Mupen64", nullptr);
}

Plugin::~Plugin()
{
    if (!FreeLibrary(m_module))
    {
        DialogService::show_dialog(std::format(L"Failed to free library {:#06x}.", (unsigned long)m_module).c_str(),
                                   L"Core", fsvc_error);
    }
}

void Plugin::config(HWND hwnd)
{
}

void Plugin::test(HWND hwnd)
{
}

void Plugin::about(HWND hwnd)
{
}

void Plugin::initiate(ZESpecFuncs &funcs)
{
}

void Plugin::initiate_dummy()
{
}

void Plugin::deinitiate_dummy()
{
}

void PluginUtil::init()
{
    Messenger::subscribe(Messenger::Message::EmuStopping, [](const auto &...) { stop_audio_thread(); });
}

t_plugin_discovery_result PluginUtil::discover_plugins(const std::filesystem::path &directory)
{
    std::vector<std::unique_ptr<Plugin>> plugins;
    std::vector<std::pair<std::filesystem::path, std::wstring>> results;

    // this will fail to match files with the extension not lowercased, but I don't think this is a big deal.
    auto dll_files = std::filesystem::directory_iterator(directory) |
                     std::views::filter([](const std::filesystem::directory_entry &entry) {
                         return entry.is_regular_file() && entry.path().extension().compare(L".dll") == 0;
                     }) |
                     std::views::transform([](const std::filesystem::directory_entry &entry) { return entry.path(); });

    for (const auto &file : dll_files)
    {
        auto [result, plugin] = Plugin::create(file);

        results.emplace_back(file, result);
        if (!result.empty()) continue;

        plugins.emplace_back(std::move(plugin));
    }

    // Special case: plugins are present but not in the plugin directory
    for (const auto &file : {g_config.selected_video_plugin, g_config.selected_audio_plugin,
                             g_config.selected_input_plugin, g_config.selected_rsp_plugin})
    {
        auto it = std::find_if(results.begin(), results.end(), [&](const auto &pair) {
            std::error_code ec;
            return std::filesystem::equivalent(pair.first, file, ec);
        });
        if (it != results.end()) continue;

        auto [result, plugin] = Plugin::create(file);

        results.emplace_back(file, result);
        if (!result.empty()) continue;

        plugins.emplace_back(std::move(plugin));
    }

    return t_plugin_discovery_result{
        .plugins = std::move(plugins),
        .results = results,
    };
}

void PluginUtil::init_dummy_and_extended_funcs()
{
    dummy_video_info.byteswapped = 1;
    dummy_video_info.rom = (uint8_t *)dummy_header;
    dummy_video_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    dummy_video_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    dummy_video_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    dummy_video_info.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    dummy_video_info.dpc_start_reg = &g_main_ctx.core_ctx->dpc_register->dpc_start;
    dummy_video_info.dpc_end_reg = &g_main_ctx.core_ctx->dpc_register->dpc_end;
    dummy_video_info.dpc_current_reg = &g_main_ctx.core_ctx->dpc_register->dpc_current;
    dummy_video_info.dpc_status_reg = &g_main_ctx.core_ctx->dpc_register->dpc_status;
    dummy_video_info.dpc_clock_reg = &g_main_ctx.core_ctx->dpc_register->dpc_clock;
    dummy_video_info.dpc_bufbusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_bufbusy;
    dummy_video_info.dpc_pipebusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_pipebusy;
    dummy_video_info.dpc_tmem_reg = &g_main_ctx.core_ctx->dpc_register->dpc_tmem;
    dummy_video_info.vi_status_reg = &g_main_ctx.core_ctx->vi_register->vi_status;
    dummy_video_info.vi_origin_reg = &g_main_ctx.core_ctx->vi_register->vi_origin;
    dummy_video_info.vi_width_reg = &g_main_ctx.core_ctx->vi_register->vi_width;
    dummy_video_info.vi_intr_reg = &g_main_ctx.core_ctx->vi_register->vi_v_intr;
    dummy_video_info.vi_v_current_line_reg = &g_main_ctx.core_ctx->vi_register->vi_current;
    dummy_video_info.vi_timing_reg = &g_main_ctx.core_ctx->vi_register->vi_burst;
    dummy_video_info.vi_v_sync_reg = &g_main_ctx.core_ctx->vi_register->vi_v_sync;
    dummy_video_info.vi_h_sync_reg = &g_main_ctx.core_ctx->vi_register->vi_h_sync;
    dummy_video_info.vi_leap_reg = &g_main_ctx.core_ctx->vi_register->vi_leap;
    dummy_video_info.vi_h_start_reg = &g_main_ctx.core_ctx->vi_register->vi_h_start;
    dummy_video_info.vi_v_start_reg = &g_main_ctx.core_ctx->vi_register->vi_v_start;
    dummy_video_info.vi_v_burst_reg = &g_main_ctx.core_ctx->vi_register->vi_v_burst;
    dummy_video_info.vi_x_scale_reg = &g_main_ctx.core_ctx->vi_register->vi_x_scale;
    dummy_video_info.vi_y_scale_reg = &g_main_ctx.core_ctx->vi_register->vi_y_scale;
    dummy_video_info.check_interrupts = []() {};

    dummy_audio_info.main_hwnd = g_main_ctx.hwnd;
    dummy_audio_info.hinst = g_main_ctx.hinst;
    dummy_audio_info.byteswapped = 1;
    dummy_audio_info.rom = (uint8_t *)dummy_header;
    dummy_audio_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    dummy_audio_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    dummy_audio_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    dummy_audio_info.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    dummy_audio_info.ai_dram_addr_reg = &g_main_ctx.core_ctx->ai_register->ai_dram_addr;
    dummy_audio_info.ai_len_reg = &g_main_ctx.core_ctx->ai_register->ai_len;
    dummy_audio_info.ai_control_reg = &g_main_ctx.core_ctx->ai_register->ai_control;
    dummy_audio_info.ai_status_reg = &g_main_ctx.core_ctx->ai_register->ai_status;
    dummy_audio_info.ai_dacrate_reg = &g_main_ctx.core_ctx->ai_register->ai_dacrate;
    dummy_audio_info.ai_bitrate_reg = &g_main_ctx.core_ctx->ai_register->ai_bitrate;
    dummy_audio_info.check_interrupts = []() {};

    dummy_control_info.main_hwnd = g_main_ctx.hwnd;
    dummy_control_info.hinst = g_main_ctx.hinst;
    dummy_control_info.byteswapped = 1;
    dummy_control_info.header = (uint8_t *)dummy_header;
    dummy_control_info.controllers = dummy_controllers;

    dummy_rsp_info.byteswapped = 1;
    dummy_rsp_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    dummy_rsp_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    dummy_rsp_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    dummy_rsp_info.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    dummy_rsp_info.sp_mem_addr_reg = &g_main_ctx.core_ctx->sp_register->sp_mem_addr_reg;
    dummy_rsp_info.sp_dram_addr_reg = &g_main_ctx.core_ctx->sp_register->sp_dram_addr_reg;
    dummy_rsp_info.sp_rd_len_reg = &g_main_ctx.core_ctx->sp_register->sp_rd_len_reg;
    dummy_rsp_info.sp_wr_len_reg = &g_main_ctx.core_ctx->sp_register->sp_wr_len_reg;
    dummy_rsp_info.sp_status_reg = &g_main_ctx.core_ctx->sp_register->sp_status_reg;
    dummy_rsp_info.sp_dma_full_reg = &g_main_ctx.core_ctx->sp_register->sp_dma_full_reg;
    dummy_rsp_info.sp_dma_busy_reg = &g_main_ctx.core_ctx->sp_register->sp_dma_busy_reg;
    dummy_rsp_info.sp_pc_reg = &g_main_ctx.core_ctx->rsp_register->rsp_pc;
    dummy_rsp_info.sp_semaphore_reg = &g_main_ctx.core_ctx->sp_register->sp_semaphore_reg;
    dummy_rsp_info.dpc_start_reg = &g_main_ctx.core_ctx->dpc_register->dpc_start;
    dummy_rsp_info.dpc_end_reg = &g_main_ctx.core_ctx->dpc_register->dpc_end;
    dummy_rsp_info.dpc_current_reg = &g_main_ctx.core_ctx->dpc_register->dpc_current;
    dummy_rsp_info.dpc_status_reg = &g_main_ctx.core_ctx->dpc_register->dpc_status;
    dummy_rsp_info.dpc_clock_reg = &g_main_ctx.core_ctx->dpc_register->dpc_clock;
    dummy_rsp_info.dpc_bufbusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_bufbusy;
    dummy_rsp_info.dpc_pipebusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_pipebusy;
    dummy_rsp_info.dpc_tmem_reg = &g_main_ctx.core_ctx->dpc_register->dpc_tmem;
    dummy_rsp_info.check_interrupts = []() {};
    dummy_rsp_info.process_dlist_list = s_funcs.video_process_dlist;
    dummy_rsp_info.process_alist_list = s_funcs.audio_process_alist;
    dummy_rsp_info.process_rdp_list = s_funcs.video_process_rdp_list;
    dummy_rsp_info.show_cfb = s_funcs.video_show_cfb;

    s_funcs.video_extended_funcs = GEN_EXTENDED_FUNCS(g_video_logger);
    s_funcs.audio_extended_funcs = GEN_EXTENDED_FUNCS(g_audio_logger);
    s_funcs.input_extended_funcs = GEN_EXTENDED_FUNCS(g_input_logger);
    s_funcs.rsp_extended_funcs = GEN_EXTENDED_FUNCS(g_rsp_logger);
}

bool PluginUtil::mge_available()
{
    return s_funcs.video_read_video && s_funcs.video_get_video_size;
}

void PluginUtil::start_plugins()
{
    g_main_ctx.core.video_process_dlist = s_funcs.video_process_dlist;
    g_main_ctx.core.video_process_rdp_list = s_funcs.video_process_rdp_list;
    g_main_ctx.core.video_show_cfb = s_funcs.video_show_cfb;
    g_main_ctx.core.video_vi_status_changed = s_funcs.video_vi_status_changed;
    g_main_ctx.core.video_vi_width_changed = s_funcs.video_vi_width_changed;
    g_main_ctx.core.video_get_video_size = s_funcs.video_get_video_size;
    g_main_ctx.core.video_fb_read = s_funcs.video_fb_read;
    g_main_ctx.core.video_fb_write = s_funcs.video_fb_write;
    g_main_ctx.core.video_fb_get_frame_buffer_info = [](CoreFBInfo info[6]) {
        ZESpec::FBInfo z_fb[6]{};
        s_funcs.video_fb_get_frame_buffer_info(z_fb);

        for (size_t i = 0; i < 6; i++)
        {
            info[i].addr = z_fb[i].addr;
            info[i].size = z_fb[i].size;
            info[i].width = z_fb[i].width;
            info[i].height = z_fb[i].height;
        }
    };

    g_main_ctx.core.audio_ai_dacrate_changed = s_funcs.audio_ai_dacrate_changed;
    g_main_ctx.core.audio_ai_len_changed = s_funcs.audio_ai_len_changed;
    g_main_ctx.core.audio_ai_read_length = s_funcs.audio_ai_read_length;
    g_main_ctx.core.audio_process_alist = s_funcs.audio_process_alist;

    g_main_ctx.core.input_controller_command = s_funcs.input_controller_command;
    g_main_ctx.core.input_get_keys = [](int32_t controller, CoreButtons *keys) {
        ZESpec::Buttons z_keys{};
        s_funcs.input_get_keys(controller, &z_keys);
        keys->value = z_keys.value;
    };
    g_main_ctx.core.input_set_keys = [](int32_t controller, CoreButtons keys) {
        ZESpec::Buttons z_keys{keys.value};
        s_funcs.input_set_keys(controller, z_keys);
    };
    g_main_ctx.core.input_read_controller = s_funcs.input_read_controller;

    g_main_ctx.core.rsp_do_rsp_cycles = s_funcs.rsp_do_rsp_cycles;

    s_funcs.video_rom_open();
    s_funcs.input_rom_open();
    s_funcs.audio_rom_open();

    start_audio_thread();
}

void PluginUtil::stop_plugins()
{
    s_funcs.video_rom_closed();
    s_funcs.audio_rom_closed();
    s_funcs.input_rom_closed();
    s_funcs.rsp_rom_closed();
    s_funcs.video_close_dll();
    s_funcs.audio_close_dll_audio();
    s_funcs.input_close_dll();
    s_funcs.rsp_close_dll();
}
bool PluginUtil::load_plugins()
{
    if (video_plugin.get() && audio_plugin.get() && input_plugin.get() && rsp_plugin.get() &&
        video_plugin->path() == g_config.selected_video_plugin &&
        audio_plugin->path() == g_config.selected_audio_plugin &&
        input_plugin->path() == g_config.selected_input_plugin && rsp_plugin->path() == g_config.selected_rsp_plugin)
    {
        g_core_logger->info("[Core] Plugins unchanged, reusing...");
    }
    else
    {
        video_plugin.reset();
        audio_plugin.reset();
        input_plugin.reset();
        rsp_plugin.reset();

        g_view_logger->trace(L"Loading video plugin: {}", g_config.selected_video_plugin);
        g_view_logger->trace(L"Loading audio plugin: {}", g_config.selected_audio_plugin);
        g_view_logger->trace(L"Loading input plugin: {}", g_config.selected_input_plugin);
        g_view_logger->trace(L"Loading RSP plugin: {}", g_config.selected_rsp_plugin);

        Main::init_sdl();

        auto video_pl = Plugin::create(g_config.selected_video_plugin);
        auto audio_pl = Plugin::create(g_config.selected_audio_plugin);
        auto input_pl = Plugin::create(g_config.selected_input_plugin);
        auto rsp_pl = Plugin::create(g_config.selected_rsp_plugin);

        if (!video_pl.first.empty())
        {
            g_view_logger->error(L"Failed to load video plugin: {}", video_pl.first);
        }
        if (!audio_pl.first.empty())
        {
            g_view_logger->error(L"Failed to load audio plugin: {}", audio_pl.first);
        }
        if (!input_pl.first.empty())
        {
            g_view_logger->error(L"Failed to load input plugin: {}", input_pl.first);
        }
        if (!rsp_pl.first.empty())
        {
            g_view_logger->error(L"Failed to load rsp plugin: {}", rsp_pl.first);
        }

        if (video_pl.second == nullptr || audio_pl.second == nullptr || input_pl.second == nullptr ||
            rsp_pl.second == nullptr)
        {
            video_pl.second.reset();
            audio_pl.second.reset();
            input_pl.second.reset();
            rsp_pl.second.reset();
            return false;
        }

        video_plugin = std::move(video_pl.second);
        audio_plugin = std::move(audio_pl.second);
        input_plugin = std::move(input_pl.second);
        rsp_plugin = std::move(rsp_pl.second);
    }
    return true;
}

void PluginUtil::initiate_plugins()
{
    ScopeTimer timer("PluginUtil::initiate_plugins", g_view_logger.get());

    // Video plugin needs to go first because of process_dlist
    video_plugin->initiate(s_funcs);
    audio_plugin->initiate(s_funcs);
    input_plugin->initiate(s_funcs);
    rsp_plugin->initiate(s_funcs);

    // std::latch done(3);

    // ThreadPool::submit_task([&] {
    //     audio_plugin->initiate();
    //     done.count_down();
    // });

    // ThreadPool::submit_task([&] {
    //     input_plugin->initiate();
    //     done.count_down();
    // });

    // ThreadPool::submit_task([&] {
    //     rsp_plugin->initiate();
    //     done.count_down();
    // });

    // done.wait();
}

void PluginUtil::get_plugin_names(char *video, char *audio, char *input, char *rsp)
{
    const auto copy = [&](const std::shared_ptr<Plugin> &plugin, char *type) {
        RT_ASSERT(plugin.get(), L"Plugin not loaded");
        const auto result = strncpy_s(type, 64 - 1, plugin->name().c_str(), plugin->name().size());
        RT_ASSERT(!result, L"Plugin name copy failed");
    };

    copy(video_plugin, video);
    copy(audio_plugin, audio);
    copy(input_plugin, input);
    copy(rsp_plugin, rsp);
}

void PluginUtil::screenshot(const std::filesystem::path &path)
{
    const auto dir = std::filesystem::is_directory(path) ? path : path.parent_path();
    if (!std::filesystem::exists(dir)) std::filesystem::create_directories(dir);

    // Ensure trailing separator because some plugins are crap
    std::string dir_str = dir.string();
    if (!dir_str.empty() && dir_str.back() != std::filesystem::path::preferred_separator)
        dir_str += std::filesystem::path::preferred_separator;

    s_funcs.video_capture_screen(dir_str.data());
}
