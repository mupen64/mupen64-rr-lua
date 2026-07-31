#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

block()
if (NOT BUILD_TESTING)
    return()
endif()

add_executable(Mupen64RR.Core.Tests
    "stdafx.h"
    "IOUtilsTests.cpp"
    "VCRTests.cpp"
    "VRTests.cpp"
)
mupen64rr_copy_vcpkg_deps(Mupen64RR.Core.Tests)
target_compile_features(Mupen64RR.Core.Tests PRIVATE cxx_std_23)
set_target_properties(Mupen64RR.Core.Tests PROPERTIES
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
