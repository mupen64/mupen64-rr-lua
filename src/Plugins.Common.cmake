#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

#

# Common Win32 plugin dependencies.
add_library(Mupen64RR_Plugins_Common INTERFACE)
target_link_libraries(Mupen64RR_Plugins_Common INTERFACE
    Mupen64RR_Common
    Mupen64RR_Core_Headers
)
target_compile_definitions(Mupen64RR_Plugins_Common INTERFACE
    PLUGIN_WITH_CALLBACKS
    "VERSION_SUFFIX=\"${MUPEN64RR_VERSION_SUFFIX}\""
    "$<$<BOOL:${MUPEN64RR_NIGHTLY}>:NIGHTLY>"
)
