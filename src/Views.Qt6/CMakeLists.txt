#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_subdirectory(Utils)
add_subdirectory(Core)

qt_add_executable(Mupen64RR_Views_Qt6 "Main.cpp")
mupen64rr_reuse_common_pch(Mupen64RR_Views_Qt6)
target_compile_features(Mupen64RR_Views_Qt6 PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Views_Qt6 PROPERTIES
    # output location
    OUTPUT_NAME "mupen64-qt6"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
target_include_directories(Mupen64RR_Views_Qt6 PRIVATE ".")
target_link_libraries(Mupen64RR_Views_Qt6 PRIVATE
    Mupen64RR_Common
    Mupen64RR_Common_Views
    Mupen64RR_Core

    Mupen64RR_Views_Qt6_Utils
    Mupen64RR_Views_Qt6_Utilsplugin
    Mupen64RR_Views_Qt6_Core
    Mupen64RR_Views_Qt6_Coreplugin

    Qt::Quick
    Qt::QuickControls2
)

qt_add_qml_module(Mupen64RR_Views_Qt6 URI "Views" QML_FILES
    "Views/DialogService.qml"
    "Views/MainMenuBar.qml"
    "Views/MainWindow.qml"
    "Views/MessageBox.qml"
)

# MinGW needs this for std::print(ln) some reason...
if(MINGW)
    target_link_libraries(Mupen64RR_Views_Qt6 PRIVATE stdc++exp)
endif()

windeployqt6(
    TARGET Mupen64RR_Views_Qt6
    DEPLOY_TO_BUILD_DIR
    ARGS --qmldir .
)
