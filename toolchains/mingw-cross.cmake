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
if(DEFINED CACHE{MUPEN64RR_VCPKG_TOOLCHAIN})
    if(EXISTS "${MUPEN64RR_VCPKG_TOOLCHAIN}")
        # Map our MINGW_TARGET to vcpkg's triplet naming convention.
        if(MINGW_TARGET STREQUAL "x86_64")
            set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "vcpkg MinGW triplet")
        else()
            set(VCPKG_TARGET_TRIPLET "x86-mingw-static" CACHE STRING "vcpkg MinGW triplet")
        endif()
        include("${MUPEN64RR_VCPKG_TOOLCHAIN}")
        message(STATUS "MinGW cross toolchain: vcpkg enabled (triplet ${VCPKG_TARGET_TRIPLET})")
    endif()
endif()
