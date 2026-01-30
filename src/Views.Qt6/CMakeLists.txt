#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Views.Qt6.Headers INTERFACE
    "include/mupapi.h"
)
set_target_properties(Mupen64RR.Views.Qt6.Headers PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(Mupen64RR.Views.Qt6.Headers INTERFACE "include")
target_link_libraries(Mupen64RR.Views.Qt6.Headers INTERFACE
    Mupen64RR.Common
    Mupen64RR.Core.Headers
)

qt_add_executable(Mupen64RR.Views.Qt6
    "main.cpp"
    "model/Core.cpp"
    "model/Core.hpp"
    "model/Logging.cpp"
    "model/Logging.hpp"
    "model/Plugin.cpp"
    "model/Plugin.hpp"
    "view/service/QtCoreService.cpp"
    "view/service/QtCoreService.hpp"
    "view/service/QtOpenGLService.cpp"
    "view/service/QtOpenGLService.hpp"
    "view/MainWindow_Slots.cpp"
    "view/MainWindow.cpp"
    "view/MainWindow.hpp"
    "view/MainWindow.ui"
    "view/Utils.hpp"
)
set_target_properties(Mupen64RR.Views.Qt6 PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    # output location
    OUTPUT_NAME "mupen64"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
target_link_libraries(Mupen64RR.Views.Qt6 PRIVATE
    Mupen64RR.Common
    Mupen64RR.Core
    Mupen64RR.Views.Qt6.Headers
    Qt6::Widgets
    Boost::boost
    Boost::filesystem
    spdlog::spdlog
)
target_link_libraries(Mupen64RR.Views.Qt6 PRIVATE dl)

