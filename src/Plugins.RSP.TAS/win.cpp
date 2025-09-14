/*
 * Copyright (c) 2025, hacktarux-azimer-rsp-hle maintainers, contributors, and original authors (Hacktarux, Azimer).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "win.h"
#include "main.h"
#include "Config.h"
#include "core_plugin.h"
#include "hle.h"

HINSTANCE g_instance;
std::filesystem::path g_app_path;
PlatformService g_platform_service;
static uint8_t fake_header[0x1000];
static uint32_t fake_AI_DRAM_ADDR_REG;
static uint32_t fake_AI_LEN_REG;
static uint32_t fake_AI_CONTROL_REG;
static uint32_t fake_AI_STATUS_REG;
static uint32_t fake_AI_DACRATE_REG;
static uint32_t fake_AI_BITRATE_REG;

// ProcessAList function from audio plugin, only populated when audio_external is true
void (*g_processAList)() = nullptr;

std::filesystem::path get_app_full_path()
{
    char path[MAX_PATH] = {0};

    const DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
    {
        return {};
    }

    return path;
}


char* getExtension(char* str)
{
    if (strlen(str) > 3)
        return str + strlen(str) - 3;
    else
        return NULL;
}

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_instance = hinst;
        g_app_path = get_app_full_path();
        config_load();
        break;
    default:
        break;
    }

    return TRUE;
}

void* plugin_load(const std::filesystem::path& path)
{
    const auto module = LoadLibrary(path.wstring().c_str());

    if (!module)
    {
        MessageBox(NULL, L"Failed to load the external audio plugin.\nEmulation will not behave as expected.", L"Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    core_audio_info info;
    // FIXME: Do we have to provide hwnd?
    info.main_hwnd = NULL;
    info.hinst = (HINSTANCE)rsp.hinst;
    info.byteswapped = TRUE;
    info.rom = fake_header;
    info.rdram = rsp.rdram;
    info.dmem = rsp.dmem;
    info.imem = rsp.imem;
    info.mi_intr_reg = rsp.mi_intr_reg;
    info.ai_dram_addr_reg = &fake_AI_DRAM_ADDR_REG;
    info.ai_len_reg = &fake_AI_LEN_REG;
    info.ai_control_reg = &fake_AI_CONTROL_REG;
    info.ai_status_reg = &fake_AI_STATUS_REG;
    info.ai_dacrate_reg = &fake_AI_DACRATE_REG;
    info.ai_bitrate_reg = &fake_AI_BITRATE_REG;
    info.check_interrupts = rsp.check_interrupts;
    auto initiateAudio = (BOOL(__cdecl*)(core_audio_info))GetProcAddress(module, "InitiateAudio");
    g_processAList = (void(__cdecl*)(void))GetProcAddress(module, "ProcessAList");
    initiateAudio(info);

    return module;
}

EXPORT void CALL DllAbout(HWND hwnd)
{
    const auto msg = PLUGIN_NAME L"\n"
                                 L"Part of the Mupen64 project family."
                                 L"\n\n"
                                 L"https://github.com/mupen64/mupen64-rr-lua";

    MessageBox(hwnd, msg, L"About", MB_ICONINFORMATION | MB_OK);
}

EXPORT void CALL DllConfig(HWND hwnd)
{
    if (rsp_alive())
    {
        MessageBox(hwnd, L"Close the ROM before configuring the plugin.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    config_show_dialog(hwnd);
}

EXPORT void CALL GetDllInfo(core_plugin_info* PluginInfo)
{
    PluginInfo->ver = 0x0101;
    PluginInfo->type = (int16_t)plugin_rsp;
    strncpy(PluginInfo->name, PLUGIN_NAME, std::size(PluginInfo->name));
    PluginInfo->unused_normal_memory = 1;
    PluginInfo->unused_byteswapped = 1;
}

EXPORT void CALL RomOpen(void)
{
    g_config_readonly = true;
}

EXPORT void CALL InitiateRSP(core_rsp_info Rsp_Info, uint32_t* CycleCount)
{
    rsp = Rsp_Info;
}

EXPORT void CALL RomClosed()
{
    on_rom_closed();
}

EXPORT uint32_t CALL DoRspCycles(uint32_t Cycles)
{
    return do_rsp_cycles(Cycles);
}
