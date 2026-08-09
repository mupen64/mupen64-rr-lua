#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]
add_library(Mupen64RR.Common.Win32 INTERFACE
    "include/WinFilePicker.hpp"
    "include/JoystickControl.hpp"
    "include/SettingsListView.hpp"
    "include/ResizeAnchor.hpp"
)
target_compile_features(Mupen64RR.Common.Win32 INTERFACE cxx_std_26)
target_include_directories(Mupen64RR.Common.Win32 INTERFACE include)

add_compile_definitions(NOMINMAX)
