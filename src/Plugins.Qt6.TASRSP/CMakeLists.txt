#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Qt6.TASRSP MODULE
    "Main.h"
    "HLE.h"
    "Disasm.h"

    "JPEG.cpp"
    "Main.cpp"
    "Disasm.cpp"
    "UCode1.cpp"
    "UCode2.cpp"
    "UCode3.cpp"
    "MP3.cpp"
)
set_target_properties(Mupen64RR.Plugins.Qt6.TASRSP PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "TASRSP"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.Qt6.TASRSP PRIVATE ".")
target_compile_definitions(Mupen64RR.Plugins.Qt6.TASRSP PRIVATE "MUPAPI_PLUGIN_DECLS")
target_link_libraries(Mupen64RR.Plugins.Qt6.TASRSP PRIVATE Mupen64RR.Views.Qt6.Headers)
