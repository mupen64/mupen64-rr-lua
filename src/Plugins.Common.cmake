#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

# Common plugin dependencies.
add_library(Mupen64RR.Plugins.Win32.Common INTERFACE)
target_link_libraries(Mupen64RR.Plugins.Win32.Common INTERFACE
    Mupen64RR.Common
    Mupen64RR.Core.Headers
    Mupen64RR.Views.Win32.Headers
)
target_compile_definitions(Mupen64RR.Plugins.Win32.Common INTERFACE PLUGIN_WITH_CALLBACKS)