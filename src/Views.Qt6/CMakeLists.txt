#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_subdirectory(Utils)
add_subdirectory(Core)

qt_add_executable(Mupen64RR.Views.Qt6 "Main.cpp")
target_compile_features(Mupen64RR.Views.Qt6 PRIVATE cxx_std_26)
set_target_properties(Mupen64RR.Views.Qt6 PROPERTIES
    # output location
    OUTPUT_NAME "mupen64-qt6"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
target_include_directories(Mupen64RR.Views.Qt6 PRIVATE ".")
target_link_libraries(Mupen64RR.Views.Qt6 PRIVATE
    Mupen64RR.Common
    Mupen64RR.Common.Views
    Mupen64RR.Core

    Mupen64RR.Views.Qt6.Utils
    Mupen64RR.Views.Qt6.Utilsplugin
    Mupen64RR.Views.Qt6.Core
    Mupen64RR.Views.Qt6.Coreplugin

    Qt::Quick
    Qt::QuickControls2
)
set(qml_sources
    "Views/DialogService.qml"
    "Views/MainMenuBar.qml"
    "Views/MainWindow.qml"
    "Views/MessageBox.qml"
)
set(qml_singleton_sources
    "Views/SettingsCore.qml"
    "Views/SettingsPaths.qml"
    "Views/SettingsVCR.qml"
)
# mark sources as singletons *before* qt_add_qml_module as
# generation of qmldir happens there
set_source_files_properties(
    ${qml_singleton_sources}
    PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
qt_add_qml_module(Mupen64RR.Views.Qt6
    URI "Views"
    QML_FILES ${qml_sources} ${qml_singleton_sources}
)


# MinGW needs this for std::print(ln) some reason...
if(MINGW)
    target_link_libraries(Mupen64RR.Views.Qt6 PRIVATE stdc++exp)
endif()

# Deploy Qt dependencies on Windows
windeployqt6(
    TARGET Mupen64RR.Views.Qt6
    DEPLOY_TO_BUILD_DIR
    ARGS --qmldir .
)
