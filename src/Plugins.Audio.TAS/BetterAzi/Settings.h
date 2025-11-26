/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "my_types.h"

class Settings {
public:
    unsigned long configFrequency;
    unsigned long configBufferLevel;
    unsigned long configBufferFPS;
    unsigned long configBackendFPS;
    bool configAIEmulation;
    bool configSyncAudio;
    bool configForceSync;
    bool configDisallowSleepXA2;
    bool configDisallowSleepDS8;
    unsigned long configBitRate;
    bool configResTimer;

    u16 CartID;
    char CartName[21];
};
