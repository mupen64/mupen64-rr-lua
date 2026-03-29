#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Win32.SDLAudio MODULE
    "Config.hpp"
    "Main.cpp"
    "Main.hpp"
    "Main_Win32.cpp"
    "Main_Win32.hpp"
    "SDLBackend.cpp"
    "SDLBackend.hpp"
)
set_target_properties(Mupen64RR.Plugins.Win32.SDLAudio PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "sdl-audio"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(Mupen64RR.Plugins.Win32.SDLAudio PRIVATE 
    Mupen64RR.Plugins.Win32.Common
    SDL3::SDL3
)