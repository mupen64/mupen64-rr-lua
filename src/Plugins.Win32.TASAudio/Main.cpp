/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "Config.hpp"
#include "IOUtils.hpp"
#include "SDLBackend.hpp"
#include "core_types.h"
#include <CommonPCH.hpp>
#include <VersionNameHelpers.hpp>
#include <Views.Win32/MupenRRSpecPlugin.h>

static std::optional<MupenRRSpecPlugin::PluginInit> g_audio_info{};
std::optional<SDLAudio::SDLBackend> g_backend{};
MupenRRSpecPlugin::ExtendedFuncs *g_ef = nullptr;

std::filesystem::path g_dll_path{}; // currently set in Main_Win32.cpp
std::filesystem::path g_config_path{};
static bool g_sdl_is_init = false;

static uint32_t compute_sample_rate(uint32_t system_type, uint32_t dacrate)
{
    uint32_t vi_clock = 0;
    switch (system_type)
    {
    case sys_ntsc:
        vi_clock = 48681812;
        break;
    case sys_pal:
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
    return g_config_path / "TASAudio.json";
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

EXPORT void CALL M64RRGetMetadata(MupenRRSpecPlugin::PluginMetadata *metadata)
{
    metadata->type = MupenRRSpecPlugin::PluginType::Audio;

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

EXPORT void CALL M64RRShutdown()
{
    if (g_backend.has_value()) g_backend.reset();
}

EXPORT void CALL M64RRInitiate(MupenRRSpecPlugin::PluginInit *init)
{
    g_ef = init->ef;
    g_config_path = ZilmarExtSpec::get_config_path(g_ef);

    g_audio_info.emplace(*init);

    try
    {
        SDLAudio::Config cfg = read_config();
        g_backend.emplace(std::move(cfg));
    }
    catch (std::exception &e)
    {
        g_ef->log_error(IOUtils::to_wide_string(std::format("Exception at InitiateAudio(): {}", e.what())).c_str());
    }
}

EXPORT void CALL M64RRRomClosed()
{
    if (g_backend.has_value()) g_backend.reset();
}

EXPORT void CALL M64RRAIDacrateChanged(int32_t system_type)
{
    // update sample rate
    if (!g_audio_info || !g_backend) return;
    try
    {
        uint32_t sample_rate = compute_sample_rate(system_type, *g_audio_info->ai_dacrate_reg);
        g_backend->set_sample_rate(sample_rate);
    }
    catch (std::exception &e)
    {
        g_ef->log_error(IOUtils::to_wide_string(std::format("Exception at AiDacrateChanged(): {}", e.what())).c_str());
    }
}

EXPORT void CALL M64RRAILenChanged()
{
    const auto effective_speed_mode = g_ef->get_effective_speed_mode();
    if (effective_speed_mode == CoreSpeedMode::UltraFastForward) return;

    // push new samples
    if (!g_audio_info || !g_backend) return;
    uint32_t addr = *g_audio_info->ai_dram_addr_reg & 0x00FF'FFF8;
    uint32_t len = *g_audio_info->ai_len_reg & 0x0003'FFF8;

    try
    {
        g_backend->push_samples(g_audio_info->rdram + addr, len);
        g_backend->sync_audio();
    }
    catch (std::exception &e)
    {
        g_ef->log_error(IOUtils::to_wide_string(std::format("Exception at AiLenChanged(): {}", e.what())).c_str());
    }
}
