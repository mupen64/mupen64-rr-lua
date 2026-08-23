qt_add_library(Mupen64RR.Views.Qt6.Utils STATIC
    "include/QtIconImageProvider.hpp"
    "include/QtUtils.hpp"
    "QtIconImageProvider.cpp"
)
target_compile_features(Mupen64RR.Views.Qt6.Utils PUBLIC cxx_std_26)
target_include_directories(Mupen64RR.Views.Qt6.Utils PUBLIC "include")
target_link_libraries(Mupen64RR.Views.Qt6.Utils PUBLIC
    Qt6::Core
    Qt6::Quick

    Mupen64RR.ViewModels.Qt6
)