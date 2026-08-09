#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

block()
if (NOT BUILD_TESTING)
    return()
endif()

add_executable(Mupen64RR.Core.Tests
    "Common.hpp"

    "PluginAPITests.cpp"
    "IOUtilsTests.cpp"
    "VCRTests.cpp"
    "VRTests.cpp"
)
mupen64rr_copy_vcpkg_deps(Mupen64RR.Core.Tests)
target_compile_features(Mupen64RR.Core.Tests PRIVATE cxx_std_26)
set_target_properties(Mupen64RR.Core.Tests PROPERTIES
    OUTPUT_NAME "Core.Tests"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
)
target_link_libraries(Mupen64RR.Core.Tests PRIVATE
    Catch2::Catch2WithMain
    Mupen64RR.Core._TestIncludes
)

# Skip running tests when cross-compiling between OSes.
if("${CMAKE_SYSTEM_NAME}" STREQUAL "${CMAKE_HOST_SYSTEM_NAME}")
    catch_discover_tests(Mupen64RR.Core.Tests)
else()
    message(STATUS "Cross-compiling: skipping test discovery for Mupen64RR.Core.Tests (run manually on target)")
endif()
endblock()
