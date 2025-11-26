/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "common.h"
#include "SoundDriverInterface.h"
#include "SoundDriverFactory.h"
#include "audiohle.h"
#include <string.h> // memcpy(), strcpy()
#include <stdio.h> // needed for configuration
#include <cassert>

SoundDriverInterface* snd = NULL;
bool ai_delayed_carry;
bool bBackendChanged = false;
bool first_time = true;
HINSTANCE hInstance;
OSVERSIONINFOEX OSInfo;
core_audio_info AudioInfo;
u32 Dacrate = 0;


void SetTimerResolution()
{
    const HMODULE hmod = GetModuleHandle("ntdll.dll");
    if (!hmod)
    {
        return;
    }

    typedef LONG(NTAPI * tNtSetTimerResolution)(IN ULONG DesiredResolution, IN Boolean SetResolution, OUT PULONG CurrentResolution);
    tNtSetTimerResolution NtSetTimerResolution = (tNtSetTimerResolution)GetProcAddress(hmod, "NtSetTimerResolution");
    ULONG CurrentResolution = 0;
    NtSetTimerResolution(10000, TRUE, &CurrentResolution);
}

bool WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hInstance = hinstDLL;
    return TRUE;
}

EXPORT void CALL DllAbout(void* hParent)
{
    const auto msg = PLUGIN_FULL_NAME "\n"
                                      "Part of the Mupen64 project family."
                                      "\n\n"
                                      "https://github.com/mupen64/better-azi";

    MessageBox((HWND)hParent, msg, "About", MB_ICONINFORMATION | MB_OK);
}

EXPORT void CALL DllConfig(void* hParent)
{
    SoundDriverType currentDriver = Configuration::getDriver();
    Configuration::ConfigDialog((HWND)hParent);
    if (currentDriver != Configuration::getDriver())
    {
        bBackendChanged = true;
    }
}

EXPORT Boolean CALL InitiateAudio(core_audio_info Audio_Info)
{
    if (snd != NULL)
    {
        snd->AI_Shutdown();
        delete snd;
    }

    if (Configuration::getResTimer() == true)
    {
        SetTimerResolution();
    }

    memcpy(&AudioInfo, &Audio_Info, sizeof(core_audio_info));
    DRAM = Audio_Info.rdram;
    DMEM = Audio_Info.dmem;
    IMEM = Audio_Info.imem;

    Configuration::Header = (t_romheader*)Audio_Info.rom;
    Configuration::LoadDefaults();
    Configuration::LoadSettings();
    snd = SoundDriverFactory::CreateSoundDriver(Configuration::getDriver());

    if (snd == NULL)
        return FALSE;

    snd->AI_Startup();
    ai_delayed_carry = false;
    return TRUE;
}

EXPORT void CALL CloseDLL(void)
{
    DEBUG_OUTPUT "Call: CloseDLL()\n";
    if (snd != NULL)
    {
        snd->AI_Shutdown();
        delete snd;
        snd = NULL;
    }
}

EXPORT void CALL GetDllInfo(core_plugin_info* PluginInfo)
{
    PluginInfo->unused_byteswapped = TRUE;
    PluginInfo->unused_normal_memory = FALSE;
    strcpy_s(PluginInfo->name, 100, PLUGIN_FULL_NAME);
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
    if (snd == NULL)
        return;
    HLEStart();
}

EXPORT void CALL RomOpen(void)
{
    DEBUG_OUTPUT "Call: RomOpen()\n";
    Configuration::RomRunning = true;
    first_time = false;
    Configuration::LoadSettings();
}

EXPORT void CALL RomClosed(void)
{
    Configuration::RomRunning = false;
    Configuration::LoadSettings();
    DEBUG_OUTPUT "Call: RomClosed()\n";
    Dacrate = 0; // Forces a revisit to initialize audio
    if (snd == NULL)
        return;
    if (bBackendChanged == true)
    {
        snd->AI_Shutdown();
        delete snd;
        snd = SoundDriverFactory::CreateSoundDriver(Configuration::getDriver());
        snd->AI_Startup();
        bBackendChanged = false;
    }
    else
    {
        snd->AI_ResetAudio();
    }
}

EXPORT void CALL AiDacrateChanged(int SystemType)
{
    u32 video_clock;

    ai_delayed_carry = false;
    DEBUG_OUTPUT "Call: AiDacrateChanged()\n";
    if (snd == NULL)
        return;
    if (Dacrate == *AudioInfo.ai_dacrate_reg)
        return;

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
        DEBUG_OUTPUT "Unable to standardize Frequeny!\n";
    DEBUG_OUTPUT "Frequency = %i\n", Frequency;
    snd->AI_SetFrequency(Frequency);
}

EXPORT void CALL AiLenChanged(void)
{
    u32 address = *AudioInfo.ai_dram_addr_reg & 0x00FFFFF8;
    u32 length = *AudioInfo.ai_len_reg & 0x3FFF8;

    if (snd == NULL)
        return;

    if (ai_delayed_carry)
        address += 0x2000;

    if ((address + length & 0x1FFF) == 0)
        ai_delayed_carry = true;
    else
        ai_delayed_carry = false;

    snd->AI_LenChanged(AudioInfo.rdram + address, length);
}

EXPORT u32 CALL AiReadLength(void)
{
    if (snd == NULL)
        return 0;
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
