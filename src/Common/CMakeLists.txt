#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR_Common STATIC
    "CommonPCH.cpp"
    "include/Common/Assert.hpp"
    "include/Common/CommonPCH.hpp"
    "include/Common/DummyPluginStub.hpp"
    "include/Common/IOUtils.hpp"
    "include/Common/MiscHelpers.hpp"
    "include/Common/StrUtils.hpp"
    "include/Common/VersionNameHelpers.hpp"
    "include/Common/FNV1A.hpp"
)
target_compile_features(Mupen64RR_Common PUBLIC cxx_std_26)
target_include_directories(Mupen64RR_Common PUBLIC
    include
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/xxhash64"
)
target_precompile_headers(Mupen64RR_Common PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Common/CommonPCH.hpp"
)

target_link_libraries(Mupen64RR_Common PUBLIC
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    PkgConfig::lz4
    vendor::xxhash64
    SDL3::SDL3
)

# only MSVCRT implements C11 Annex K, libsafec is needed on other platforms
if (NOT WIN32)
    target_link_libraries(Mupen64RR_Common PUBLIC PkgConfig::libsafec)
endif()

target_compile_definitions(Mupen64RR_Common PUBLIC NOMINMAX)
