#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Win32.TASInput STATIC
    "TASInput.hpp"
    "Common.hpp"
    "Resource.h"
    "NewConfig.hpp"
    "Helpers.hpp"
    "Main.hpp"
    "GamepadManager.hpp"
    "ConfigDialog.hpp"
    "Combo.hpp"

    "TASInput.cpp"
    "NewConfig.cpp"
    "Main.cpp"
    "GamepadManager.cpp"
    "ConfigDialog.cpp"
    "Combo.cpp"

    "Resource.rc"
)
target_compile_features(Mupen64RR.Plugins.Win32.TASInput PRIVATE cxx_std_26)
set_target_properties(Mupen64RR.Plugins.Win32.TASInput PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "TASInput"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.Win32.TASInput PRIVATE ".")
target_link_libraries(Mupen64RR.Plugins.Win32.TASInput PRIVATE
    Mupen64RR.Plugins.Win32.Common
    SDL3::SDL3
    nlohmann_json::nlohmann_json
)
target_link_libraries(Mupen64RR.Plugins.Win32.TASInput PRIVATE
    setupapi
    hid
    imm32
    version
    winmm
    comctl32
    shcore
    gdiplus
)

target_compile_definitions(Mupen64RR.Plugins.Win32.TASInput PRIVATE
    M64RRGetMetadata=M64RRBuiltinTASInputGetMetadata
    M64RRProcessEvent=M64RRBuiltinTASInputProcessEvent
    M64RRReadController=M64RRBuiltinTASInputReadController
    M64RRShowConfig=M64RRBuiltinTASInputShowConfig
    M64RRGetKeys=M64RRBuiltinTASInputGetKeys
    M64RRSetKeys=M64RRBuiltinTASInputSetKeys
    g_plugin=M64RRBuiltinTASInputPlugin
    DllMain=M64RRBuiltinTASInputDllMain
)
