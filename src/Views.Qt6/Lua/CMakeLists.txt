#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Views_Qt6_Lua STATIC)
mupen64rr_reuse_common_pch(Mupen64RR_Views_Qt6_Lua)
qt_add_qml_module(Mupen64RR_Views_Qt6_Lua URI "Lua"
    IMPORTS
        TARGET Mupen64RR_Views_Qt6_Core
        TARGET Mupen64RR_Views_Qt6_Utils
    SOURCES
        "LuaInstance.hpp"
        "LuaInstanceManager.hpp"
        "LuaCanvas.hpp"
        "LuaCanvasManager.hpp"

        "LuaInstance.cpp"
        "LuaInstanceManager.cpp"
        "LuaCanvas.cpp"
        "LuaCanvasManager.cpp"
    QML_FILES
        "LuaQmlCanvas.qml"
)
target_compile_features(Mupen64RR_Views_Qt6_Lua PUBLIC cxx_std_26)
target_include_directories(Mupen64RR_Views_Qt6_Lua PUBLIC "." "../Core")
target_link_libraries(Mupen64RR_Views_Qt6_Lua PUBLIC
    Mupen64RR_Common
    Mupen64RR_Common_Views
    Mupen64RR_Core
    Mupen64RR_Views_Qt6_Core
    Mupen64RR_Views_Qt6_Utils

    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Lua::Lua
)
