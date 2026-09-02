#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Views_Qt6_Config STATIC)
qt_add_qml_module(Mupen64RR_Views_Qt6_Config
    URI "Config"
    IMPORTS
        TARGET Mupen64RR_Views_Qt6_Utils
        TARGET Mupen64RR_Views_Qt6_Components
    QML_FILES
        "Bool.qml"
        "Choices.qml"
        "Double.qml"
        "FilePath.qml"
        "FolderPath.qml"
        "Int.qml"
        "Page.qml"
        "Row.qml"
)
target_link_libraries(Mupen64RR_Views_Qt6_Config PUBLIC
    Qt6::Quick Qt6::QuickControls2
)
