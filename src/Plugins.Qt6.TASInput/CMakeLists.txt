#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Plugins_Qt6_TASInput STATIC)
qt_add_qml_module(Mupen64RR_Plugins_Qt6_TASInput
    URI "TASInput"
    SOURCES
        "Config.hpp"
        "JoystickData.hpp"
        "Main.hpp"
        "UIManager.hpp"

        "Config.cpp"
        "JoystickData.cpp"
        "Main.cpp"
        "UIManager.cpp"
    QML_FILES
        "ButtonsSection.qml"
        "Joystick.qml"
        "JoystickSection.qml"
        "MainWindow.qml"
        "TASButton.qml"
        "TASPadLabel.qml"
)
target_compile_features(Mupen64RR_Plugins_Qt6_TASInput PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Plugins_Qt6_TASInput PROPERTIES
    OUTPUT_NAME "TASInput_Qt6"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
mupen64rr_reuse_common_pch(Mupen64RR_Plugins_Qt6_TASInput)
target_link_libraries(Mupen64RR_Plugins_Qt6_TASInput PRIVATE
    Mupen64RR_Plugins_Common
    Qt6::Core
    Qt6::Quick
)
target_compile_definitions(Mupen64RR_Plugins_Qt6_TASInput PRIVATE
    PLUGIN_NAMESPACE=BuiltinTAS::TASInput_Qt6
)
