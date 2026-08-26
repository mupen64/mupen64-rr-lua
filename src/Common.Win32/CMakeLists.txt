#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]
add_library(Mupen64RR.Common.Win32 INTERFACE
    "include/Common.Win32/Common.hpp"
    "include/Common.Win32/WinFilePicker.hpp"
    "include/Common.Win32/JoystickControl.hpp"
    "include/Common.Win32/SettingsListView.hpp"
    "include/Common.Win32/ResizeAnchor.hpp"
)
target_compile_features(Mupen64RR.Common.Win32 INTERFACE cxx_std_26)
target_include_directories(Mupen64RR.Common.Win32 INTERFACE include)

target_compile_definitions(Mupen64RR.Common.Win32 INTERFACE NOMINMAX)
