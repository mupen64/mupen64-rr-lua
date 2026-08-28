#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]



add_library(Mupen64RR_Plugins_TASAudio STATIC
    "Config.cpp"
    "Main.cpp"
    "SDLBackend.cpp"

    "Config.hpp"
    "Main.hpp"
    "SDLBackend.hpp"
)
# Conditional changes on Win32
if (MUPEN64RR_BUILD_WIN32)
    # bundle Win32 sources
    target_sources(Mupen64RR_Plugins_TASAudio PRIVATE
        "Config_Win32.hpp"
        "Main_Win32.hpp"
        "Resource.h"

        "Config_Win32.cpp"
        "Main_Win32.cpp"
        "Resource.rc"
    )
    # link Win32 plugin libraries
    target_link_libraries(Mupen64RR_Plugins_TASAudio PRIVATE Mupen64RR_Plugins_Win32_Common)
else()
    # link cross-platform plugin libraries
    target_link_libraries(Mupen64RR_Plugins_TASAudio PRIVATE Mupen64RR_Plugins_Common)
endif()
target_link_libraries(Mupen64RR_Plugins_TASAudio PRIVATE
    SDL3::SDL3
    nlohmann_json::nlohmann_json
)

mupen64rr_reuse_common_pch(Mupen64RR_Plugins_TASAudio)
target_compile_features(Mupen64RR_Plugins_TASAudio PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Plugins_TASAudio PROPERTIES
    OUTPUT_NAME "TASAudio"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_compile_definitions(NOMINMAX)

target_compile_definitions(Mupen64RR_Plugins_TASAudio PRIVATE
    M64RRGetMetadata=M64RRBuiltinTASAudioGetMetadata
    M64RRProcessEvent=M64RRBuiltinTASAudioProcessEvent
    M64RRAIDacrateChanged=M64RRBuiltinTASAudioAIDacrateChanged
    M64RRAILenChanged=M64RRBuiltinTASAudioAILenChanged
    M64RRShowConfig=M64RRBuiltinTASAudioShowConfig
    g_plugin=M64RRBuiltinTASAudioPlugin
)
