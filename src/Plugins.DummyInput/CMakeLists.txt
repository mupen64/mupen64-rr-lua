#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR_Plugins_DummyInput STATIC
    "Main.cpp"
)
target_compile_features(Mupen64RR_Plugins_DummyInput PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Plugins_DummyInput PROPERTIES
    OUTPUT_NAME "NoInput"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
mupen64rr_reuse_common_pch(Mupen64RR_Plugins_DummyInput)
target_link_libraries(Mupen64RR_Plugins_DummyInput PRIVATE Mupen64RR_Plugins_Common)
target_compile_definitions(Mupen64RR_Plugins_DummyInput PRIVATE
    M64RRGetMetadata=M64RRBuiltinNoInputGetMetadata
    M64RRProcessEvent=M64RRBuiltinNoInputProcessEvent
)
