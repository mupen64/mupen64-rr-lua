#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

# Toolchain file for MinGW cross-compilation from Linux to Windows.
# ============================================================================
# Usage examples:
#
#   64-bit:  cmake --preset mingw-linux-x64
#   32-bit:  cmake --preset mingw-linux-x86
#
# Both presets set CMAKE_C_COMPILER / CMAKE_CXX_COMPILER automatically.
# You can also use this toolchain manually:
#
#   cmake -B build/mingw64 \
#         -DCMAKE_TOOLCHAIN_FILE=toolchains/mingw-cross.cmake \
#         -DMINGW_TARGET=x86_64 \
#         -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
#         -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
#
# Dependencies:
#   Install mingw-w64 on your system.  On Debian/Ubuntu:
#     sudo apt install g++-mingw-w64-x86-64    # 64-bit
#     sudo apt install g++-mingw-w64-i686      # 32-bit (multiarch)
#
#   vcpkg (optional):  if you have a vcpkg installation with MinGW triplets,
#   set MUPEN64RR_VCPKG_TOOLCHAIN to its vcpkg.cmake path and the toolchain
#   will wire it in automatically.
# ============================================================================

set(CMAKE_SYSTEM_NAME Windows)

# ---- architecture selection --------------------------------------------------
if(NOT DEFINED MINGW_TARGET)
    set(MINGW_TARGET "x86_64" CACHE STRING "MinGW target (x86_64 or i686)")
endif()

if(MINGW_TARGET STREQUAL "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64 CACHE STRING "" FORCE)
    set(_mingw_prefix "x86_64-w64-mingw32")
elseif(MINGW_TARGET STREQUAL "i686")
    set(CMAKE_SYSTEM_PROCESSOR x86 CACHE STRING "" FORCE)
    set(_mingw_prefix "i686-w64-mingw32")
else()
    message(FATAL_ERROR "Invalid MINGW_TARGET '${MINGW_TARGET}'. Use x86_64 or i686.")
endif()

# ---- resource compiler -------------------------------------------------------
find_program(CMAKE_RC_COMPILER
    NAMES "${_mingw_prefix}-windres"
    REQUIRED
    DOC "MinGW resource compiler"
)

# ---- sysroot / find-root-path -----------------------------------------------
# Try to locate the MinGW sysroot automatically.
execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-sysroot
    OUTPUT_VARIABLE _mingw_sysroot
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(_mingw_sysroot AND EXISTS "${_mingw_sysroot}")
    set(CMAKE_SYSROOT "${_mingw_sysroot}" CACHE INTERNAL "")
    set(ENV{PKG_CONFIG_SYSROOT_DIR} "${_mingw_sysroot}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ---- optional vcpkg integration ----------------------------------------------
# Auto-detect vcpkg if not explicitly provided.
if(NOT DEFINED CACHE{MUPEN64RR_VCPKG_TOOLCHAIN})
    set(_vcpkg_root "")

    # 1. Explicit VCPKG_ROOT env var takes priority
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(_vcpkg_root "$ENV{VCPKG_ROOT}")
    endif()

    # 2. Try to derive from the vcpkg executable
    if(NOT _vcpkg_root)
        find_program(_vcpkg_exe NAMES vcpkg NO_CACHE)
        if(_vcpkg_exe)
            get_filename_component(_vcpkg_candidate "${_vcpkg_exe}" DIRECTORY)
            if(EXISTS "${_vcpkg_candidate}/scripts/buildsystems/vcpkg.cmake")
                set(_vcpkg_root "${_vcpkg_candidate}")
            endif()
        endif()
    endif()

    # 3. Common install locations
    if(NOT _vcpkg_root AND EXISTS "$ENV{HOME}/.local/share/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(_vcpkg_root "$ENV{HOME}/.local/share/vcpkg")
    endif()

    if(_vcpkg_root AND EXISTS "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
        set(MUPEN64RR_VCPKG_TOOLCHAIN "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake" CACHE INTERNAL "Location of vcpkg's toolchain file.")
        message(STATUS "MinGW cross toolchain: auto-detected vcpkg at ${_vcpkg_root}")
    else()
        message(WARNING "MinGW cross toolchain: vcpkg not found. Set VCPKG_ROOT or MUPEN64RR_VCPKG_TOOLCHAIN.")
    endif()
endif()

if(DEFINED CACHE{MUPEN64RR_VCPKG_TOOLCHAIN})
    if(EXISTS "${MUPEN64RR_VCPKG_TOOLCHAIN}")
        # Map our MINGW_TARGET to vcpkg's triplet naming convention.
        if(MINGW_TARGET STREQUAL "x86_64")
            set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "vcpkg MinGW triplet")
        else()
            set(VCPKG_TARGET_TRIPLET "x86-mingw-dynamic" CACHE STRING "vcpkg MinGW triplet")
        endif()
        include("${MUPEN64RR_VCPKG_TOOLCHAIN}")
        message(STATUS "MinGW cross toolchain: vcpkg enabled (triplet ${VCPKG_TARGET_TRIPLET})")
    endif()
endif()
