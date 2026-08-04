#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]
add_library(Mupen64RR.Common INTERFACE
    "include/CommonPCH.hpp"
    "include/DummyPluginStub.hpp"
    "include/IOUtils.hpp"
    "include/MiscHelpers.hpp"
    "include/StrUtils.hpp"
    "include/VersionNameHelpers.hpp"
    "include/FNV1A.hpp"
)
set_target_properties(Mupen64RR.Common PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(Mupen64RR.Common INTERFACE include)
target_link_libraries(Mupen64RR.Common INTERFACE
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    vendor::xxhash64
)

# only MSVCRT implements C11 Annex K, libsafec is needed on other platforms
if (NOT WIN32)
    target_link_libraries(Mupen64RR.Common INTERFACE PkgConfig::libsafec)
endif()

if (MUPEN64RR_BUILD_WIN32)
    add_custom_target(Mupen64RR.Common.AutoUpdater ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${MUPEN64RR_OUT_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/update.py"
            "${MUPEN64RR_OUT_DIR}/update.py"
        COMMENT "Copying update.py to ${MUPEN64RR_OUT_DIR}"
        VERBATIM
    )
endif()
