#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(mupen64-rr
    VERSION 1.5.0
    DESCRIPTION "Powerful N64 TASing emulator with Lua scripting support"
    LANGUAGES C CXX
)

# Options
# ===========================================

# The dynarec only supports x86 and x86_64; building for any other
# architecture (e.g. ARM64) errors out (see src/Core/CMakeLists.txt).
set(MUPEN64RR_ENABLE_DYNAREC ON CACHE BOOL "If on, enables the dynamic recompiler.")

# The Windows view can, of course, only be compiled on Windows.
# This is in place to allow future frontends to be written separately.
set(MUPEN64RR_BUILD_WIN32 OFF CACHE BOOL "If on, enables the Win32 view and plugins.")

if(MUPEN64RR_BUILD_WIN32 AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    message(FATAL_ERROR "Unsupported platform for the Win32 frontend!")
endif()

# This is the cross-platform command-line frontend, intended primarily as an integration test between plugins and frontend code.
set(MUPEN64RR_BUILD_QT6 OFF CACHE BOOL "If on, enables the Qt6 view (experimental)")

# Basic checks and setup
# ===========================================

# initial checks
if(NOT WIN32)
    message(WARNING "Building on platforms other than Windows is experimental. You're on your own, good luck.")
endif()

# Check generator type
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(is_multi_config)
    message(WARNING "These build files are not designed to handle multi-config generators. You're on your own, good luck.")
endif()

# set default build type
if(NOT CMAKE_BUILD_TYPE AND NOT is_multi_config)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "The build type to use with CMake. [Debug;Release;MinSizeRel;RelWithDebInfo]")
endif()

# version suffix
if(DEFINED ENV{VERSION_SUFFIX})
    set(MUPEN64RR_VERSION_SUFFIX "$ENV{VERSION_SUFFIX}")
else()
    set(MUPEN64RR_VERSION_SUFFIX "")
endif()

# nightly build flag
if(DEFINED ENV{NIGHTLY})
    set(MUPEN64RR_NIGHTLY "$ENV{NIGHTLY}")
else()
    set(MUPEN64RR_NIGHTLY "0")
endif()

message(STATUS "Full version name: ${PROJECT_VERSION}${MUPEN64RR_VERSION_SUFFIX}")
message(STATUS "Nightly build: ${MUPEN64RR_NIGHTLY}")

# build output
set(MUPEN64RR_OUT_DIR "${PROJECT_BINARY_DIR}/out/")
set(MUPEN64RR_PLUGIN_OUT_DIR "${PROJECT_BINARY_DIR}/out/plugin/")
set(MUPEN64RR_TEST_OUT_DIR "${PROJECT_BINARY_DIR}/test/out/")

# Libraries
# =======================================

# make sure pkg-config is available
find_package(PkgConfig REQUIRED)

# Core cross-platform dependencies
find_package(Catch2 CONFIG REQUIRED)
find_package(libdeflate CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
pkg_check_modules(lz4 IMPORTED_TARGET GLOBAL "liblz4")

# libsafec (required on most runtimes to support C11 Annex K)
if(NOT WIN32)
    pkg_check_modules(libsafec IMPORTED_TARGET GLOBAL "libsafec")
endif()

# Lua, because Lua sucks
find_package(Lua REQUIRED)

if(LUA_FOUND AND NOT TARGET Lua::Lua)
    add_library(Lua::Lua INTERFACE IMPORTED)
    set_target_properties(
        Lua::Lua
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
    )
endif()

# Windows-only dependencies
if(MUPEN64RR_BUILD_WIN32)
    find_package(spdlog CONFIG REQUIRED)
    find_package(glew CONFIG REQUIRED)

    pkg_check_modules(speexdsp IMPORTED_TARGET GLOBAL "speexdsp")
    pkg_check_modules(libav IMPORTED_TARGET GLOBAL "libavformat" "libavcodec" "libavutil")
endif()

if(MUPEN64RR_BUILD_QT6)
    find_package(Qt6 CONFIG REQUIRED COMPONENTS
        Core Gui Widgets
        Qml Quick QuickControls2
    )
endif()

# Utility functions
# =======================================

#[=======================[
Copies all vcpkg dependencies into the target's folder.
]=======================]
function(mupen64rr_copy_vcpkg_deps target)
    if(NOT(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND DEFINED VCPKG_INSTALLED_DIR))
        return()
    endif()

    add_custom_command(
        TARGET "${target}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin" "$<TARGET_FILE_DIR:${target}>"
        VERBATIM
    )
endfunction()

#[=======================[
Copies MinGW runtime DLLs (libgcc, libstdc++, libwinpthread) to the target's
output directory. Only active when cross-compiling with MinGW.
]=======================]
function(mupen64rr_copy_mingw_runtime target)
    if(NOT MINGW)
        return()
    endif()

    # Derive the MinGW prefix from the compiler name.
    # e.g. /bin/x86_64-w64-mingw32-g++ -> x86_64-w64-mingw32
    get_filename_component(_compiler_name "${CMAKE_CXX_COMPILER}" NAME)
    string(REGEX REPLACE "-g\\+\\+.*$" "" _mingw_prefix "${_compiler_name}")
    set(_mingw_bindir "/usr/${_mingw_prefix}/bin")

    add_custom_command(
        TARGET "${target}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${_mingw_bindir}/libgcc_s_seh-1.dll"
        "${_mingw_bindir}/libstdc++-6.dll"
        "${_mingw_bindir}/libwinpthread-1.dll"
        "$<TARGET_FILE_DIR:${target}>"
        VERBATIM
    )
endfunction()

# CMake modules
# =======================================
include(CTest)
include(Catch)

# Global build settings
# =======================================

# Avoid "lib" prefixes for plugins (e.g. libTASVideo.dll instead of TASVideo.dll).
set(CMAKE_SHARED_MODULE_PREFIX "")

if(WIN32)
    # Use UTF-16 versions of functions
    add_compile_definitions(UNICODE _UNICODE)
endif()

if(MSVC)
    # Include linker options to:
    # - disable Address-Space Layout Randomization (ASLR)
    # - enable the program to use pointers larger than 2GB
    # - disable the generation of a manifest file (since it's pulled in internally via the resource script)
    add_link_options(/DYNAMICBASE:NO /LARGEADDRESSAWARE /MANIFEST:NO)

    # Data Execution Prevention (DEP) is left enabled on 64-bit, where it can't be turned
    # off anyway and where the x86_64 dynarec allocates its code buffers as executable. Only
    # the 32-bit build opts out, as it has historically relied on that.
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_link_options(/NXCOMPAT:NO)
    endif()

    # Enable UTF-8 in Windows-adjacent APIs where possible
    add_compile_options(/utf-8)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i[346]86|x86_64|AMD64|amd64|x64)$" AND CMAKE_C_COMPILER_ID MATCHES "^GCC$|Clang$")
    # Enable SSSE3 instructions where applicable
    add_compile_options(-mssse3)
endif()

# define _DEBUG for debug builds
add_compile_definitions("$<$<CONFIG:Debug>:_DEBUG>")

# Subdirectories
# ===========================================

# vendored libraries
add_subdirectory(vendor)

# subprojects
add_subdirectory(src)
