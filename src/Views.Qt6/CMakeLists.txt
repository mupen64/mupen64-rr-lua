#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_executable(Mupen64RR.Views.Qt6
    "Main.hpp"
    "Plugin.hpp"

    "DialogService.cpp"
    "Main.cpp"
    "Plugin.cpp"
)
target_compile_features(Mupen64RR.Views.Qt6 PRIVATE cxx_std_26)
set_target_properties(Mupen64RR.Views.Qt6 PROPERTIES
    # output location
    OUTPUT_NAME "mupen64-qt6"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
target_link_libraries(Mupen64RR.Views.Qt6 PRIVATE
    Mupen64RR.Common
    Mupen64RR.Common.Views
    Mupen64RR.Core
    Mupen64RR.Plugins.TASVideo
    Mupen64RR.Plugins.TASAudio
    Mupen64RR.Plugins.DummyInput
    Mupen64RR.Plugins.TASRSP
)

# MinGW needs this for std::print(ln) some reason...
if(MINGW)
    target_link_libraries(Mupen64RR.Views.CLI PRIVATE stdc++exp)
endif()
