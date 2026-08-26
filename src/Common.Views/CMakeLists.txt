#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Common.Views STATIC
    "include/Common.Views/App.hpp"
    "include/Common.Views/Config.hpp"
    "include/Common.Views/Hotkey.hpp"
    "include/Common.Views/Messages.hpp"
    "include/Common.Views/Messenger.hpp"
    "include/Common.Views/ActionManager.hpp"
    "include/Common.Views/Resampler.hpp"
    "include/Common.Views/Encoder.hpp"
    "include/Common.Views/IDialogService.hpp"
    "include/Common.Views/ConfigLegacy.hpp"
    "include/Common.Views/OptionItem.hpp"
    "Config.cpp"
    "ConfigLegacy.cpp"
    "Hotkey.cpp"
    "Messenger.cpp"
    "ActionManager.cpp"
    "Resampler.cpp"
    "OptionItem.cpp"
)

if (WIN32)
    target_sources(Mupen64RR.Common.Views PRIVATE
        "include/Common.Views/WinFFmpegEncoder.hpp"
        "include/Common.Views/WinVFWEncoder.hpp"
        "WinFFmpegEncoder.cpp"
        "WinVFWEncoder.cpp"
    )
endif()

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
    spdlog::spdlog
    PkgConfig::speexdsp
    PkgConfig::libav
)

if (WIN32)
    target_link_libraries(Mupen64RR.Common.Views PUBLIC vfw32)
endif()

if (NOT WIN32)
    target_link_libraries(Mupen64RR.Common.Views PUBLIC PkgConfig::libsafec)
endif()

target_compile_definitions(Mupen64RR.Common.Views PUBLIC
    NOMINMAX
    MINI_CASE_SENSITIVE
)
