#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Common STATIC
    "include/CommonPCH.hpp"
    "include/DummyPluginStub.hpp"
    "include/IOUtils.hpp"
    "include/MiscHelpers.hpp"
    "include/StrUtils.hpp"
    "include/VersionNameHelpers.hpp"
    "include/FNV1A.hpp"
    "include/Hotkey.hpp"
    "Hotkey.cpp"
)

set_target_properties(Mupen64RR.Common PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(Mupen64RR.Common PUBLIC include)

target_link_libraries(Mupen64RR.Common PUBLIC
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    vendor::xxhash64
    SDL3::SDL3
)

# only MSVCRT implements C11 Annex K, libsafec is needed on other platforms
if (NOT WIN32)
    target_link_libraries(Mupen64RR.Common PUBLIC PkgConfig::libsafec)
endif()
