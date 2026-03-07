#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Win32.TASInput MODULE
    "TASInput.h"
    "Common.h"
    "resource.h"
    "NewConfig.h"
    "Helpers.h"
    "Main.h"
    "JoystickControl.h"
    "GamepadManager.h"
    "ConfigDialog.h"
    "Combo.h"
        
    "TASInput.cpp" 
    "NewConfig.cpp"
    "Main.cpp"
    "JoystickControl.cpp"
    "GamepadManager.cpp"
    "ConfigDialog.cpp"
    "Combo.cpp"

    "Resource.rc"
)
set_target_properties(Mupen64RR.Plugins.Win32.TASInput PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "TASInput"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.Win32.TASInput PRIVATE ".")
target_link_libraries(Mupen64RR.Plugins.Win32.TASInput PRIVATE 
    Mupen64RR.Plugins.Win32.Common
    SDL3::SDL3
)
target_link_libraries(Mupen64RR.Plugins.Win32.TASInput PRIVATE
    setupapi
    hid
    imm32
    version
    winmm
    comctl32
    shcore
    Gdiplus
)
target_compile_definitions(Mupen64RR.Plugins.Win32.TASInput PRIVATE 
    _USE_MATH_DEFINES
    CORE_PLUGIN_INPUT_OLD_INITIATE_CONTROLLERS
)