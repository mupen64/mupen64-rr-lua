#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR.Views.Qt6.Core STATIC)
qt_add_qml_module(Mupen64RR.Views.Qt6.Core URI "Core" SOURCES 
    "CoreEnums.hpp"
    "MupenCore.hpp"

    "MupenCore.cpp"
)
target_link_libraries(Mupen64RR.Views.Qt6.Core PUBLIC
    Mupen64RR.Views.Qt6.Utils
)