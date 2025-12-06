#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Qt6.AudioSDL MODULE
    "resamplers/Swr.cpp"
    "resamplers/Swr.hpp"

    "BufferUtils.hpp"
    "Config.hpp"
    "Main.cpp"
    "Main.hpp"
    "Resamplers.hpp"
    "SDLBackend.cpp"
    "SDLBackend.hpp"
)
set_target_properties(Mupen64RR.Plugins.Qt6.AudioSDL PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "audio-sdl"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_compile_definitions(Mupen64RR.Plugins.Qt6.AudioSDL PRIVATE "MUPAPI_PLUGIN_DECLS")
target_link_libraries(Mupen64RR.Plugins.Qt6.AudioSDL PRIVATE 
    Mupen64RR.Views.Qt6.Headers
    SDL2::SDL2
    PkgConfig::libswresample
)