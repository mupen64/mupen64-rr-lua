/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "SoundDriver.h"
#include "WaveOut.h"
#include <cassert>

// Load the buffer from the AI interface to our emulated buffer
void SoundDriver::AI_LenChanged(u8* start, u32 length)
{
    if (length == 0)
    {
        *AudioInfo.ai_status_reg = 0;
        *AudioInfo.mi_intr_reg |= MI_INTR_AI;
        AudioInfo.check_interrupts();
        return;
    }

    // Bleed off some of this buffer to smooth out audio (buffer overflow handling)
    if (length < m_MaxBufferSize && Configuration::getSyncAudio())
    {
        if (m_MaxBufferSize == m_BufferRemaining)
        {
            DEBUG_OUTPUT "B"; // Debug that we overflowed (shouldn't happen with locked FPS)
        }
        while (m_MaxBufferSize == m_BufferRemaining)
        {
            Sleep(1);
        }
    }

    lastLength = length;

    WaitForSingleObject(m_hMutex, INFINITE);

    BufferAudio();

    if (m_AI_DMASecondaryBytes > 0)
    {
        DEBUG_OUTPUT "X";
        s32 tmp = m_AI_DMASecondaryBytes - m_AI_DMAPrimaryBytes;
        if (tmp > 0)
        {
            m_AI_DMAPrimaryBuffer = m_AI_DMASecondaryBuffer + tmp;
            m_AI_DMASecondaryBuffer = NULL;
            m_AI_DMASecondaryBytes = 0;
        }
        else
        {
            m_AI_DMAPrimaryBuffer = m_AI_DMASecondaryBuffer;
            m_AI_DMASecondaryBuffer = NULL;
            m_AI_DMAPrimaryBytes = m_AI_DMASecondaryBytes;
            m_AI_DMASecondaryBytes = 0;
        }
    }

    m_AI_DMASecondaryBuffer = start;
    m_AI_DMASecondaryBytes = length;
    if (m_AI_DMAPrimaryBytes == 0)
    {
        m_AI_DMAPrimaryBuffer = m_AI_DMASecondaryBuffer;
        m_AI_DMASecondaryBuffer = NULL;
        m_AI_DMAPrimaryBytes = m_AI_DMASecondaryBytes;
        m_AI_DMASecondaryBytes = 0;
    }

    if (Configuration::getAIEmulation() == true)
    {
        *AudioInfo.ai_status_reg = AI_STATUS_DMA_BUSY;
        if (m_AI_DMAPrimaryBytes > 0 && m_AI_DMASecondaryBytes > 0)
        {
            *AudioInfo.ai_status_reg = AI_STATUS_DMA_BUSY | AI_STATUS_FIFO_FULL;
        }
    }
    BufferAudio();

    ReleaseMutex(m_hMutex);

    // Bleed off some of this buffer to smooth out audio
    if (length < m_MaxBufferSize && Configuration::getSyncAudio() == true && m_AI_DMASecondaryBytes > 0)
    {
        if (m_MaxBufferSize == m_BufferRemaining)
        {
            DEBUG_OUTPUT "O"; // Debug that we overflowed (shouldn't happen with locked FPS)
        }
        while (m_MaxBufferSize == m_BufferRemaining)
        {
            Sleep(1);
        }
    }
}

void SoundDriver::AI_SetFrequency(u32 Frequency)
{
    m_SamplesPerSecond = Frequency;
    SetFrequency(Frequency);
    m_MaxBufferSize = (u32)(Frequency / Configuration::getBufferFPS()) * 4 * Configuration::getBufferLevel();
    m_BufferRemaining = 0;
    m_CurrentReadLoc = m_CurrentWriteLoc = m_BufferRemaining = 0;
}

u32 SoundDriver::AI_ReadLength()
{
    const u32 LENGTHFACTOR = 32;
    u32 retVal;

    if (Configuration::getAIEmulation() == false || lastLength == 0)
        return 0;

    WaitForSingleObject(m_hMutex, INFINITE);

    if (m_BufferRemaining > lastLength * 2 + lastLength / 4)
    {
        retVal = lastLength; // TODO: I do not remember why I set this...
    }
    else
    {
        retVal = m_AI_DMAPrimaryBytes;
    }

    if (retVal == lastReadLength)
    {
        if (retVal > LENGTHFACTOR * (lastReadCount + 1))
        {
            lastReadCount++;
            retVal -= LENGTHFACTOR * lastReadCount;
        }
    }
    else
    {
        lastReadCount = 0;
        lastReadLength = retVal;
    }
#ifdef _WIN32
    ReleaseMutex(m_hMutex);
#endif
    return retVal & ~0x7;
}

void SoundDriver::AI_Startup()
{
    Initialize();
    m_AI_DMAPrimaryBytes = m_AI_DMASecondaryBytes = 0;
    m_AI_DMAPrimaryBuffer = m_AI_DMASecondaryBuffer = NULL;

    m_MaxBufferSize = MAX_SIZE;
    m_CurrentReadLoc = m_CurrentWriteLoc = m_BufferRemaining = 0;
    m_DMAEnabled = false;
    lastReadLength = 0;
    lastReadCount = 0;
    lastLength = 0;

    if (m_hMutex == NULL)
    {
        m_hMutex = CreateMutex(NULL, FALSE, NULL);
    }

    StartAudio();
    SetVolume(Configuration::getVolume());
}

