#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Views_Qt6_Actions STATIC)
qt_add_qml_module(Mupen64RR_Views_Qt6_Actions
    URI "Actions"
    SOURCES
        "ActionHelpers.hpp"
        "HeldShortcutMap.hpp"
        "ActionHelpers.cpp"
        "HeldShortcutMap.cpp"
    QML_FILES
        "ActionManager.qml"
        "ActionRepeater.qml"
        "EmuAction.qml"
        "EmuHeldAction.qml"
        "EmuMenu.qml"
        "HeldShortcut.qml"
)
target_compile_features(Mupen64RR_Views_Qt6_Actions PUBLIC cxx_std_23)
target_link_libraries(Mupen64RR_Views_Qt6_Actions PUBLIC
    Qt6::Quick Qt6::QuickControls2
)
