/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "common.h"

#pragma pack(push, 1)
struct t_trivial_config
{
    int32_t version = 1;
    int32_t sync_audio{};
    int32_t force_sync{};
    int32_t ai_emulation{};
    int32_t volume = 100;
    int32_t buffer_level = 3;
    int32_t buffer_fps = 45;
    int32_t backend_fps = 90;
    int32_t disallow_sleep_xa2{};
    int32_t disallow_sleep_ds8{};
    int32_t frequency = 44100;
    int32_t bit_rate = 16;
};
#pragma pack(pop)

class Configuration
{
  public:
    static const int MAX_FOLDER_LENGTH = 500;
    static const int MAX_DEVICE_LENGTH = 100;

    static char configAudioLogFolder[MAX_FOLDER_LENGTH];
    static t_trivial_config currentSettings;

    static void setAIEmulation(bool value) { currentSettings.ai_emulation = value; }
    static void setSyncAudio(bool value) { currentSettings.sync_audio = value; }
    static void setForceSync(bool value) { currentSettings.force_sync = value; }
    static void setVolume(unsigned long value) { currentSettings.volume = value; }
    static void setFrequency(unsigned long value) { currentSettings.frequency = value; }
    static void setBitRate(unsigned long value) { currentSettings.bit_rate = value; }
    static void setBufferLevel(unsigned long value) { currentSettings.buffer_level = value; }
    static void setBufferFPS(unsigned long value) { currentSettings.buffer_fps = value; }
    static void setBackendFPS(unsigned long value) { currentSettings.backend_fps = value; }
    static void setDisallowSleepXA2(bool value) { currentSettings.disallow_sleep_xa2 = value; }
    static void setDisallowSleepDS8(bool value) { currentSettings.disallow_sleep_ds8 = value; }

    static void ResetAdvancedPage(HWND hDlg);

    static bool RomRunning;
    static void LoadSettings();
    static void SaveSettings();
    static bool config_load();
    static bool config_save();
    static void ConfigDialog(HWND hParent);

    static bool getAIEmulation() { return currentSettings.ai_emulation; }
    static unsigned long getVolume() { return currentSettings.volume; }
    static bool getForceSync() { return currentSettings.force_sync; }
    static bool getSyncAudio() { return currentSettings.sync_audio; }
    static unsigned long getFrequency() { return currentSettings.frequency; }
    static unsigned long getBitRate() { return currentSettings.bit_rate; }
    static unsigned long getBufferLevel() { return currentSettings.buffer_level; }
    static unsigned long getBufferFPS() { return currentSettings.buffer_fps; }
    static unsigned long getBackendFPS() { return currentSettings.backend_fps; }
    static bool getDisallowSleepXA2() { return currentSettings.disallow_sleep_xa2; }
    static bool getDisallowSleepDS8() { return currentSettings.disallow_sleep_ds8; }

    static char *getAudioLogFolder()
    {
        static char retVal[MAX_FOLDER_LENGTH];
        strcpy(retVal, configAudioLogFolder);
        return retVal;
    }
};
