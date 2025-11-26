/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <windows.h>

#include "SoundDriver.h"
#include <xaudio2.h>

class VoiceCallback : public IXAudio2VoiceCallback {
public:
    VoiceCallback() = default;
    ~VoiceCallback() = default;

    void __stdcall OnStreamEnd() {}
    void __stdcall OnVoiceProcessingPassEnd() {}
    void __stdcall OnVoiceProcessingPassStart(UINT32);
    void __stdcall OnBufferEnd(void*);
    void __stdcall OnBufferStart(void*) {}
    void __stdcall OnLoopEnd(void*) {}
    void __stdcall OnVoiceError(void*, HRESULT) {}
};

class XAudio2SoundDriver : public SoundDriver {
public:
    XAudio2SoundDriver();
    ~XAudio2SoundDriver();

    BOOL Initialize();
    void DeInitialize();

    BOOL Setup();
    void Teardown();

    void SetFrequency(u32 Frequency);
    void PlayBuffer(u8* data, int bufferSize);

    void AiUpdate(BOOL Wait);
    void StopAudio();
    void StartAudio();
    void StopAudioThread();
    void StartAudioThread();

    void SetVolume(u32 volume);

    static SoundDriverInterface* CreateSoundDriver() { return new XAudio2SoundDriver(); }

protected:
    bool dllInitialized;
    static DWORD WINAPI AudioThreadProc(LPVOID lpParameter);

private:
    HANDLE hAudioThread;
    bool bStopAudioThread;
    static bool ClassRegistered;
};
