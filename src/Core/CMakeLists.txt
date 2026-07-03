#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Core.Headers INTERFACE
    "include/core_plugin.h"
    "include/core_types.h"
    "include/core_api.h"
)
set_target_properties(Mupen64RR.Core.Headers PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(Mupen64RR.Core.Headers INTERFACE "include")
target_link_libraries(Mupen64RR.Core.Headers INTERFACE Mupen64RR.Common)

add_library(Mupen64RR.Core STATIC
    "Core.hpp"
    "Alloc.hpp"
    "Cheats.hpp"
    "Memory/PifLut.hpp"
    "Memory/DMA.hpp"
    "Memory/FlashRAM.hpp"
    "Memory/Memory.hpp"
    "Memory/Pif.hpp"
    "Memory/Savestates.hpp"
    "Memory/StateHash.hpp"
    "Memory/Summercart.hpp"
    "Memory/TLB.hpp"
    "R4300/Debug.hpp"
    "R4300/Ops.hpp"
    "R4300/Cop1Helpers.hpp"
    "R4300/Disasm.hpp"
    "R4300/Exception.hpp"
    "R4300/Interrupt.hpp"
    "R4300/Macros.hpp"
    "R4300/R4300.hpp"
    "R4300/Recomp.hpp"
    "R4300/Recomph.hpp"
    "R4300/Rom.hpp"
    "R4300/Timers.hpp"
    "R4300/Tracelog.hpp"
    "R4300/VCR.hpp"
    "R4300/x86/Assemble.hpp"
    "R4300/x86/Gcop1Helpers.hpp"
    "R4300/x86/RegCache.hpp"

    "Core.cpp"
    "Alloc.cpp"
    "Cheats.cpp"
    "Memory/PifLut.cpp"
    "Memory/DMA.cpp"
    "Memory/FlashRAM.cpp"
    "Memory/Memory.cpp"
    "Memory/Pif.cpp"
    "Memory/Savestates.cpp"
    "Memory/StateHash.cpp"
    "Memory/Summercart.cpp"
    "Memory/TLB.cpp"
    "R4300/Debug.cpp"
    "R4300/PureInterp.cpp"
    "R4300/Cop0.cpp"
    "R4300/Cop1.cpp"
    "R4300/Cop1D.cpp"
    "R4300/Cop1Helpers.cpp"
    "R4300/Cop1L.cpp"
    "R4300/Cop1S.cpp"
    "R4300/Cop1W.cpp"
    "R4300/Disasm.cpp"
    "R4300/Exception.cpp"
    "R4300/Interrupt.cpp"
    "R4300/R4300.cpp"
    "R4300/Recomp.cpp"
    "R4300/RegImm.cpp"
    "R4300/Rom.cpp"
    "R4300/Special.cpp"
    "R4300/Timers.cpp"
    "R4300/Tracelog.cpp"
    "R4300/BC.cpp"

    "R4300/VCR.cpp"
)
set_target_properties(Mupen64RR.Core PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME Core
)
target_include_directories(Mupen64RR.Core PUBLIC "include")
target_include_directories(Mupen64RR.Core PRIVATE ".")
target_link_libraries(Mupen64RR.Core PRIVATE
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    vendor::aladdin-md5
)
target_link_libraries(Mupen64RR.Core PUBLIC
    Mupen64RR.Common
    Mupen64RR.Core.Headers
)
target_compile_options(Mupen64RR.Core PRIVATE -Werror=nonportable-include-path)

# options: disable dynarec
if (${MUPEN64RR_ENABLE_DYNAREC})
    target_compile_definitions(Mupen64RR.Core PRIVATE "MUPEN64RR_ENABLE_DYNAREC")
    target_sources(Mupen64RR.Core PRIVATE
        "R4300/x86/Assemble.cpp"
        "R4300/x86/GBc.cpp"
        "R4300/x86/GCop0.cpp"
        "R4300/x86/GCop1.cpp"
        "R4300/x86/GCop1D.cpp"
        "R4300/x86/GCop1Helpers.cpp"
        "R4300/x86/GCop1L.cpp"
        "R4300/x86/GCop1S.cpp"
        "R4300/x86/GCop1W.cpp"
        "R4300/x86/GR4300.cpp"
        "R4300/x86/GRegImm.cpp"
        "R4300/x86/GSpecial.cpp"
        "R4300/x86/GTLB.cpp"
        "R4300/x86/RegCache.cpp"
        "R4300/x86/RJump.cpp"
    )
endif()

# FOR TESTING ONLY. DO NOT USE OTHERWISE.
add_library(Mupen64RR.Core._TestIncludes INTERFACE)
target_link_libraries(Mupen64RR.Core._TestIncludes INTERFACE
    Mupen64RR.Core
)
target_include_directories(Mupen64RR.Core._TestIncludes INTERFACE "../" "./")
