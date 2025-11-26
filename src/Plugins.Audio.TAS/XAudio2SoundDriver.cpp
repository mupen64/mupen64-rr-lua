/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "common.h"
#include "XAudio2SoundDriver.h"
#include <stdio.h>
#include <cassert>
#include "SoundDriverFactory.h"

bool XAudio2SoundDriver::ClassRegistered = SoundDriverFactory::RegisterSoundDriver(SND_DRIVER_XA2, CreateSoundDriver, "XAudio2 Driver", 15);

static IXAudio2* g_engine;
static IXAudio2SourceVoice* g_source;
static IXAudio2MasteringVoice* g_master;
static bool audioIsPlaying = false;
static bool canPlay = false;
static u8 bufferData[4][44100 * 4];
static int writeBuffer = 0;
static int readBuffer = 0;
static int filledBuffers;
static int bufferBytes;
static int cacheSize = 0;
static int interrupts = 0;
static VoiceCallback voiceCallback;
static HANDLE hMutex;

XAudio2SoundDriver::XAudio2SoundDriver()
{
    g_engine = nullptr;
    g_source = nullptr;
    g_master = nullptr;
    dllInitialized = false;
    bStopAudioThread = false;
    hAudioThread = nullptr;
    CoInitialize(nullptr);
}

XAudio2SoundDriver::~XAudio2SoundDriver()
{
    DeInitialize();
    CoUninitialize();
}

BOOL XAudio2SoundDriver::Initialize()
{
    if (g_source)
    {
        g_source->Start();
    }
    audioIsPlaying = false;
    writeBuffer = 0;
    readBuffer = 0;
    filledBuffers = 0;
    bufferBytes = 0;
    lastLength = 1;

    cacheSize = 0;
    interrupts = 0;
    return false;
}

BOOL XAudio2SoundDriver::Setup()
{
    if (dllInitialized == true)
        return true;

    dllInitialized = true;
    hAudioThread = nullptr;
    audioIsPlaying = false;
    writeBuffer = 0;
    readBuffer = 0;
    filledBuffers = 0;
    bufferBytes = 0;
    lastLength = 1;

    cacheSize = 0;
    interrupts = 0;

    hMutex = CreateMutex(nullptr, FALSE, nullptr);
    if (FAILED(XAudio2Create(&g_engine)))
    {
        return -1;
    }

    if (FAILED(g_engine->CreateMasteringVoice(&g_master)))
    {
        g_engine->Release();
        return -2;
    }
    canPlay = true;

    WAVEFORMATEX wfm = {0};
    wfm.wFormatTag = WAVE_FORMAT_PCM;
    wfm.nChannels = 2;
    wfm.nSamplesPerSec = 44100;
    wfm.wBitsPerSample = 16; // TODO: Allow 8bit audio...
    wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

    if (FAILED(g_engine->CreateSourceVoice(&g_source, &wfm, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, NULL, NULL)))
    {
        g_engine->Release();
        return -3;
    }

    g_source->Start();
    SetVolume(Configuration::getVolume());

    return FALSE;
}
void XAudio2SoundDriver::DeInitialize()
{
    Teardown();
}

void XAudio2SoundDriver::Teardown()
{
    if (dllInitialized == false)
        return;
    canPlay = false;
    StopAudioThread();
    if (hMutex != nullptr)
        WaitForSingleObject(hMutex, INFINITE);
    if (g_source != nullptr)
    {
        g_source->Stop();
        g_source->FlushSourceBuffers();
        if (hMutex != nullptr)
            ReleaseMutex(hMutex);
        g_source->DestroyVoice();
    }
    if (g_master != nullptr)
        g_master->DestroyVoice();
    if (g_engine != nullptr)
    {
        g_engine->StopEngine();
        g_engine->Release();
    }
    g_engine = nullptr;
    g_master = nullptr;
    g_source = nullptr;
    if (hMutex != nullptr)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    hMutex = nullptr;
    dllInitialized = false;
    canPlay = false;
}

