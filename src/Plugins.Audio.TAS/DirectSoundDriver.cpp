/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "common.h"
#include <stdio.h>
#include <cassert>
#include "DirectSoundDriver.h"
#include "SoundDriverFactory.h"

#define DS_SEGMENTS 4
#define LOCK_SIZE sLOCK_SIZE
#define TOTAL_SIZE (LOCK_SIZE * DS_SEGMENTS)
#define MAXBUFFER 27000
#define BUFFSIZE (writeLoc - readLoc)

bool DirectSoundDriver::ClassRegistered = SoundDriverFactory::RegisterSoundDriver(SND_DRIVER_DS8, CreateSoundDriver, "DirectSound 8 Driver", 20);

static DWORD sLOCK_SIZE;
static DWORD last_pos = 0, write_pos = 0, play_pos = 0, next_pos = 0;
static DWORD last_play = 0;
static LPVOID lpvPtr1, lpvPtr2;
static DWORD dwBytes1, dwBytes2;
static DWORD DMALen[3] = {0, 0, 0};
static BYTE* DMAData[3] = {NULL, NULL, NULL};

static LPDIRECTSOUNDBUFFER lpdsbuff = NULL;
static LPDIRECTSOUNDBUFFER lpdsb = NULL;

DWORD WINAPI AudioThreadProc(DirectSoundDriver* ac)
{
    ac->threadRunning = true;
    lpdsbuff = ac->lpdsbuf;

    while (lpdsbuff == NULL)
    {
        Sleep(1);
        if (ac->audioIsDone == true)
            goto _exit_;
    }
    DEBUG_OUTPUT "DS8: Audio Thread Started...\n";

    DWORD dwStatus;
    IDirectSoundBuffer_GetStatus(lpdsbuff, &dwStatus);
    if ((dwStatus & DSBSTATUS_PLAYING) == 0)
    {
        IDirectSoundBuffer_Play(lpdsbuff, 0, 0, 0);
    }

    SetThreadPriority(ac->handleAudioThread, THREAD_PRIORITY_HIGHEST);

    while (ac->audioIsDone == false)
    { // While the thread is still alive
        while (last_pos == write_pos)
        { // Cycle around until a new buffer position is available
            if (lpdsbuff == NULL)
                return 0;
            // Check to see if the audio pointer moved on to the next segment
            if (write_pos == last_pos)
            {
                if (Configuration::getDisallowSleepDS8() == false || write_pos == 0)
                    Sleep(1);
            }
            WaitForSingleObject(ac->hMutex, INFINITE);
            if FAILED (lpdsbuff->GetCurrentPosition((unsigned long*)&play_pos, NULL))
            {
                MessageBox(NULL, "Error getting audio position...", PLUGIN_FULL_NAME, MB_OK | MB_ICONSTOP);
                goto _exit_;
            }
            ReleaseMutex(ac->hMutex);
            // Determine our write position by where our play position resides
            if (play_pos < LOCK_SIZE)
                write_pos = LOCK_SIZE * DS_SEGMENTS - LOCK_SIZE;
            else
                write_pos = play_pos / LOCK_SIZE * LOCK_SIZE - LOCK_SIZE;
            last_play = play_pos;
        }
        // This means we had a buffer segment skipped skip
        if (next_pos != write_pos)
        {
            DEBUG_OUTPUT "A";
        }

        // Store our last position
        last_pos = write_pos;

        // Set out next anticipated segment
        next_pos = write_pos + LOCK_SIZE;
        if (next_pos >= LOCK_SIZE * DS_SEGMENTS)
        {
            next_pos -= LOCK_SIZE * DS_SEGMENTS;
        }
        if (ac->audioIsDone == true)
            break;

        // Fill queue buffer here with LOCK_SIZE
        // TODO: Add buffer processing command here....
        // Time to write out to the buffer
        WaitForSingleObject(ac->hMutex, INFINITE);
        if (DS_OK != lpdsbuff->Lock(write_pos, LOCK_SIZE, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0))
        {
            MessageBox(NULL, "Error locking sound buffer", PLUGIN_FULL_NAME, MB_OK | MB_ICONSTOP);
            goto _exit_;
        }

        ac->LoadAiBuffer((BYTE*)lpvPtr1, dwBytes1);
        if (dwBytes2)
        {
            ac->LoadAiBuffer((BYTE*)lpvPtr2, dwBytes2);
            DEBUG_OUTPUT "P";
        }

        if FAILED (lpdsbuff->Unlock(lpvPtr1, dwBytes1, lpvPtr2, dwBytes2))
        {
            MessageBox(NULL, "Error unlocking sound buffer", PLUGIN_FULL_NAME, MB_OK | MB_ICONSTOP);
            goto _exit_;
        }

        ReleaseMutex(ac->hMutex);
    }

_exit_:
    DEBUG_OUTPUT "DS8: Audio Thread Terminated...\n";
    ReleaseMutex(ac->hMutex);
    ac->threadRunning = false;
}


