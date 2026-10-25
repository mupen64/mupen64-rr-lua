/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <Core/Plugin.hpp>

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

    void M64RRBuiltinTASInputGetMetadata(M64RRSpec::PluginMetadata *);
    void M64RRBuiltinTASInputProcessEvent(M64RRSpec::Event);
    void M64RRBuiltinTASInputReadController(int32_t, unsigned char *);
    void M64RRBuiltinTASInputShowConfig(M64RRSpec::WindowHandle);
    void M64RRBuiltinTASInputGetKeys(int32_t, CoreButtons *);
    void M64RRBuiltinTASInputSetKeys(int32_t, CoreButtons);

    void M64RRBuiltinNoAudioGetMetadata(M64RRSpec::PluginMetadata *);
    void M64RRBuiltinNoInputGetMetadata(M64RRSpec::PluginMetadata *);
    void M64RRBuiltinNoInputProcessEvent(M64RRSpec::Event);
    void M64RRBuiltinNoVideoGetMetadata(M64RRSpec::PluginMetadata *);
}

namespace DummyVideo
{
void M64RRGetMetadata(M64RRSpec::PluginMetadata *);
}

namespace DummyInput
{
void M64RRGetMetadata(M64RRSpec::PluginMetadata *);
void M64RRProcessEvent(M64RRSpec::Event);
} // namespace DummyInput

namespace DummyAudio
{
void M64RRGetMetadata(M64RRSpec::PluginMetadata *);
}

namespace TASVideo
{
void M64RRGetMetadata(M64RRSpec::PluginMetadata *);
void M64RRProcessEvent(M64RRSpec::Event);
void M64RRProcessDList();
void M64RRReadVideo(void *, int32_t *, int32_t *);
void M64RRShowConfig(M64RRSpec::WindowHandle);
} // namespace TASVideo

namespace TASAudio
{
void M64RRGetMetadata(M64RRSpec::PluginMetadata *);
void M64RRProcessEvent(M64RRSpec::Event);
void M64RRAIDacrateChanged(CoreSystemType);
void M64RRAILenChanged();
void M64RRShowConfig(M64RRSpec::WindowHandle);
} // namespace TASAudio

namespace TASInput
{
void M64RRGetMetadata(M64RRSpec::PluginMetadata *);
void M64RRProcessEvent(M64RRSpec::Event);
void M64RRReadController(int32_t, unsigned char *);
void M64RRShowConfig(M64RRSpec::WindowHandle);
void M64RRGetKeys(int32_t, CoreButtons *);
void M64RRSetKeys(int32_t, CoreButtons);
} // namespace TASInput

namespace TASRSP
{
void M64RRGetMetadata(M64RRSpec::PluginMetadata *);
void M64RRProcessEvent(M64RRSpec::Event);
uint32_t M64RRDoRSPCycles(uint32_t);
} // namespace TASRSP
} // namespace BuiltinTAS
