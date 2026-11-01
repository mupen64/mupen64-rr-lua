/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "BuiltinTAS.hpp"

namespace BuiltinTAS
{
static std::array<StrUtils::unordered_string_map<void *>, (size_t)PluginID::NumPlugins> g_builtin_symbols = {
    // DummyVideo
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)DummyVideo::M64RRGetMetadata},
    },
    // DummyAudio
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)DummyAudio::M64RRGetMetadata},
    },
    // DummyInput
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)DummyInput::M64RRGetMetadata},
        {"M64RRProcessEvent", (void *)DummyInput::M64RRProcessEvent},
    },
    // TASVideo
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)TASVideo::M64RRGetMetadata},
        {"M64RRProcessEvent", (void *)TASVideo::M64RRProcessEvent},
        {"M64RRProcessDList", (void *)TASVideo::M64RRProcessDList},
        {"M64RRReadVideo", (void *)TASVideo::M64RRReadVideo},
        {"M64RRShowConfig", (void *)TASVideo::M64RRShowConfig},
    },
    // TASAudio
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)TASAudio::M64RRGetMetadata},
        {"M64RRProcessEvent", (void *)TASAudio::M64RRProcessEvent},
        {"M64RRAIDacrateChanged", (void *)TASAudio::M64RRAIDacrateChanged},
        {"M64RRAILenChanged", (void *)TASAudio::M64RRAILenChanged},
    },
    // TASInput
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)TASInput_Qt6::M64RRGetMetadata},
        {"M64RRProcessEvent", (void *)TASInput_Qt6::M64RRProcessEvent},
        {"M64RRReadController", (void *)TASInput_Qt6::M64RRReadController},
        {"M64RRGetKeys", (void *)TASInput_Qt6::M64RRGetKeys},
        {"M64RRSetKeys", (void *)TASInput_Qt6::M64RRSetKeys},
    },
    // TASRSP
    StrUtils::unordered_string_map<void *>{
        {"M64RRGetMetadata", (void *)TASRSP::M64RRGetMetadata},
        {"M64RRProcessEvent", (void *)TASRSP::M64RRProcessEvent},
        {"M64RRDoRSPCycles", (void *)TASRSP::M64RRDoRSPCycles},
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
