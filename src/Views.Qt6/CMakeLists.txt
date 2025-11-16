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
    "model/Core.hpp"
    "model/Core.cpp"
    "model/Plugin.hpp"
    "model/Plugin.cpp"
    "ui/MainWindow.hpp"
    "ui/MainWindow.cpp"
    "ui/MainWindow.ui"
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

