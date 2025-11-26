/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <cstdio>

typedef struct
{
    // RIFF Header
    char ChunkID[4]; // "RIFF"
    unsigned long ChunkSize; // FileSize - 8
    char format[4]; // "WAVE"
    // fmt Header
    char Subchunk1ID[4]; // "fmt "
    unsigned long Subchunk1Size; // 16 for PCM
    unsigned short AudioFormat; // PCM=1
    unsigned short NumChannels; // Mono=1, Stereo=2
    unsigned long SampleRate; // 8000, 44100, etc.
    unsigned long ByteRate; // SampleRate*NumChannels*BitsPerSample/8
    unsigned short BlockAlign; // NumChannels * BitsPerSample/8
    unsigned short BitsPerSample; // 8bits = 8, 16 bits = 16
    // Data Header
    char Subchunk2ID[4]; // "data"
    unsigned long Subchunk2Size; // NumSamples * NumChannels * BitsPerSample/8

    // Everything after is Data

} WaveHeader;

class WaveOut {
protected:
    WaveHeader header;
    FILE* waveoutput;
    unsigned long datasize;

public:
    WaveOut();
    void BeginWaveOut(char* filename, unsigned short channels, unsigned short bitsPerSample, unsigned long sampleRate);
    void EndWaveOut();
    void WriteData(unsigned char* data, unsigned long size);
};
