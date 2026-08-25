#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR.Views.Qt6.Core STATIC)
qt_add_qml_module(Mupen64RR.Views.Qt6.Core URI "Core"
    IMPORTS
        TARGET Mupen64RR.Views.Qt6.Utils
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
target_compile_features(Mupen64RR.Views.Qt6.Core PUBLIC cxx_std_26)
target_include_directories(Mupen64RR.Views.Qt6.Core PUBLIC ".")
target_link_libraries(Mupen64RR.Views.Qt6.Core PUBLIC
    Mupen64RR.Common
    Mupen64RR.Common.Views
    Mupen64RR.Core

    Mupen64RR.Views.Qt6.Utils

    Qt6::Core
    Qt6::Quick

    vendor::bs-thread-pool
    vendor::decan
)

target_link_libraries(Mupen64RR.Views.Qt6.Core PUBLIC
    Mupen64RR.Plugins.DummyVideo
    Mupen64RR.Plugins.DummyAudio
    Mupen64RR.Plugins.DummyInput

    Mupen64RR.Plugins.TASVideo
    Mupen64RR.Plugins.TASAudio
    Mupen64RR.Plugins.TASRSP
)