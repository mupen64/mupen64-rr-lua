#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR_Lua_TestLib MODULE "main.cpp")
mupen64rr_reuse_common_pch(Mupen64RR_Lua_TestLib)
target_compile_features(Mupen64RR_Lua_TestLib PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Lua_TestLib PROPERTIES
    OUTPUT_NAME "luatestlib"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
)
target_link_libraries(Mupen64RR_Lua_TestLib PRIVATE
    Lua::Lua
    Mupen64RR_Common
)
