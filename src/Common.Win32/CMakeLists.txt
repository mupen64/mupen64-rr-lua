#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]
add_library(Mupen64RR.Common.Win32 INTERFACE
    "include/WinFilePicker.hpp"
    "include/JoystickControl.hpp"
)
set_target_properties(Mupen64RR.Common.Win32 PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(Mupen64RR.Common.Win32 INTERFACE include)
