#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR.Views.Qt6.Utils STATIC)
qt_add_qml_module(Mupen64RR.Views.Qt6.Utils URI "Utils" SOURCES
    "include/QJSInterop.hpp"
    "include/QtIconImageProvider.hpp"
    "include/QtUtils.hpp"
    "include/QmlCallableContext.hpp"
    "QtIconImageProvider.cpp"
)
target_compile_features(Mupen64RR.Views.Qt6.Utils PUBLIC cxx_std_26)
target_include_directories(Mupen64RR.Views.Qt6.Utils PUBLIC "include")
target_link_libraries(Mupen64RR.Views.Qt6.Utils PUBLIC
    Mupen64RR.Common
    Mupen64RR.Common.Views
    Mupen64RR.Core

    Qt6::Core
    Qt6::Quick
)
