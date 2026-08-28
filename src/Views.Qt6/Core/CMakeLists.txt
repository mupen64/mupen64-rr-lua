#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Views_Qt6_Core STATIC)
mupen64rr_reuse_common_pch(Mupen64RR_Views_Qt6_Core)
qt_add_qml_module(Mupen64RR_Views_Qt6_Core URI "Core"
    IMPORTS
        TARGET Mupen64RR_Views_Qt6_Utils
    SOURCES
        "plugin/BuiltinTAS.hpp"
        "plugin/Plugin.hpp"
        "CoreEnums.hpp"
        "EmuContext.hpp"
        "EmuDisplay.hpp"

        "plugin/BuiltinTAS.cpp"
        "plugin/Plugin.cpp"
        "CoreEnums.cpp"
        "EmuContext.cpp"
        "EmuDisplay.cpp"
)
target_compile_features(Mupen64RR_Views_Qt6_Core PUBLIC cxx_std_26)
target_include_directories(Mupen64RR_Views_Qt6_Core PUBLIC ".")
target_link_libraries(Mupen64RR_Views_Qt6_Core PUBLIC
    Mupen64RR_Common
    Mupen64RR_Common_Views
    Mupen64RR_Core

    Mupen64RR_Views_Qt6_Utils

    Qt6::Core
    Qt6::Quick

    vendor::decan
)

target_link_libraries(Mupen64RR_Views_Qt6_Core PUBLIC
    Mupen64RR_Plugins_DummyVideo
    Mupen64RR_Plugins_DummyAudio
    Mupen64RR_Plugins_DummyInput

    Mupen64RR_Plugins_TASVideo
    Mupen64RR_Plugins_TASAudio
    Mupen64RR_Plugins_TASRSP
)
