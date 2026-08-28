#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

block()
if (NOT BUILD_TESTING)
    return()
endif()

add_executable(Mupen64RR_Core_Tests
    "Common.hpp"

    "PluginAPITests.cpp"
    "IOUtilsTests.cpp"
    "VCRTests.cpp"
    "VRTests.cpp"
)
mupen64rr_copy_vcpkg_deps(Mupen64RR_Core_Tests)
mupen64rr_reuse_common_pch(Mupen64RR_Core_Tests)
target_compile_features(Mupen64RR_Core_Tests PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Core_Tests PROPERTIES
    OUTPUT_NAME "Core.Tests"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
)
target_link_libraries(Mupen64RR_Core_Tests PRIVATE
    Catch2::Catch2WithMain
    Mupen64RR_Core__TestIncludes
)

# Skip running tests when cross-compiling between OSes.
if(NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests(Mupen64RR_Core_Tests)
else()
    message(STATUS "Cross-compiling: skipping test discovery for Mupen64RR_Core_Tests (run manually on target)")
endif()
endblock()
