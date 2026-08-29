#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR_Plugins_DummyVideo STATIC
    "Main.cpp"
)
target_compile_features(Mupen64RR_Plugins_DummyVideo PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Plugins_DummyVideo PROPERTIES
    OUTPUT_NAME "NoVideo"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
mupen64rr_reuse_common_pch(Mupen64RR_Plugins_DummyVideo)
target_link_libraries(Mupen64RR_Plugins_DummyVideo PRIVATE Mupen64RR_Plugins_Common)
target_compile_definitions(Mupen64RR_Plugins_DummyVideo PRIVATE
    M64RRGetMetadata=M64RRBuiltinNoVideoGetMetadata
    M64RRProcessEvent=M64RRBuiltinNoVideoProcessEvent
)
