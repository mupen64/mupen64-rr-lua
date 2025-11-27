/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Windows.h>
#include <mmreg.h>
#include <dsound.h>
#include "SoundDriver.h"
#include "SoundDriverInterface.h"

class DirectSoundDriver : public SoundDriver {
protected:
    void (*CallBack)(DWORD);
    DWORD dwFreqTarget{};
    BOOL audioIsPlaying{};
    bool threadRunning{};
    HANDLE handleAudioThread{};
    DWORD dwAudioThreadId{};
    HANDLE hMutex{};
    LPDIRECTSOUNDBUFFER lpdsbuf{};
    LPDIRECTSOUND8 lpds{};
    bool audioIsDone{};
    BYTE SoundBuffer[500000]{};
    DWORD readLoc{};
    DWORD writeLoc{};
    volatile DWORD remainingBytes{};
    DWORD SampleRate{};
    DWORD SegmentSize{};

public:
    friend DWORD WINAPI AudioThreadProc(DirectSoundDriver* ac);

    DirectSoundDriver() = default;
    ~DirectSoundDriver() = default;

    BOOL Initialize();
    void DeInitialize();

    void SetFrequency(u32 Frequency);
    void SetSegmentSize(DWORD length);

    void AiUpdate(BOOL Wait);
    void StopAudio();
    void StartAudio();

    void SetVolume(u32 volume);

    static SoundDriverInterface* CreateSoundDriver() { return new DirectSoundDriver(); }

private:
    static bool ClassRegistered;
};
