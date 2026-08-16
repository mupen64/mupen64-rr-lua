/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// ReSharper disable CppCStyleCast

#include "Common.hpp"
#include <Common.Views/Config.hpp>
#include <Common.Views/IDialogService.hpp>
#include <plugin/Plugin.hpp>
#include <plugin/M64RRPlugin.hpp>
#include <plugin/ZEPlugin.hpp>
#include <components/ConfigDialog.hpp>
#include <components/Statusbar.hpp>
#include <components/MGECompositor.hpp>
#include <ThreadPool.hpp>
#include <Common.Views/Messages.hpp>

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

ZESpecFuncs g_plugin_funcs{};

static void audio_thread_proc(std::stop_token st)
{
    while (!st.stop_requested())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        g_plugin_funcs.audio_ai_update(0);
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
    if (!g_plugin_funcs.audio_ai_update)
    {
        g_view_logger->info("Skipping audio thread creation");
        return;
    }

    g_view_logger->info("Starting audio thread...");
    if (s_audio_thread.joinable()) stop_audio_thread();
    s_audio_thread = std::jthread(audio_thread_proc);
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
            auto importDllHandle = GetModuleHandle(importDllName);
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
    g_plugin_funcs.video_get_video_size(width, height);
}

void PluginUtil::read_video(void *buffer)
{
    g_plugin_funcs.video_read_video(&buffer);
}

void PluginUtil::update_screen()
{
    if (PluginUtil::mge_available())
        g_main_ctx.dispatcher->invoke([] { MGECompositor::update_screen(); });
    else
        g_plugin_funcs.video_update_screen();
}

void PluginUtil::key_down(uint32_t wParam, int32_t lParam)
{
    if (g_plugin_funcs.input_key_down && g_main_ctx.core_ctx->vr_get_launched())
        g_plugin_funcs.input_key_down(wParam, lParam);
}

void PluginUtil::key_up(uint32_t wParam, int32_t lParam)
{
    if (g_plugin_funcs.input_key_up && g_main_ctx.core_ctx->vr_get_launched())
        g_plugin_funcs.input_key_up(wParam, lParam);
}

void PluginUtil::move_screen(uint32_t wParam, int32_t lParam)
{
    if (g_main_ctx.core_ctx->vr_get_launched()) g_plugin_funcs.video_move_screen((int)wParam, lParam);
}

std::pair<std::string, std::unique_ptr<Plugin>> Plugin::create(std::filesystem::path path)
{
    Main::init_sdl();

    const auto module = LoadLibrary(path.string().c_str());
    uint64_t last_error = GetLastError();

    if (module == nullptr)
    {
        return std::make_pair(std::format("LoadLibrary (code {})", last_error), nullptr);
    }

    auto result1 = ZEPlugin::create(module, path);
    auto result2 = M64RRPlugin::create(module, path);

    if (result1.first.empty()) return result1;
    if (result2.first.empty()) return result2;

    FreeLibrary(module);
    return std::make_pair("Incompatible with this version of Mupen64", nullptr);
}

