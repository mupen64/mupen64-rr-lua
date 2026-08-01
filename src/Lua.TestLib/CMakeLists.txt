#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Lua.TestLib MODULE "main.cpp")
set_target_properties(Mupen64RR.Lua.TestLib PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "luatestlib"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
)
target_link_libraries(Mupen64RR.Lua.TestLib PRIVATE Lua::Lua)