//------------------------------------------------------------------------

// Setup and Teardown Functions

// Generates nice alignment with N64 samples...
void DirectSoundDriver::SetSegmentSize(DWORD length)
{
    if (SampleRate == 0)
    {
        return;
    }
    SegmentSize = length;

    WaitForSingleObject(hMutex, INFINITE);

    WAVEFORMATEX wfm{};
    memset(&wfm, 0, sizeof(WAVEFORMATEX));
    wfm.wFormatTag = WAVE_FORMAT_PCM;
    wfm.nChannels = 2;
    wfm.nSamplesPerSec = SampleRate;
    wfm.wBitsPerSample = 16;
    wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

    DSBUFFERDESC dsbdesc{};
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE;
    dsbdesc.dwBufferBytes = SegmentSize * DS_SEGMENTS;
    dsbdesc.lpwfxFormat = &wfm;

    const HRESULT hr = IDirectSound_CreateSoundBuffer(lpds, &dsbdesc, &lpdsbuf, NULL);
    assert(!FAILED(hr));

    IDirectSoundBuffer_Play(lpdsbuf, 0, 0, DSBPLAY_LOOPING);
    lpdsbuff = this->lpdsbuf;
    ReleaseMutex(hMutex);
}

// TODO: Should clear out AI registers on romopen and initialize
BOOL DirectSoundDriver::Initialize()
{
    this->threadRunning = false;
    DeInitialize();
    SampleRate = 0;

    DEBUG_OUTPUT "DS8: Initialize()\n";
    hMutex = CreateMutex(NULL, FALSE, NULL);

    WaitForSingleObject(hMutex, INFINITE);

    HRESULT hr = DirectSoundCreate8(NULL, &lpds, NULL);

    if (FAILED(hr))
    {
        DEBUG_OUTPUT "DS8: Unable to DirectSoundCreate\n";
        return -2;
    }

    if (FAILED(hr = IDirectSound_SetCooperativeLevel(lpds, (HWND)AudioInfo.main_hwnd, DSSCL_PRIORITY)))
    {
        DEBUG_OUTPUT "DS8: Failed to SetCooperativeLevel\n";
        return -1;
    }

    if (lpdsbuf)
    {
        IDirectSoundBuffer_Release(lpdsbuf);
        lpdsbuf = NULL;
    }

    DSBUFFERDESC ds_primary_buff{};
    memset(&ds_primary_buff, 0, sizeof(DSBUFFERDESC));
    ds_primary_buff.dwSize = sizeof(DSBUFFERDESC);
    ds_primary_buff.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    ds_primary_buff.dwBufferBytes = 0;
    ds_primary_buff.lpwfxFormat = NULL;

    WAVEFORMATEX wfm{};
    memset(&wfm, 0, sizeof(WAVEFORMATEX));
    wfm.wFormatTag = WAVE_FORMAT_PCM;
    wfm.nChannels = 2;
    wfm.nSamplesPerSec = 44100;
    wfm.wBitsPerSample = 16;
    wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

    hr = IDirectSound_CreateSoundBuffer(lpds, &ds_primary_buff, &lpdsb, NULL);

    if (SUCCEEDED(hr))
    {
        // IDirectSoundBuffer_SetFormat(lpdsb, &wfm);
        IDirectSoundBuffer_Play(lpdsb, 0, 0, DSBPLAY_LOOPING);
    }

    ReleaseMutex(hMutex);

    if (SampleRate > 0)
        SetFrequency(SampleRate);

    DEBUG_OUTPUT "DS8: Init Success...\n";

    DMALen[0] = DMALen[1] = 0;
    DMAData[0] = DMAData[1] = NULL;
    return FALSE;
}

