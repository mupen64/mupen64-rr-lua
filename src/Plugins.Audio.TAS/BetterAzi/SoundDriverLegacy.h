/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <windows.h>
#include "SoundDriverInterface.h"

#define SND_IS_NOT_EMPTY 0x4000000
#define SND_IS_FULL 0x8000000

class SoundDriverLegacy : public SoundDriverInterface {
public:
    // Deprecated
    virtual u32 GetReadStatus() = 0; // Returns the status on the read pointer
    virtual u32 AddBuffer(u8* start, u32 length) = 0; // Uploads a new buffer and returns status

    // Sound Driver Factory method
    static SoundDriverLegacy* SoundDriverFactory();

    virtual void SetVolume(u32 volume) {}; // We could potentially do this ourselves within the buffer copy method
    virtual ~SoundDriverLegacy() {};

    void AI_SetFrequency(u32 Frequency);
    void AI_LenChanged(u8* start, u32 length);
    u32 AI_ReadLength();
    void AI_Startup();
    void AI_Shutdown();
    void AI_ResetAudio();
    void AI_Update(Boolean Wait);

protected:
    // Temporary (to allow for incremental development)
    bool m_audioIsInitialized;

    // Mutex Handle
#ifdef _WIN32
    HANDLE m_hMutex;
#else
    pthread_mutex_t m_Mutex;
#endif

    u32 m_SamplesPerSecond;

    SoundDriverLegacy()
    {
        m_audioIsInitialized = false;
#ifdef _WIN32
        m_hMutex = NULL;
#else
        m_Mutex = NULL;
#endif
    }
};
