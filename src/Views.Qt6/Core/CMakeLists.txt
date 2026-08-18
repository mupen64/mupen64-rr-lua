qt_add_library(Mupen64RR.Views.Qt6.Core STATIC)
qt_add_qml_module(Mupen64RR.Views.Qt6.Core
    URI "Core"
    SOURCES "MupenCore.hpp" "MupenCore.cpp"
)
target_link_libraries(Mupen64RR.Views.Qt6.Core PUBLIC
    Mupen64RR.ViewModels.Qt6
)