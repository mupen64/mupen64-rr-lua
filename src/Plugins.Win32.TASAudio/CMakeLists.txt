#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Win32.TASAudio MODULE
    "Mupen64plusHLE/ucodes.h"
    "Mupen64plusHLE/memory.h"
    "Mupen64plusHLE/hle_internal.h"
    "Mupen64plusHLE/hle_external.h"
    "Mupen64plusHLE/common.h"
    "Mupen64plusHLE/audio.h"
    "SoundDriverInterface.h"
    "SoundDriver.h"
    "Types.h"
    "DirectSoundDriver.h"
    "Configuration.h"
    "Common.h"
    "AudioHLE.h"
    "Resource.h"

    "Mupen64plusHLE/musyx.cpp"
    "Mupen64plusHLE/Mupen64Support.cpp"
    "Mupen64plusHLE/memory.cpp"
    "Mupen64plusHLE/audio.cpp"
    "SoundDriver.cpp"
    "Main.cpp"
    "HLEMain.cpp"
    "DirectSoundDriver.cpp"
    "Configuration.cpp"
    "ABI3mp3.cpp"
    "ABI3.cpp"
    "ABI2.cpp"
    "ABI1.cpp"
    "ABI_Resample.cpp"
    "ABI_MixerInterleave.cpp"
    "ABI_Filters.cpp"
    "ABI_Envmixer.cpp"
    "ABI_Buffers.cpp"
    "ABI_Adpcm.cpp"

    "Resource.rc"
)
set_target_properties(Mupen64RR.Plugins.Win32.TASAudio PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "TASAudio"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.Win32.TASAudio PRIVATE ".")
target_link_libraries(Mupen64RR.Plugins.Win32.TASAudio PRIVATE Mupen64RR.Plugins.Win32.Common)
target_link_libraries(Mupen64RR.Plugins.Win32.TASAudio PRIVATE
    Comctl32
    uxtheme
    Msimg32
    Gdiplus
    dsound
)