/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "common.h"
#if defined(ENABLE_BACKEND_DIRECTSOUND8_LEGACY) || defined(ENABLE_BACKEND_XAUDIO2_LEGACY)
#include "SoundDriverLegacy.h"
// Load the buffer from the AI interface to our emulated buffer
void SoundDriverLegacy::AI_LenChanged(u8* start, u32 length)
{
    if (m_audioIsInitialized == false)
    {
        *AudioInfo.AI_STATUS_REG = AI_STATUS_DMA_BUSY;
        *AudioInfo.MI_INTR_REG |= MI_INTR_AI;
    }
    else
    {
        AddBuffer(start, length);
    }
}

void SoundDriverLegacy::AI_SetFrequency(u32 Frequency)
{
    m_SamplesPerSecond = Frequency;
    if (m_audioIsInitialized == true)
        SetFrequency(Frequency);
}

u32 SoundDriverLegacy::AI_ReadLength()
{
    if (m_audioIsInitialized == false)
        return 0;
    return GetReadStatus();
}

void SoundDriverLegacy::AI_Startup()
{
    if (m_audioIsInitialized == true)
        DeInitialize();
    m_audioIsInitialized = false;
    m_audioIsInitialized = (Initialize() == FALSE);
    if (m_audioIsInitialized == true)
        SetVolume(Configuration::getVolume());
    StartAudio();
}

void SoundDriverLegacy::AI_Shutdown()
{
    StopAudio();
    if (m_audioIsInitialized == true)
        DeInitialize();
    m_audioIsInitialized = false;
    // DeInitialize();
}

void SoundDriverLegacy::AI_ResetAudio()
{
    if (m_audioIsInitialized == true)
        AI_Shutdown();
    DeInitialize();
    m_audioIsInitialized = false;
    AI_Startup();
    m_audioIsInitialized = (Initialize() == FALSE);
    StartAudio();
}

void SoundDriverLegacy::AI_Update(Boolean Wait)
{
    AiUpdate(Wait);
}
#endif
