#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Qt6.NoRSP MODULE
    "Main.cpp"
    "Main.h"
)
set_target_properties(Mupen64RR.Plugins.Qt6.NoRSP PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "no-rsp"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(Mupen64RR.Plugins.Qt6.NoRSP PRIVATE Mupen64RR.Views.Qt6.Headers)
target_compile_definitions(Mupen64RR.Plugins.Qt6.NoRSP PRIVATE "MUPAPI_PLUGIN_DECLS")