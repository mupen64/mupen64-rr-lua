#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Win32.TASAudio MODULE
    "Config.cpp"
    "Config_Win32.cpp"
    "Main.cpp"
    "Main_Win32.cpp"
    "SDLBackend.cpp"
    
    "Config.hpp"
    "Config_Win32.hpp"
    "Main.hpp"
    "Main_Win32.hpp"
    "SDLBackend.hpp"

    "Resource.h"
    "Resource.rc"
)
set_target_properties(Mupen64RR.Plugins.Win32.TASAudio PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "TASAudio"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(Mupen64RR.Plugins.Win32.TASAudio PRIVATE
    Mupen64RR.Plugins.Win32.Common
    SDL3::SDL3
    nlohmann_json::nlohmann_json
)