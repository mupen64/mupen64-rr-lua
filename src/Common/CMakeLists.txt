#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Common INTERFACE
    "include/CommonPCH.hpp"
    "include/DummyPluginStub.hpp"
    "include/IOUtils.hpp"
    "include/MiscHelpers.hpp"
    "include/StrUtils.hpp"
    "include/VersionNameHelpers.hpp"
    "include/FNV1A.hpp"
)

target_compile_features(Mupen64RR.Common INTERFACE cxx_std_23)
target_include_directories(Mupen64RR.Common INTERFACE include)

target_link_libraries(Mupen64RR.Common INTERFACE
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    lz4::lz4
    vendor::xxhash64
    SDL3::SDL3
)

# only MSVCRT implements C11 Annex K, libsafec is needed on other platforms
if (NOT WIN32)
    target_link_libraries(Mupen64RR.Common INTERFACE PkgConfig::libsafec)
endif()

add_compile_definitions(NOMINMAX)