Plugin::~Plugin()
{
    if (!FreeLibrary(m_module))
    {
        DialogService::show_dialog(std::format("Failed to free library {}.", (void *)m_module), "Core", fsvc_error);
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
    Messenger::subscribe<Messenger::Message::EmuStopping>([] { stop_audio_thread(); });
}

t_plugin_discovery_result PluginUtil::discover_plugins(const std::filesystem::path &directory)
{
    std::vector<std::unique_ptr<Plugin>> plugins;
    std::vector<std::pair<std::filesystem::path, std::string>> results;

    // this will fail to match files with the extension not lowercased, but I don't think this is a big deal.
    auto dll_files = std::filesystem::directory_iterator(directory) |
                     std::views::filter([](const std::filesystem::directory_entry &entry) {
                         return entry.is_regular_file() && entry.path().extension().compare(".dll") == 0;
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

bool PluginUtil::mge_available()
{
    return g_plugin_funcs.video_read_video && g_plugin_funcs.video_get_video_size;
}

void PluginUtil::start_plugins()
{
    g_main_ctx.core.video_process_dlist = g_plugin_funcs.video_process_dlist;
    g_main_ctx.core.video_process_rdp_list = g_plugin_funcs.video_process_rdp_list;
    g_main_ctx.core.video_show_cfb = g_plugin_funcs.video_show_cfb;
    g_main_ctx.core.video_vi_status_changed = g_plugin_funcs.video_vi_status_changed;
    g_main_ctx.core.video_vi_width_changed = g_plugin_funcs.video_vi_width_changed;
    g_main_ctx.core.video_get_video_size = g_plugin_funcs.video_get_video_size;
    g_main_ctx.core.video_fb_read = g_plugin_funcs.video_fb_read;
    g_main_ctx.core.video_fb_write = g_plugin_funcs.video_fb_write;
    g_main_ctx.core.video_fb_get_frame_buffer_info = [](CoreFBInfo info[6]) {
        ZESpec::FBInfo z_fb[6]{};
        g_plugin_funcs.video_fb_get_frame_buffer_info(z_fb);

        for (size_t i = 0; i < 6; i++)
        {
            info[i].addr = z_fb[i].addr;
            info[i].size = z_fb[i].size;
            info[i].width = z_fb[i].width;
            info[i].height = z_fb[i].height;
        }
    };

    g_main_ctx.core.audio_ai_dacrate_changed = g_plugin_funcs.audio_ai_dacrate_changed;
    g_main_ctx.core.audio_ai_len_changed = g_plugin_funcs.audio_ai_len_changed;
    g_main_ctx.core.audio_ai_read_length = g_plugin_funcs.audio_ai_read_length;
    g_main_ctx.core.audio_process_alist = g_plugin_funcs.audio_process_alist;

    g_main_ctx.core.input_controller_command = g_plugin_funcs.input_controller_command;
    g_main_ctx.core.input_get_keys = [](int32_t controller, CoreButtons *keys) {
        ZESpec::Buttons z_keys{};
        g_plugin_funcs.input_get_keys(controller, &z_keys);
        keys->value = z_keys.value;
    };
    g_main_ctx.core.input_set_keys = [](int32_t controller, CoreButtons keys) {
        ZESpec::Buttons z_keys{keys.value};
        g_plugin_funcs.input_set_keys(controller, z_keys);
    };
    g_main_ctx.core.input_read_controller = g_plugin_funcs.input_read_controller;

    g_main_ctx.core.rsp_do_rsp_cycles = g_plugin_funcs.rsp_do_rsp_cycles;

    g_plugin_funcs.video_rom_open();
    g_plugin_funcs.input_rom_open();
    g_plugin_funcs.audio_rom_open();

    start_audio_thread();
}

void PluginUtil::stop_plugins()
{
    g_plugin_funcs.video_rom_closed();
    g_plugin_funcs.audio_rom_closed();
    g_plugin_funcs.input_rom_closed();
    g_plugin_funcs.rsp_rom_closed();
    g_plugin_funcs.video_close_dll();
    g_plugin_funcs.audio_close_dll_audio();
    g_plugin_funcs.input_close_dll();
    g_plugin_funcs.rsp_close_dll();
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

        g_view_logger->trace("Loading video plugin: {}", g_config.selected_video_plugin);
        g_view_logger->trace("Loading audio plugin: {}", g_config.selected_audio_plugin);
        g_view_logger->trace("Loading input plugin: {}", g_config.selected_input_plugin);
        g_view_logger->trace("Loading RSP plugin: {}", g_config.selected_rsp_plugin);

        Main::init_sdl();

        auto video_pl = Plugin::create(g_config.selected_video_plugin);
        auto audio_pl = Plugin::create(g_config.selected_audio_plugin);
        auto input_pl = Plugin::create(g_config.selected_input_plugin);
        auto rsp_pl = Plugin::create(g_config.selected_rsp_plugin);

        if (!video_pl.first.empty())
        {
            g_view_logger->error("Failed to load video plugin: {}", video_pl.first);
        }
        if (!audio_pl.first.empty())
        {
            g_view_logger->error("Failed to load audio plugin: {}", audio_pl.first);
        }
        if (!input_pl.first.empty())
        {
            g_view_logger->error("Failed to load input plugin: {}", input_pl.first);
        }
        if (!rsp_pl.first.empty())
        {
            g_view_logger->error("Failed to load rsp plugin: {}", rsp_pl.first);
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

    // RSP plugin depends on some exported functions :P
    std::latch latch(3);
    ThreadPool::submit_task([&] {
        video_plugin->initiate(g_plugin_funcs);
        latch.count_down();
    });
    ThreadPool::submit_task([&] {
        audio_plugin->initiate(g_plugin_funcs);
        latch.count_down();
    });
    ThreadPool::submit_task([&] {
        input_plugin->initiate(g_plugin_funcs);
        latch.count_down();
    });
    latch.wait();

    rsp_plugin->initiate(g_plugin_funcs);
}

void PluginUtil::get_plugin_names(char *video, char *audio, char *input, char *rsp)
{
    const auto copy = [&](const std::shared_ptr<Plugin> &plugin, char *type) {
        RT_ASSERT(plugin.get(), "Plugin not loaded");
        const auto result = strncpy_s(type, 64 - 1, plugin->name().c_str(), plugin->name().size());
        RT_ASSERT(!result, "Plugin name copy failed");
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

    g_plugin_funcs.video_capture_screen(dir_str.data());
}
