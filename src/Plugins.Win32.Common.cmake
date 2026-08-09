#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

#

# Common Win32 plugin dependencies.
add_library(Mupen64RR.Plugins.Win32.Common INTERFACE)
target_link_libraries(Mupen64RR.Plugins.Win32.Common INTERFACE
    Mupen64RR.Plugins.Common
    Mupen64RR.Common.Win32
    Mupen64RR.Views.Win32.Headers
)