void SoundDriver::AI_Shutdown()
{
    StopAudio();
    DeInitialize();
    m_BufferRemaining = 0;
    if (m_hMutex)
    {
        CloseHandle(m_hMutex);
        m_hMutex = nullptr;
    }
}

void SoundDriver::AI_ResetAudio()
{
    m_BufferRemaining = 0;
    if (m_audioIsInitialized == true)
        AI_Shutdown();
    DeInitialize();
    m_audioIsInitialized = false;
    AI_Startup();
    m_audioIsInitialized = Initialize() == FALSE;
    StartAudio();
}

void SoundDriver::AI_Update(Boolean Wait)
{
    if (Wait)
        Sleep(10); // TODO:  Fixme -- Ai Update appears to be problematic
    AiUpdate(Wait);
}

void SoundDriver::BufferAudio()
{
    m_DMAEnabled = (*AudioInfo.ai_control_reg & AI_CONTROL_DMA_ON) == AI_CONTROL_DMA_ON;
    if (m_DMAEnabled == false)
        return;
    while (m_BufferRemaining < m_MaxBufferSize && (m_AI_DMAPrimaryBytes > 0 || m_AI_DMASecondaryBytes > 0))
    {
        *(u16*)(m_Buffer + m_CurrentWriteLoc) = *(u16*)(m_AI_DMAPrimaryBuffer + 2);
        *(u16*)(m_Buffer + m_CurrentWriteLoc + 2) = *(u16*)m_AI_DMAPrimaryBuffer;
        m_CurrentWriteLoc += 4;
        m_AI_DMAPrimaryBuffer += 4;
        m_CurrentWriteLoc %= m_MaxBufferSize;
        assert(m_BufferRemaining <= m_MaxBufferSize);
        m_BufferRemaining += 4;
        assert(m_BufferRemaining <= m_MaxBufferSize);
        m_AI_DMAPrimaryBytes -= 4;
        if (m_AI_DMAPrimaryBytes == 0)
        {
            m_AI_DMAPrimaryBytes = m_AI_DMASecondaryBytes;
            m_AI_DMAPrimaryBuffer = m_AI_DMASecondaryBuffer; // Switch
            m_AI_DMASecondaryBytes = 0;
            m_AI_DMASecondaryBuffer = NULL;
            if (Configuration::getAIEmulation() == true)
            {
                lastReadLength = 0;
                lastReadCount = 0;
                *AudioInfo.ai_status_reg = AI_STATUS_DMA_BUSY;
                *AudioInfo.ai_status_reg &= ~AI_STATUS_FIFO_FULL;
                *AudioInfo.mi_intr_reg |= MI_INTR_AI;
                AudioInfo.check_interrupts();
                if (m_AI_DMAPrimaryBytes == 0)
                    *AudioInfo.ai_status_reg = 0;
            }
        }
    }
}

// Copies data to the audio playback buffer
u32 SoundDriver::LoadAiBuffer(u8* start, u32 length)
{
    u32 bytesToMove = length;
    u8* ptrStart = start;
    u8 nullBuff[MAX_SIZE];
    u32 writePtr = 0;
    static u32 lastSample = 0;

    if (start == NULL)
        ptrStart = nullBuff;

    assert((length & 0x3) == 0);
    assert(bytesToMove <= m_MaxBufferSize); // We shouldn't be asking for more.

    m_DMAEnabled = (*AudioInfo.ai_control_reg & AI_CONTROL_DMA_ON) == AI_CONTROL_DMA_ON;

    if (bytesToMove > m_MaxBufferSize)
    {
        memset(ptrStart, 0, length);
        return length;
    }

    // Return silence -- DMA is disabled
    if (m_DMAEnabled == false)
    {
        memset(ptrStart, 0, length);
        return length;
    }

    WaitForSingleObject(m_hMutex, INFINITE);

    // Step 0: Replace depleted stored buffer for next run
    BufferAudio();

    // Step 1: Deplete stored buffer (should equal length size)
    if (bytesToMove <= m_BufferRemaining)
    {
        while (bytesToMove > 0 && m_BufferRemaining > 0)
        {
            lastSample = *(u32*)(ptrStart + writePtr) = *(u32*)(m_Buffer + m_CurrentReadLoc);
            m_CurrentReadLoc += 4;
            writePtr += 4;
            m_CurrentReadLoc %= m_MaxBufferSize;
            assert(m_BufferRemaining <= m_MaxBufferSize);
            m_BufferRemaining -= 4;
            assert(m_BufferRemaining <= m_MaxBufferSize);
            bytesToMove -= 4;
        }
        assert(writePtr == length);
    }

    // Step 2: Fill bytesToMove with silence
    if (bytesToMove == length)
        DEBUG_OUTPUT "S";

    while (bytesToMove > 0)
    {
        *(u32*)(ptrStart + writePtr) = lastSample;
        writePtr += 4;
        bytesToMove -= 4;
    }

    // Step 3: Replace depleted stored buffer for next run
    BufferAudio();

    ReleaseMutex(m_hMutex);

    assert(bytesToMove == 0);
    return length - bytesToMove;
}
