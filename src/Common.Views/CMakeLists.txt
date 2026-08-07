#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Common.Views STATIC
    "include/Config.hpp"
    "include/Hotkey.hpp"
    "include/Messages.hpp"
    "include/Messenger.hpp"
    "include/ActionManager.hpp"
    "Config.cpp"
    "Hotkey.cpp"
    "Messenger.cpp"
    "ActionManager.cpp"
)

target_compile_features(Mupen64RR.Common.Views PUBLIC cxx_std_23)
target_include_directories(Mupen64RR.Common.Views PUBLIC include)

target_link_libraries(Mupen64RR.Common.Views PUBLIC
    Mupen64RR.Common
    Mupen64RR.Core.Headers
    vendor::xxhash64
    vendor::mini
    vendor::microlru
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    SDL3::SDL3
    nlohmann_json::nlohmann_json
)

# only MSVCRT implements C11 Annex K, libsafec is needed on other platforms
if (NOT WIN32)
    target_link_libraries(Mupen64RR.Common.Views PUBLIC PkgConfig::libsafec)
endif()
