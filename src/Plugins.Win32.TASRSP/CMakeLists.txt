#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Win32.TASRSP MODULE
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

target_compile_features(Mupen64RR.Plugins.Win32.TASRSP PRIVATE cxx_std_23)
set_target_properties(Mupen64RR.Plugins.Win32.TASRSP PROPERTIES
    OUTPUT_NAME "TASRSP"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.Win32.TASRSP PRIVATE ".")
target_link_libraries(Mupen64RR.Plugins.Win32.TASRSP PRIVATE
    Mupen64RR.Plugins.Win32.Common
    nlohmann_json::nlohmann_json
)
