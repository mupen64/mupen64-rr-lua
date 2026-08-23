/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <m64rr/Plugin.hpp>

namespace BuiltinTAS
{
extern "C"
{
    void M64RRBuiltinTASAudioGetMetadata(M64RRSpec::PluginMetadata *);
    void M64RRBuiltinTASAudioProcessEvent(M64RRSpec::Event);
    void M64RRBuiltinTASAudioAIDacrateChanged(CoreSystemType);
    void M64RRBuiltinTASAudioAILenChanged();
    void M64RRBuiltinTASAudioShowConfig(M64RRSpec::WindowHandle);

    void M64RRBuiltinTASRSPGetMetadata(M64RRSpec::PluginMetadata *);
    void M64RRBuiltinTASRSPProcessEvent(M64RRSpec::Event);
    uint32_t M64RRBuiltinTASRSPDoRSPCycles(uint32_t);

    void M64RRBuiltinTASVideoGetMetadata(M64RRSpec::PluginMetadata *);
    void M64RRBuiltinTASVideoProcessEvent(M64RRSpec::Event);
    void M64RRBuiltinTASVideoProcessDList();
    void M64RRBuiltinTASVideoReadVideo(void *, int32_t *, int32_t *);
    void M64RRBuiltinTASVideoShowConfig(M64RRSpec::WindowHandle);

    // void M64RRBuiltinTASInputGetMetadata(M64RRSpec::PluginMetadata *);
    // void M64RRBuiltinTASInputProcessEvent(M64RRSpec::Event);
    // void M64RRBuiltinTASInputReadController(int32_t, unsigned char *);
    // void M64RRBuiltinTASInputShowConfig(M64RRSpec::WindowHandle);
    // void M64RRBuiltinTASInputGetKeys(int32_t, CoreButtons *);
    // void M64RRBuiltinTASInputSetKeys(int32_t, CoreButtons);

    void M64RRBuiltinNoAudioGetMetadata(M64RRSpec::PluginMetadata *);
    void M64RRBuiltinNoInputGetMetadata(M64RRSpec::PluginMetadata *);
    void M64RRBuiltinNoInputProcessEvent(M64RRSpec::Event);
    void M64RRBuiltinNoVideoGetMetadata(M64RRSpec::PluginMetadata *);
}

enum class PluginID : uint8_t
{
    DummyVideo = 0,
    DummyAudio,
    DummyInput,
    TASVideo,
    TASAudio,
    TASInput,
    TASRSP,
    NumPlugins,
};

void *builtin_dlsym(PluginID id, const char *symbol);

} // namespace BuiltinTAS
