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
    "alloc.hpp"
    "cheats.hpp"
    "Memory/pif_lut.hpp"
    "Memory/dma.hpp"
    "Memory/flashram.hpp"
    "Memory/memory.hpp"
    "Memory/pif.hpp"
    "Memory/savestates.hpp"
    "Memory/summercart.hpp"
    "Memory/tlb.hpp"
    "R4300/Debug.hpp"
    "R4300/ops.hpp"
    "R4300/cop1_helpers.hpp"
    "R4300/disasm.hpp"
    "R4300/exception.hpp"
    "R4300/interrupt.hpp"
    "R4300/macros.hpp"
    "R4300/r4300.hpp"
    "R4300/recomp.hpp"
    "R4300/recomph.hpp"
    "R4300/rom.hpp"
    "R4300/timers.hpp"
    "R4300/tracelog.hpp"
    "R4300/vcr.hpp"
    "R4300/x86/assemble.hpp"
    "R4300/x86/gcop1_helpers.hpp"
    "R4300/x86/regcache.hpp"

    "Core.cpp"
    "alloc.cpp"
    "cheats.cpp"
    "Memory/pif_lut.cpp"
    "Memory/dma.cpp"
    "Memory/flashram.cpp"
    "Memory/memory.cpp"
    "Memory/pif.cpp"
    "Memory/savestates.cpp"
    "Memory/summercart.cpp"
    "Memory/tlb.cpp"
    "R4300/Debug.cpp"
    "R4300/pure_interp.cpp"
    "R4300/cop0.cpp"
    "R4300/cop1.cpp"
    "R4300/cop1_d.cpp"
    "R4300/cop1_helpers.cpp"
    "R4300/cop1_l.cpp"
    "R4300/cop1_s.cpp"
    "R4300/cop1_w.cpp"
    "R4300/disasm.cpp"
    "R4300/exception.cpp"
    "R4300/interrupt.cpp"
    "R4300/r4300.cpp"
    "R4300/recomp.cpp"
    "R4300/regimm.cpp"
    "R4300/rom.cpp"
    "R4300/special.cpp"
    "R4300/timers.cpp"
    "R4300/tracelog.cpp"
    "R4300/bc.cpp"

    "R4300/vcr.cpp"
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

# options: disable dynarec
if (${MUPEN64RR_ENABLE_DYNAREC})
    target_compile_definitions(Mupen64RR.Core PRIVATE "MUPEN64RR_ENABLE_DYNAREC")
    target_sources(Mupen64RR.Core PRIVATE
        "r4300/x86/assemble.cpp"
        "r4300/x86/gbc.cpp"
        "r4300/x86/gcop0.cpp"
        "r4300/x86/gcop1.cpp"
        "r4300/x86/gcop1_d.cpp"
        "r4300/x86/gcop1_helpers.cpp"
        "r4300/x86/gcop1_l.cpp"
        "r4300/x86/gcop1_s.cpp"
        "r4300/x86/gcop1_w.cpp"
        "r4300/x86/gr4300.cpp"
        "r4300/x86/gregimm.cpp"
        "r4300/x86/gspecial.cpp"
        "r4300/x86/gtlb.cpp"
        "r4300/x86/regcache.cpp"
        "r4300/x86/rjump.cpp"
    )
endif()

# FOR TESTING ONLY. DO NOT USE OTHERWISE.
add_library(Mupen64RR.Core._TestIncludes INTERFACE)
target_link_libraries(Mupen64RR.Core._TestIncludes INTERFACE
    Mupen64RR.Core
)
target_include_directories(Mupen64RR.Core._TestIncludes INTERFACE "../" "./")
