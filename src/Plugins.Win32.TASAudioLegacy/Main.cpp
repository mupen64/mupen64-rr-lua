/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.h"
#include "SoundDriverInterface.h"
#include "AudioHLE.h"
#include "DirectSoundDriver.h"

static void log_shim(const wchar_t *str)
{
    wprintf(str);
}

static core_plugin_extended_funcs ef_shim = {
    .size = sizeof(core_plugin_extended_funcs),
    .log_trace = log_shim,
    .log_info = log_shim,
    .log_warn = log_shim,
    .log_error = log_shim,
};

SoundDriverInterface *snd = NULL;
bool ai_delayed_carry;
bool first_time = true;
HINSTANCE hInstance;
OSVERSIONINFOEX OSInfo;
core_audio_info AudioInfo;
u32 Dacrate = 0;
core_plugin_extended_funcs *g_ef = &ef_shim;

bool WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hInstance = hinstDLL;
    return TRUE;
}

EXPORT void CALL ReceiveExtendedFuncs(core_plugin_extended_funcs *funcs)
{
    g_ef = funcs;
}

EXPORT void CALL DllAbout(void *hParent)
{
    const auto msg = PLUGIN_NAME L"\n"
                                 L"Part of the Mupen64 project family."
                                 L"\n\n"
                                 L"https://github.com/mupen64/mupen64-rr-lua";

    MessageBox((HWND)hParent, msg, L"About", MB_ICONINFORMATION | MB_OK);
}

EXPORT void CALL DllConfig(void *hParent)
{
    Configuration::ConfigDialog((HWND)hParent);
}

EXPORT Boolean CALL InitiateAudio(core_audio_info Audio_Info)
{
    if (snd != NULL)
    {
        snd->AI_Shutdown();
        delete snd;
    }

    memcpy(&AudioInfo, &Audio_Info, sizeof(core_audio_info));
    DRAM = Audio_Info.rdram;
    DMEM = Audio_Info.dmem;
    IMEM = Audio_Info.imem;

    Configuration::LoadSettings();
    snd = DirectSoundDriver::CreateSoundDriver();

    if (snd == NULL) return FALSE;

    snd->AI_Startup();
    ai_delayed_carry = false;
    return TRUE;
}

EXPORT void CALL CloseDLL(void)
{
    if (snd != NULL)
    {
        snd->AI_Shutdown();
        delete snd;
        snd = NULL;
    }
}

EXPORT void CALL GetDllInfo(core_plugin_info *PluginInfo)
{
    PluginInfo->unused_byteswapped = TRUE;
    PluginInfo->unused_normal_memory = FALSE;
    strcpy_s(PluginInfo->name, 100, IOUtils::to_utf8_string(PLUGIN_NAME).c_str());
    PluginInfo->type = plugin_audio;
    PluginInfo->ver = 0x0101;
}

EXPORT void CALL ProcessAList(void)
{
    Configuration::RomRunning = true;
    if (first_time)
    {
        first_time = false;
        Configuration::LoadSettings();
    }
    if (snd == NULL) return;
    HLEStart();
}

EXPORT void CALL RomOpen(void)
{
    Configuration::RomRunning = true;
    first_time = false;
    Configuration::LoadSettings();
}

EXPORT void CALL RomClosed(void)
{
    Configuration::RomRunning = false;
    Configuration::LoadSettings();
    Dacrate = 0; // Forces a revisit to initialize audio
    if (snd == NULL) return;
    snd->AI_ResetAudio();
}

EXPORT void CALL AiDacrateChanged(int SystemType)
{
    u32 video_clock;

    ai_delayed_carry = false;
    if (snd == NULL) return;
    if (Dacrate == *AudioInfo.ai_dacrate_reg) return;

    Dacrate = *AudioInfo.ai_dacrate_reg & 0x00003FFF;

    switch (SystemType)
    {
    case 0:
        video_clock = 48681812;
        break;
    case 1:
        video_clock = 49656530;
        break;
    case 2:
        video_clock = 48628316;
        break;
    default:
        assert(FALSE);
    }

    u32 Frequency = video_clock / (Dacrate + 1);

    if (Frequency > 7000 && Frequency < 9000)
        Frequency = 8000;
    else if (Frequency > 10000 && Frequency < 12000)
        Frequency = 11025;
    else if (Frequency > 18000 && Frequency < 20000)
        Frequency = 19000;
    else if (Frequency > 21000 && Frequency < 23000)
        Frequency = 22050;
    else if (Frequency > 31000 && Frequency < 33000)
        Frequency = 32000;
    else if (Frequency > 43000 && Frequency < 45000)
        Frequency = 44100;
    else if (Frequency > 47000 && Frequency < 49000)
        Frequency = 48000;
    else
        g_ef->log_error(std::format(L"Unknown AI Frequency {}", Frequency).c_str());

    snd->AI_SetFrequency(Frequency);
}

EXPORT void CALL AiLenChanged(void)
{
    u32 address = *AudioInfo.ai_dram_addr_reg & 0x00FFFFF8;
    u32 length = *AudioInfo.ai_len_reg & 0x3FFF8;

    if (snd == NULL) return;

    if (ai_delayed_carry) address += 0x2000;

    if ((address + length & 0x1FFF) == 0)
        ai_delayed_carry = true;
    else
        ai_delayed_carry = false;

    snd->AI_LenChanged(AudioInfo.rdram + address, length);
}

EXPORT u32 CALL AiReadLength(void)
{
    if (snd == NULL) return 0;
    *AudioInfo.ai_len_reg = snd->AI_ReadLength();
    return *AudioInfo.ai_len_reg;
}

EXPORT void CALL AiUpdate(Boolean Wait)
{
    if (!snd)
    {
        Sleep(1);
        return;
    }
    snd->AI_Update(Wait);
}
