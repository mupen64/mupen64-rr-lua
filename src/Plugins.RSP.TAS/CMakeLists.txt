#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.RSP.TAS MODULE
    "Main.h"
    "HLE.h"
    "Disasm.h"
    "Config.h"
    "Resource.h"

    "JPEG.cpp"
    "Main.cpp"
    "Config.cpp"
    "Disasm.cpp"
    "UCode1.cpp"
    "UCode2.cpp"
    "UCode3.cpp"
    "MP3.cpp"

    "Resource.rc"
)
set_target_properties(Mupen64RR.Plugins.RSP.TAS PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "TASRSP"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.RSP.TAS PRIVATE ".")
target_link_libraries(Mupen64RR.Plugins.RSP.TAS PRIVATE Mupen64RR.Plugins.Win32.Common)
target_link_libraries(Mupen64RR.Plugins.RSP.TAS PRIVATE
    Comctl32
    uxtheme
    Msimg32
    Gdiplus
)