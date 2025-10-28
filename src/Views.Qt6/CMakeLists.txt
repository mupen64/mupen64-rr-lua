qt_add_executable(Mupen64RR.Views.Qt6
    "MainWindow.hpp"

    "MainWindow.cpp"
    "main.cpp"

    "MainWindow.ui"
)
set_target_properties(Mupen64RR.Views.Qt6 PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    # output location
    OUTPUT_NAME "mupen64"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
target_include_directories(Mupen64RR.Views.Qt6 INTERFACE "include")
target_link_libraries(Mupen64RR.Views.Qt6 PRIVATE
    Mupen64RR.Common
    Mupen64RR.Core
    Qt6::Widgets
)

