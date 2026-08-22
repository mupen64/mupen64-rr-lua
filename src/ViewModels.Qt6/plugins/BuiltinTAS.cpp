/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "ViewModels.Qt6/plugins/BuiltinTAS.hpp"

#include <StrUtils.hpp>

namespace BuiltinTAS
{
static std::array<StrUtils::unordered_string_map<void *>, (size_t)PluginID::NumPlugins> g_builtin_symbols = {
    // DummyVideo
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)M64RRBuiltinNoVideoGetMetadata},
    },
    // DummyAudio
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)M64RRBuiltinNoAudioGetMetadata},
    },
    // DummyInput
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)M64RRBuiltinNoInputGetMetadata},
        {"M64RRProcessEvent", (void *)M64RRBuiltinNoInputProcessEvent},
    },
    // TASVideo
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void*) M64RRBuiltinTASVideoGetMetadata},
        {"M64RRProcessEvent", (void*) M64RRBuiltinTASVideoProcessEvent},
        {"M64RRProcessDList", (void*) M64RRBuiltinTASVideoProcessDList},
        {"M64RRReadVideo", (void*) M64RRBuiltinTASVideoReadVideo},
        {"M64RRShowConfig", (void*) M64RRBuiltinTASVideoShowConfig},
    },
    // TASAudio
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)M64RRBuiltinTASAudioGetMetadata},
        {"M64RRProcessEvent", (void *)M64RRBuiltinTASAudioProcessEvent},
        {"M64RRAIDacrateChanged", (void *)M64RRBuiltinTASAudioAIDacrateChanged},
        {"M64RRAILenChanged", (void *)M64RRBuiltinTASAudioAILenChanged},
    },
    // TASInput
    StrUtils::unordered_string_map<void *>{
        // {"M64RRGetMetadata", (void*) M64RRBuiltinNoVideoGetMetadata},
    },
    // TASRSP
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)M64RRBuiltinTASRSPGetMetadata},
        {"M64RRProcessEvent", (void *)M64RRBuiltinTASRSPProcessEvent},
        {"M64RRDoRSPCycles", (void *)M64RRBuiltinTASRSPDoRSPCycles},
    },
};

void *builtin_dlsym(PluginID id, const char *symbol)
{
    if ((uint8_t)id > (uint8_t)PluginID::NumPlugins) throw std::logic_error("Invalid plugin ID");

    auto &table = g_builtin_symbols[(uint8_t)id];
    auto result = table.find(symbol);
    return (result != table.end()) ? result->second : nullptr;
}
} // namespace BuiltinTAS