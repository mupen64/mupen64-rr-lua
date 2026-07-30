#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

#

# Common Win32 plugin dependencies.
add_library(Mupen64RR.Plugins.Common INTERFACE)
target_link_libraries(Mupen64RR.Plugins.Common INTERFACE
    Mupen64RR.Common
    Mupen64RR.Core.Headers
)
target_compile_definitions(Mupen64RR.Plugins.Common INTERFACE
    PLUGIN_WITH_CALLBACKS
    "VERSION_SUFFIX=\"${MUPEN64RR_VERSION_SUFFIX}\""
    "$<$<BOOL:${MUPEN64RR_NIGHTLY}>:NIGHTLY>"
)
