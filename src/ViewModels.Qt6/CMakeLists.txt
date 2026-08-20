#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR.ViewModels.Qt6 STATIC
    "include/ViewModels.Qt6/plugins/BuiltinTAS.hpp"
    "include/ViewModels.Qt6/plugins/Plugin.hpp"
    "include/ViewModels.Qt6/Core.hpp"
    "include/ViewModels.Qt6/QtIconImageProvider.hpp"

    "plugins/BuiltinTAS.cpp"
    "plugins/Plugin.cpp"
    "Core.cpp"
    "DialogService.cpp"
    "QtIconImageProvider.cpp"
)
target_compile_features(Mupen64RR.ViewModels.Qt6 PUBLIC cxx_std_26)
target_include_directories(Mupen64RR.ViewModels.Qt6 PUBLIC "include")
target_link_libraries(Mupen64RR.ViewModels.Qt6 PUBLIC
    Mupen64RR.Common
    Mupen64RR.Common.Views
    Mupen64RR.Core

    Qt6::Core
    Qt6::Quick

    vendor::decan
)

target_link_libraries(Mupen64RR.ViewModels.Qt6 PUBLIC
    Mupen64RR.Plugins.DummyVideo
    Mupen64RR.Plugins.DummyAudio
    Mupen64RR.Plugins.DummyInput

    Mupen64RR.Plugins.TASAudio
    Mupen64RR.Plugins.TASRSP
)