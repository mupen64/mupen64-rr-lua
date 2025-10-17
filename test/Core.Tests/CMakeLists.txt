#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

block()
if (NOT BUILD_TESTING)
    return()
endif()

add_executable(Mupen64RR.Core.Tests
    "stdafx.h"
    "vcr_tests.cpp"
)
set_target_properties(Mupen64RR.Core.Tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "Core.Tests"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
)
target_precompile_headers(Mupen64RR.Core.Tests PRIVATE "stdafx.h")
target_link_libraries(Mupen64RR.Core.Tests PRIVATE
    Catch2::Catch2WithMain
    Mupen64RR.Core._TestIncludes
)
catch_discover_tests(Mupen64RR.Core.Tests)
endblock()