void XAudio2SoundDriver::PlayBuffer(u8* data, int bufferSize)
{
    XAUDIO2_BUFFER xa2buff;

    xa2buff.Flags = XAUDIO2_END_OF_STREAM; // Suppress XAudio2 warnings
    xa2buff.PlayBegin = 0;
    xa2buff.PlayLength = 0;
    xa2buff.LoopBegin = 0;
    xa2buff.LoopLength = 0;
    xa2buff.LoopCount = 0;
    xa2buff.pContext = nullptr;
    xa2buff.AudioBytes = bufferSize;
    xa2buff.pAudioData = data;
    if (canPlay)
        g_source->SubmitSourceBuffer(&xa2buff);

    XAUDIO2_VOICE_STATE xvs;
    g_source->GetState(&xvs);

    assert(xvs.BuffersQueued < 5);
}

void XAudio2SoundDriver::SetFrequency(u32 Frequency)
{
    if (Setup() < 0) /* failed to apply a sound device */
        return;
    cacheSize = (u32)(Frequency / Configuration::getBackendFPS()) * 4;
    g_source->FlushSourceBuffers();
    g_source->SetSourceSampleRate(Frequency);

    StartAudioThread();
}

void XAudio2SoundDriver::AiUpdate(BOOL Wait)
{
    if (Wait)
        WaitMessage();
}

DWORD WINAPI XAudio2SoundDriver::AudioThreadProc(LPVOID lpParameter)
{
    XAudio2SoundDriver* driver = (XAudio2SoundDriver*)lpParameter;
    static int idx = 0; // TODO: This needs to be moved...

    while (driver->bStopAudioThread == false)
    {
        if (g_source)
        {
            XAUDIO2_VOICE_STATE xvs;
            g_source->GetState(&xvs);
            // # of BuffersQueued is a knob we can turn for latency vs buffering
            // 2 is minimum.  Maximum is the size of bufferData which is still TBD (Current 5)
            // It is always possible to new a buffer prior to submission then free it on completion.  Worth it?
            while (xvs.BuffersQueued < 3 && driver->bStopAudioThread == false) // Doubled this in hopes it would help... shouldn't cause too much additional latency
            {
                u32 len = driver->LoadAiBuffer(bufferData[idx], cacheSize);
                if (len > 0)
                {
                    driver->PlayBuffer(bufferData[idx], len);
                    idx = (idx + 1) % 4;
                }
                else
                {
                    if (Configuration::getDisallowSleepXA2() == false)
                        Sleep(0); // Give up timeslice - prevents a 2ms sleep potential
                }
                g_source->GetState(&xvs);
            }
        }

        if (!Configuration::getDisallowSleepXA2())
            Sleep(1);
    }
    return 0;
}

void XAudio2SoundDriver::StartAudioThread()
{
    if (!hAudioThread && dllInitialized == true)
    {
        DEBUG_OUTPUT "Audio Thread created\n";
        bStopAudioThread = false;
        hAudioThread = CreateThread(nullptr, 0, AudioThreadProc, this, 0, nullptr);
        assert(hAudioThread != NULL);
    }
}

void XAudio2SoundDriver::StopAudioThread()
{
    if (hAudioThread)
    {
        bStopAudioThread = true;
        DWORD result = WaitForSingleObject(hAudioThread, 5000);
        if (result != WAIT_OBJECT_0)
        {
            TerminateThread(hAudioThread, 0);
        }
        DEBUG_OUTPUT "Audio Thread terminated\n";
    }
    hAudioThread = nullptr;
    bStopAudioThread = false;
}

void XAudio2SoundDriver::StopAudio()
{
    audioIsPlaying = false;
    StopAudioThread();
    if (g_source)
    {
        g_source->Stop();
        g_source->FlushSourceBuffers();
    }
}

void XAudio2SoundDriver::StartAudio()
{
    audioIsPlaying = true;
    StartAudioThread();
}

void XAudio2SoundDriver::SetVolume(u32 volume)
{
    float xaVolume = 1.0f - (float)volume / 100.0f;
    if (g_source)
        g_source->SetVolume(xaVolume);
}


void __stdcall VoiceCallback::OnBufferEnd(void* pBufferContext)
{
}

void __stdcall VoiceCallback::OnVoiceProcessingPassStart(UINT32 SamplesRequired)
{
}
