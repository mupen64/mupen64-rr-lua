#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

# PortAudio driver for N64 emulation
add_library(Mupen64RR.Plugins.Qt6.Pan64 MODULE
    "Config.hpp"
    "Resampler.hpp"
    "SampleBuffer.hpp"
    "SampleBuffer.cpp"
    "Main.cpp"
    "Main.hpp"
)
set_target_properties(Mupen64RR.Plugins.Qt6.Pan64 PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "audio-pan64"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_compile_definitions(Mupen64RR.Plugins.Qt6.Pan64 PRIVATE "MUPAPI_PLUGIN_DECLS")
target_link_libraries(Mupen64RR.Plugins.Qt6.Pan64 PRIVATE 
    Mupen64RR.Views.Qt6.Headers
    SampleRate::samplerate
    PkgConfig::portaudiocpp
)