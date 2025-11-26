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

class SoundDriver : public SoundDriverInterface {
public:
    u32 LoadAiBuffer(u8* start, u32 length);
    void BufferAudio();

    static SoundDriver* SoundDriverFactory();

    virtual void SetVolume(u32 volume) {}
    virtual ~SoundDriver() {}

    void AI_SetFrequency(u32 Frequency);
    void AI_LenChanged(u8* start, u32 length);
    u32 AI_ReadLength();
    void AI_Startup();
    void AI_Shutdown();
    void AI_ResetAudio();
    void AI_Update(Boolean Wait);

protected:
    // Variables for Buffering audio samples from AI DMA
    // Max Buffer Size (44100Hz * 16bit * Stereo)
    static const int MAX_SIZE = 44100 * 2 * 2;

    bool m_audioIsInitialized{};
    bool m_isValid{};

    HANDLE m_hMutex{};

    // Variables for AI DMA emulation
    u8* m_AI_DMAPrimaryBuffer{};
    u8* m_AI_DMASecondaryBuffer{};
    u32 m_AI_DMAPrimaryBytes{};
    u32 m_AI_DMASecondaryBytes{};

    // Variable size determined by Playback rate
    u32 m_MaxBufferSize{};

    // Currently playing Buffer
    u32 m_CurrentReadLoc{};

    // Currently writing Buffer
    u32 m_CurrentWriteLoc{};

    // Emulated buffers
    u8 m_Buffer[MAX_SIZE]{};

    // Buffer remaining
    u32 m_BufferRemaining{};

    // Sets to true when DMA is enabled
    bool m_DMAEnabled{};

    u32 m_SamplesPerSecond{};

    // Needed to smooth out the ReadLength
    u32 lastReadLength{};
    u32 lastReadCount{};
    u32 lastLength{};
};
