#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_executable(Mupen64RR.Views.Qt6
    "plugins/BuiltinTAS.hpp"
    "plugins/Plugin.hpp"
    "Core.hpp"
    "Main.hpp"

    "plugins/BuiltinTAS.cpp"
    "plugins/Plugin.cpp"
    "DialogService.cpp"
    "Core.cpp"
    "Main.cpp"
)
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

    Qt6::Core
    Qt6::Gui
    Qt6::Quick

    vendor::decan
)

target_link_libraries(Mupen64RR.Views.Qt6 PRIVATE
    Mupen64RR.Plugins.DummyVideo
    Mupen64RR.Plugins.DummyAudio
    Mupen64RR.Plugins.DummyInput

    Mupen64RR.Plugins.TASAudio
    Mupen64RR.Plugins.TASRSP
)

# QML MODULES
# =====================================

qt_add_qml_module(Mupen64RR.Views.Qt6
    URI "Mupen64RR.UI"
    VERSION 1.0
    QML_FILES
    "views/MainWindow.qml"
)

# MinGW needs this for std::print(ln) some reason...
if(MINGW)
    target_link_libraries(Mupen64RR.Views.CLI PRIVATE stdc++exp)
endif()
