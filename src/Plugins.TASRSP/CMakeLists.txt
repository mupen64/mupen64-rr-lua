#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.TASRSP STATIC
    "Config.hpp"
    "HLE.hpp"
    "Main.hpp"

    "Config.cpp"
    "JPEG.cpp"
    "Main.cpp"
    "MP3.cpp"
    "UCode1.cpp"
    "UCode2.cpp"
    "UCode3.cpp"
)

target_compile_features(Mupen64RR.Plugins.TASRSP PRIVATE cxx_std_26)
set_target_properties(Mupen64RR.Plugins.TASRSP PROPERTIES
    OUTPUT_NAME "TASRSP"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.TASRSP PRIVATE ".")
target_link_libraries(Mupen64RR.Plugins.TASRSP PRIVATE
    Mupen64RR.Plugins.Common
    nlohmann_json::nlohmann_json
)

target_compile_definitions(Mupen64RR.Plugins.TASRSP PRIVATE
    M64RRGetMetadata=M64RRBuiltinTASRSPGetMetadata
    M64RRProcessEvent=M64RRBuiltinTASRSPProcessEvent
    M64RRDoRSPCycles=M64RRBuiltinTASRSPDoRSPCycles
    g_plugin=M64RRBuiltinTASRSPPlugin
)
