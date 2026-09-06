#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Views_Qt6_Components STATIC)
qt_add_qml_module(Mupen64RR_Views_Qt6_Components
    URI "Components"
    IMPORTS
        TARGET Mupen64RR_Views_Qt6_Utils
    SOURCES
    QML_FILES
        "DoubleSpinBox.qml"
        "DialogBase.qml"
        "ToolTipLabel.qml"
)
target_link_libraries(Mupen64RR_Views_Qt6_Components PUBLIC
    Qt6::Quick Qt6::QuickControls2
)
