#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Audio.TAS MODULE
    "BetterAzi/Mupen64plusHLE/ucodes.h"
    "BetterAzi/Mupen64plusHLE/memory.h"
    "BetterAzi/Mupen64plusHLE/hle.h"
    "BetterAzi/Mupen64plusHLE/hle_internal.h"
    "BetterAzi/Mupen64plusHLE/hle_external.h"
    "BetterAzi/Mupen64plusHLE/common.h"
    "BetterAzi/Mupen64plusHLE/audio.h"
    "BetterAzi/Mupen64plusHLE/arithmetics.h"
    "BetterAzi/Mupen64plusHLE/alist.h"
    "BetterAzi/XAudio2SoundDriver.h"
    "BetterAzi/WaveOut.h"
    "BetterAzi/SoundDriverLegacy.h"
    "BetterAzi/SoundDriverInterface.h"
    "BetterAzi/SoundDriverFactory.h"
    "BetterAzi/SoundDriver.h"
    "BetterAzi/Settings.h"
    "BetterAzi/my_types.h"
    "BetterAzi/DirectSoundDriver.h"
    "BetterAzi/Configuration.h"
    "BetterAzi/common.h"
    "BetterAzi/audiohle.h"
    "BetterAzi/resource.h"

    "BetterAzi/Mupen64plusHLE/musyx.cpp"
    "BetterAzi/Mupen64plusHLE/Mupen64Support.cpp"
    "BetterAzi/Mupen64plusHLE/memory.cpp"
    "BetterAzi/Mupen64plusHLE/audio.cpp"
    "BetterAzi/XAudio2SoundDriver.cpp"
    "BetterAzi/WaveOut.cpp"
    "BetterAzi/SoundDriverLegacy.cpp"
    "BetterAzi/SoundDriverFactory.cpp"
    "BetterAzi/SoundDriver.cpp"
    "BetterAzi/main.cpp"
    "BetterAzi/HLEMain.cpp"
    "BetterAzi/DirectSoundDriver.cpp"
    "BetterAzi/Configuration.cpp"
    "BetterAzi/ABI3mp3.cpp"
    "BetterAzi/ABI3.cpp"
    "BetterAzi/ABI2.cpp"
    "BetterAzi/ABI1.cpp"
    "BetterAzi/ABI_Resample.cpp"
    "BetterAzi/ABI_MixerInterleave.cpp"
    "BetterAzi/ABI_Filters.cpp"
    "BetterAzi/ABI_Envmixer.cpp"
    "BetterAzi/ABI_Buffers.cpp"
    "BetterAzi/ABI_Adpcm.cpp"

    "BetterAzi/resource.rc"
)
set_target_properties(Mupen64RR.Plugins.Audio.TAS PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "TASAudio"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
remove_definitions(-DUNICODE -D_UNICODE) # FIXME: Support Unicode
target_include_directories(Mupen64RR.Plugins.Audio.TAS PRIVATE ".")
target_link_libraries(Mupen64RR.Plugins.Audio.TAS PRIVATE Mupen64RR.Plugins.Win32.Common)
target_link_libraries(Mupen64RR.Plugins.Audio.TAS PRIVATE
    Comctl32
    uxtheme
    Msimg32
    Gdiplus
    dsound
)