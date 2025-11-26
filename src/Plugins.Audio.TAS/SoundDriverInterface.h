/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include "common.h"
#include "Configuration.h"

class SoundDriverInterface {
private:
public:
    virtual ~SoundDriverInterface() {};
    // Setup and Teardown Functions
    virtual Boolean Initialize() = 0;
    virtual void DeInitialize() = 0;

    // Management functions
    virtual void AiUpdate(Boolean Wait) {}; // Optional
    virtual void StopAudio() = 0; // Stops the Audio PlayBack (as if paused)
    virtual void StartAudio() = 0; // Starts the Audio PlayBack (as if unpaused)
    virtual void SetFrequency(u32 Frequency) = 0; // Sets the Nintendo64 Game Audio Frequency

    // Audio Spec interface methods (new)
    virtual void AI_SetFrequency(u32 Frequency) = 0;
    virtual void AI_LenChanged(u8* start, u32 length) = 0;
    virtual u32 AI_ReadLength() = 0;
    virtual void AI_Startup() = 0;
    virtual void AI_Shutdown() = 0;
    virtual void AI_ResetAudio() = 0;
    virtual void AI_Update(Boolean Wait) = 0;

    virtual void SetVolume(u32 volume) = 0;
};
