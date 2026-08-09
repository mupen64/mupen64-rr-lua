#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_executable(Mupen64RR.Views.CLI
    "Dialog.hpp"
    "Main.hpp"
    "Plugin.hpp"

    "Dialog.cpp"
    "Main.cpp"
    "Plugin.cpp"
)
target_compile_features(Mupen64RR.Views.CLI PRIVATE cxx_std_26)
set_target_properties(Mupen64RR.Views.CLI PROPERTIES
    # output location
    OUTPUT_NAME "mupen64-cli"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
target_link_libraries(Mupen64RR.Views.CLI PRIVATE
    Mupen64RR.Common
    Mupen64RR.Core
    vendor::decan
)
