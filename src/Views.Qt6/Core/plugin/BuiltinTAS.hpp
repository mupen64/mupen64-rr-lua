/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <Core/Plugin.hpp>

namespace BuiltinTAS
{
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

namespace TASRSP
{
void M64RRGetMetadata(M64RRSpec::PluginMetadata *);
void M64RRProcessEvent(M64RRSpec::Event);
uint32_t M64RRDoRSPCycles(uint32_t);
} // namespace TASRSP

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
