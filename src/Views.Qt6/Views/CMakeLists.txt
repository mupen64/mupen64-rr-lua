#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Views_Qt6_Views STATIC)
qt_add_qml_module(Mupen64RR_Views_Qt6_Views
    URI "Mupen64.Views"
    QML_FILES
        "ConfigEmulationPage.qml"
        "ConfigFoldersPage.qml"
        "ConfigHotkeysPage.qml"
        "ConfigWindow.qml"
        "DialogService.qml"
        "LuaScriptManager.qml"
        "MainWindow.qml"
        "MovieDialog.qml"
        "RomBrowser.qml"
        "RomBrowserCell.qml"
        "SettingsActions.qml"
        "SettingsCore.qml"
        "SettingsPaths.qml"
        "SimpleTableCell.qml"
    IMPORTS
        TARGET Mupen64RR_Views_Qt6_Lua
)
target_link_libraries(Mupen64RR_Views_Qt6_Views PUBLIC
    Qt6::Quick Qt6::QuickControls2
)
