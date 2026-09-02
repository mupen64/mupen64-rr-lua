#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_subdirectory(Utils)
add_subdirectory(Components)
add_subdirectory(Config)
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
    Mupen64RR_Views_Qt6_Componentsplugin
    Mupen64RR_Views_Qt6_Configplugin
    Mupen64RR_Views_Qt6_Core
    Mupen64RR_Views_Qt6_Coreplugin

    Qt::Quick
    Qt::QuickControls2
)
set(qml_sources
    "Views/ConfigDialog.qml"
    "Views/DialogService.qml"
    "Views/MainMenuBar.qml"
    "Views/MainWindow.qml"
    "Views/MessageBox.qml"
)
set(qml_singleton_sources
    "Views/SettingsCore.qml"
    "Views/SettingsPaths.qml"
)
set(qml_config_pages
    "Views/Config/EmulationPage.qml"
)
# mark singletons
set_source_files_properties(${qml_singleton_sources} PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
foreach(item IN LISTS qml_config_pages)
    string(REGEX REPLACE "Views/Config/(.+)\\.qml" "Config\\1" page_typename "${item}")
    # message(STATUS "${item} -> ${page_typename}")
    set_source_files_properties("${item}" PROPERTIES
        QT_QML_SOURCE_TYPENAME "${page_typename}"
    )
endforeach()

qt_add_qml_module(Mupen64RR_Views_Qt6
    URI "Views"
    QML_FILES
        ${qml_sources}
        ${qml_singleton_sources}
        ${qml_config_pages}
)


# MinGW needs this for std::print(ln) some reason...
if(MINGW)
    target_link_libraries(Mupen64RR_Views_Qt6 PRIVATE stdc++exp)
endif()

# Deploy Qt dependencies on Windows
windeployqt6(
    TARGET Mupen64RR_Views_Qt6
    DEPLOY_TO_BUILD_DIR
    ARGS --qmldir .
)