void DirectSoundDriver::DeInitialize()
{
    DEBUG_OUTPUT "DS8: DeInitialize()\n";
    audioIsDone = true;
    StopAudio();
    if (lpdsbuf)
    {
        IDirectSoundBuffer_Stop(lpdsbuf);
        IDirectSoundBuffer_Release(lpdsbuf);
        lpdsbuf = NULL;
    }
    if (lpds)
    {
        IDirectSound_Release(lpds);
        lpds = NULL;
    }
    if (hMutex)
    {
        CloseHandle(hMutex);
        hMutex = NULL;
    }

    lpdsbuf = NULL;
    lpds = NULL;
    audioIsDone = false;
    hMutex = NULL;
    audioIsPlaying = FALSE;
    readLoc = writeLoc = remainingBytes = 0;
    DMALen[0] = DMALen[1] = 0;
    DMAData[0] = DMAData[1] = NULL;
    DEBUG_OUTPUT "DS8: DeInitialize() complete\n";
}

// ---------BLAH--------

// Buffer Functions for the Audio Code
void DirectSoundDriver::SetFrequency(u32 Frequency2)
{
    DWORD Frequency = Frequency2;

    printf("DS8: SetFrequency()\n");
    StopAudio();

    sLOCK_SIZE = (u32)(Frequency / Configuration::getBackendFPS()) * 4; //(Frequency / 80) * 4;// 0x600;// (22050 / 30) * 4;// 0x4000;// (Frequency / 60) * 4;
    SampleRate = Frequency;
    SegmentSize = 0; // Trash it... we need to redo the Frequency anyway...
    SetSegmentSize(LOCK_SIZE);
    DEBUG_OUTPUT "DS8: Frequency: %li - SegmentSize: %li\n", Frequency, SegmentSize;
    lastLength = 0;
    writeLoc = 0x0000;
    readLoc = 0x0000;
    remainingBytes = 0;
    StartAudio();
    DEBUG_OUTPUT "DS8: SetFrequency() Complete\n";
}

void DirectSoundDriver::AiUpdate(BOOL Wait)
{
    if (Wait)
        WaitMessage();
}

// Management functions
// TODO: For silent emulation... the Audio should still be "processed" somehow...
void DirectSoundDriver::StopAudio()
{
    if (!audioIsPlaying)
        return;
    DEBUG_OUTPUT "DS8: StopAudio()\n";
    if (lpdsbuf != NULL)
    {
        lpdsbuf->Stop();
    }
    audioIsPlaying = FALSE;
    audioIsDone = true;
    if (this->handleAudioThread != NULL)
    {
        if (WaitForSingleObjectEx(this->handleAudioThread, 1000, false) != WAIT_OBJECT_0)
        {
            DEBUG_OUTPUT "DS8: Unsafe Thread Termination\n";
            TerminateThread(this->handleAudioThread, 0);
        }
        this->threadRunning = false;
    }
    this->handleAudioThread = NULL;
    DEBUG_OUTPUT "DS8: StopAudio() complete\n";
}

void DirectSoundDriver::StartAudio()
{
    if (audioIsPlaying)
        return;
    DEBUG_OUTPUT "DS8: StartAudio()\n";
    audioIsPlaying = TRUE;
    audioIsDone = false;
    writeLoc = 0x0000;
    readLoc = 0x0000;
    remainingBytes = 0;
    if (this->handleAudioThread != NULL)
    {
        DEBUG_OUTPUT "Audiothread != NULL";
        assert(0);
    }
    else
    {
        this->handleAudioThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AudioThreadProc, this, 0, &this->dwAudioThreadId);
    }
    if (lpdsbuf != NULL)
    {
        IDirectSoundBuffer_Play(lpdsbuf, 0, 0, DSBPLAY_LOOPING);
    }
    DEBUG_OUTPUT "DS8: StartAudio() Complete\n";
}

void DirectSoundDriver::SetVolume(u32 volume)
{
    DWORD ds_volume = (DWORD)volume * -25;
    if (volume == 100)
        ds_volume = (DWORD)DSBVOLUME_MIN;
    if (volume == 0)
        ds_volume = DSBVOLUME_MAX;
    if (lpdsb != NULL)
        lpdsb->SetVolume(ds_volume);
}
