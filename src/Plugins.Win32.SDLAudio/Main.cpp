/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Config.hpp"
#include "SDLBackend.hpp"
#include "core_plugin.h"
#include <CommonPCH.h>
#include <DummyPluginStub.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <VersionNameHelpers.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>
#include <optional>
#include <stdexcept>
#include <utility>
#include <winnt.h>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"SDL Audio", L"1.0.0")

static std::optional<core_audio_info> g_audio_info{};
static std::optional<SDLAudio::SDLBackend> g_backend{};
static core_plugin_extended_funcs *g_ef = nullptr;

static bool g_sdl_is_init = false;

static const SDL_InitFlags SDL_INIT_NEEDED = SDL_INIT_AUDIO;

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

BOOL __stdcall DllMain(HMODULE hmod, const DWORD reason, LPVOID)
{
    return 1;
}

EXPORT void CALL CloseDLL(void)
{
    if (g_backend.has_value()) g_backend.reset();
    SDL_QuitSubSystem(SDL_INIT_NEEDED);
}

EXPORT void CALL ReceiveExtendedFuncs(core_plugin_extended_funcs *g_fwd_funcs)
{
    g_ef = g_fwd_funcs;
}

EXPORT void CALL GetDllInfo(core_plugin_info *PluginInfo)
{
    PluginInfo->unused_byteswapped = TRUE;
    PluginInfo->unused_normal_memory = FALSE;
    strcpy_s(PluginInfo->name, 100, IOUtils::to_utf8_string(PLUGIN_NAME).c_str());
    PluginInfo->type = plugin_audio;
    PluginInfo->ver = 0x0101;
}
EXPORT void CALL DllAbout(void *hParent)
{
    const auto *msg = PLUGIN_NAME L"\n"
                                  L"Part of the Mupen64 project family."
                                  L"\n\n"
                                  L"https://github.com/mupen64/mupen64-rr-lua";
    MessageBoxW((HWND)hParent, msg, L"About", 0x00000040L | 0x00000000L);
}

EXPORT int32_t CALL InitiateAudio(core_audio_info Audio_Info)
{
    if (!g_sdl_is_init)
    {
        if (!SDL_Init(SDL_INIT_NEEDED)) {
            return 0;
        }
        g_sdl_is_init = true;
    }

    g_audio_info.emplace(Audio_Info);
    g_backend.emplace(SDLAudio::Config{}); // TODO: add config dialog

    return 1;
}

EXPORT void CALL RomOpen()
{
}

EXPORT void CALL RomClosed()
{
    if (g_backend.has_value()) g_backend.reset();
}

EXPORT void CALL AiDacrateChanged(int32_t system_type)
{
    // update sample rate
    if (!g_audio_info || !g_backend) return;
    uint32_t sample_rate = compute_sample_rate(system_type, *g_audio_info->ai_dacrate_reg);
    g_backend->set_sample_rate(sample_rate);
}

EXPORT void CALL AiLenChanged(void)
{
    // push new samples
    uint32_t addr = *g_audio_info->ai_dram_addr_reg & 0x00FF'FFF8;
    uint32_t len = *g_audio_info->ai_len_reg & 0x0003'FFF8;

    g_backend->push_samples(g_audio_info->rdram + addr, len);
    g_backend->sync_audio();
}

EXPORT uint32_t CALL AiReadLength(void)
{
    return 0;
}

EXPORT void CALL AiUpdate(int32_t wait)
{
    // no-op
}

EXPORT void CALL ProcessAList(void)
{
    // no-op
}