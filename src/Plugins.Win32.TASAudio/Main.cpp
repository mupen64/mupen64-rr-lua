/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include "Main.hpp"
#include "Config.hpp"
#include "IOUtils.hpp"
#include "SDLBackend.hpp"
#include "m64rr/Types.hpp"
#include <VersionNameHelpers.hpp>
#include <m64rr/Plugin.hpp>

M64RRSpec::PluginInit *g_plugin = nullptr;
std::optional<SDLAudio::SDLBackend> g_backend{};

std::filesystem::path g_dll_path{}; // currently set in Main_Win32.cpp
static bool g_sdl_is_init = false;

#define CONFIG_FILE_NAME "TASAudio.json"

static uint32_t compute_sample_rate(CoreSystemType system_type, uint32_t dacrate)
{
    uint32_t vi_clock = 0;
    switch (system_type)
    {
    case CoreSystemType::NTSC:
        vi_clock = 48681812;
        break;
    case CoreSystemType::PAL:
        vi_clock = 49656530;
        break;
    default:
        // fallback to NTSC
        vi_clock = 48681812;
        break;
    }

    return vi_clock / (dacrate + 1);
}

static inline std::filesystem::path config_path()
{
    const auto size = g_plugin->config_path(nullptr, 0);
    std::string path(size - 1, '\0');
    g_plugin->config_path(path.data(), size);
    return std::filesystem::path(path) / CONFIG_FILE_NAME;
}

SDLAudio::Config read_config()
{
    SDLAudio::Config cfg;
    std::fstream fs(config_path(), std::ios_base::in | std::ios_base::out | std::ios_base::app | std::ios_base::ate);
    fs.exceptions(std::ios_base::badbit);

    // if the config file was missing or empty, write the default config
    if (fs.tellg() == 0)
    {
        cfg.write_to(fs);
        return cfg;
    }

    try
    {
        fs.seekg(0, std::ios_base::beg);
        cfg.read_from(fs);
    }
    catch (const std::invalid_argument &)
    {
        // if config is invalid, use defaults
        cfg = {};
    }
    return cfg;
}
void write_config(const SDLAudio::Config &config)
{
    std::ofstream fs(config_path());
    fs.exceptions(std::ios_base::badbit);
    config.write_to(fs);
}

EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)
{
    metadata->type = M64RRSpec::PluginType::Audio;

    const auto name = IOUtils::to_utf8_string(PLUGIN_NAME);
    const auto description = "First-party TAS plugin for Mupen64."
                             "\n"
                             "TAS plugins are not to be distributed separately from Mupen64 and remain tied "
                             "to one version of the emulator."
                             "\n\n"
                             "https://mupen64.com";
    const auto target_version = IOUtils::to_utf8_string(CURRENT_VERSION);

    auto result = std::format_to_n(metadata->name, sizeof(metadata->name) - 1, "{}", name);
    metadata->name[result.size] = '\0';

    result = std::format_to_n(metadata->description, sizeof(metadata->description) - 1, "{}", description);
    metadata->description[result.size] = '\0';

    result = std::format_to_n(metadata->target_version, sizeof(metadata->target_version) - 1, "{}", target_version);
    metadata->target_version[result.size] = '\0';
}

EXPORT void CALL M64RRProcessEvent(Event event)
{
    switch (event.type)
    {
    case M64RRSpec::Event::Type::Initiate:
        g_plugin = event.initiate.init;
        break;
    case M64RRSpec::Event::Type::RomOpened: {
        try
        {
            SDLAudio::Config cfg = read_config();
            g_backend.emplace(std::move(cfg));
        }
        catch (std::exception &e)
        {
            g_plugin->log_error(
                std::format("Exception at InitiateAudio(): {}", e.what()).c_str());
        }
        break;
    }
    case M64RRSpec::Event::Type::Shutdown:
    case M64RRSpec::Event::Type::RomClosed:
        if (g_backend.has_value()) g_backend.reset();
        break;
    default:
        break;
    }
}

EXPORT void CALL M64RRAIDacrateChanged(CoreSystemType system_type)
{
    // update sample rate
    if (!g_plugin || !g_backend) return;
    try
    {
        uint32_t sample_rate = compute_sample_rate(system_type, g_plugin->ai_register->ai_dacrate);
        g_backend->set_sample_rate(sample_rate);
    }
    catch (std::exception &e)
    {
        g_plugin->log_error(
            std::format("Exception at AiDacrateChanged(): {}", e.what()).c_str());
    }
}

EXPORT void CALL M64RRAILenChanged()
{
    const auto effective_speed_mode = g_plugin->get_effective_speed_mode();
    if (effective_speed_mode == CoreSpeedMode::UltraFastForward) return;

    // push new samples
    if (!g_plugin || !g_backend) return;
    uint32_t addr = g_plugin->ai_register->ai_dram_addr & 0x00FF'FFF8;
    uint32_t len = g_plugin->ai_register->ai_len & 0x0003'FFF8;

    try
    {
        g_backend->push_samples(g_plugin->rdram + addr, len);
        g_backend->sync_audio();
    }
    catch (std::exception &e)
    {
        g_plugin->log_error(std::format("Exception at AiLenChanged(): {}", e.what()).c_str());
    }
}
