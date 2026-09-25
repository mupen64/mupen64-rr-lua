#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

#

# Common Win32 plugin dependencies.
add_library(Mupen64RR_Plugins_Win32_Common INTERFACE)
target_link_libraries(Mupen64RR_Plugins_Win32_Common INTERFACE
    Mupen64RR_Plugins_Common
    Mupen64RR_Common_Win32
    Mupen64RR_Views_Win32_Headers
    vendor::windarkmode
)
