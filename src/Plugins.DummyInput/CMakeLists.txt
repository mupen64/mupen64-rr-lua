#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.DummyInput MODULE
    "Main.cpp"
)
target_compile_features(Mupen64RR.Plugins.DummyInput PRIVATE cxx_std_23)
set_target_properties(Mupen64RR.Plugins.DummyInput PROPERTIES
    OUTPUT_NAME "NoInput"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(Mupen64RR.Plugins.DummyInput PRIVATE Mupen64RR.Plugins.Common)
