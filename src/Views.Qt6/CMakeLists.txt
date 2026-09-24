#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_subdirectory(Utils)
add_subdirectory(Actions)
add_subdirectory(Components)
add_subdirectory(Config)
add_subdirectory(Core)


# C++ FILES
# ===================================

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

    Mupen64RR_Views_Qt6_Actionsplugin
    Mupen64RR_Views_Qt6_Componentsplugin
    Mupen64RR_Views_Qt6_Configplugin
    Mupen64RR_Views_Qt6_Core
    Mupen64RR_Views_Qt6_Coreplugin
    Mupen64RR_Views_Qt6_Utils
    Mupen64RR_Views_Qt6_Utilsplugin

    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
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
# clang-cl generates a faulty UAC manifest section. Since Qt is generating
# it already, we just ignore it.
if (MSVC)
    target_link_options(Mupen64RR_Views_Qt6 PRIVATE "/MANIFESTUAC:NO")
endif()

# QML FILES
# ===================================

set(qml_sources
    "Views/ConfigWindow.qml"
    "Views/DialogService.qml"
    "Views/MainWindow.qml"
)
set(qml_settings_sources
    "Views/Settings/Actions.qml"
    "Views/Settings/Core.qml"
    "Views/Settings/Paths.qml"
)
set(qml_config_pages
    "Views/Config/EmulationPage.qml"
    "Views/Config/FoldersPage.qml"
    "Views/Config/HotkeysPage.qml"
)
# rename settings pages and mark singletons
foreach(item IN LISTS qml_settings_sources)
    string(REGEX REPLACE
        "Views/Settings/(.+)\\.qml" "Settings\\1" page_typename
        "${item}"
    )
    set_source_files_properties("${item}" PROPERTIES
        QT_QML_SOURCE_TYPENAME "${page_typename}"
    )
endforeach()
# rename config type names
foreach(item IN LISTS qml_config_pages)
    string(REGEX REPLACE
        "Views/Config/(.+)\\.qml" "Config\\1" page_typename
        "${item}"
    )
    # message(STATUS "${item} -> ${page_typename}")
    set_source_files_properties("${item}" PROPERTIES
        QT_QML_SOURCE_TYPENAME "${page_typename}"
    )
endforeach()

qt_add_qml_module(Mupen64RR_Views_Qt6
    URI "Views"
    QML_FILES
        ${qml_sources}
        ${qml_settings_sources}
        ${qml_config_pages}
)


# LOCALIZATION
# ===================================

set(fallback_ts_name "${PROJECT_NAME}_${QT_I18N_SOURCE_LANGUAGE}.ts")
set(ts_dir "${CMAKE_CURRENT_SOURCE_DIR}/Translations")
set(fallback_ts "${ts_dir}/${fallback_ts_name}")

# note: there is a zero-width space as the 2nd option to `lrelease`.
# This is a workaround to CMake not letting me pass the empty string.
qt_add_translations(Mupen64RR_Views_Qt6
    SOURCE_TARGETS Mupen64RR_Views_Qt6
    TS_OUTPUT_DIRECTORY "${ts_dir}"
    TS_FILES "${fallback_ts}"
)

add_custom_command(TARGET mupen64-rr_lupdate POST_BUILD
    COMMAND Python::Interpreter ARGS
        "${PROJECT_SOURCE_DIR}/tools/qt_translate_identity.py"
        "${fallback_ts}"
    COMMENT "Filling translations for ${fallback_ts_name}"
)
