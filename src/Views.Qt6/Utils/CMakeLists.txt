#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Views_Qt6_Utils STATIC)
mupen64rr_reuse_common_pch(Mupen64RR_Views_Qt6_Utils)
qt_add_qml_module(Mupen64RR_Views_Qt6_Utils URI "Utils" SOURCES
    "include/PathsUtil.hpp"
    "include/QJSInterop.hpp"
    "include/QtIconImageProvider.hpp"
    "include/QtUtils.hpp"
    "include/QmlCallableContext.hpp"
    "QtIconImageProvider.cpp"
)
target_compile_features(Mupen64RR_Views_Qt6_Utils PUBLIC cxx_std_26)
target_include_directories(Mupen64RR_Views_Qt6_Utils PUBLIC "include")
target_link_libraries(Mupen64RR_Views_Qt6_Utils PUBLIC
    Mupen64RR_Common
    Mupen64RR_Common_Views
    Mupen64RR_Core

    Qt6::Core
    Qt6::Quick
)
