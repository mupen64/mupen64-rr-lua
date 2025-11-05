#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(mupen64-rr
  VERSION 1.3.0
  DESCRIPTION "Powerful N64 TASing emulator with Lua scripting support"
  LANGUAGES C CXX
)

message("The C compiler is: ${CMAKE_C_COMPILER}")
message("The C++ compiler is: ${CMAKE_CXX_COMPILER}")

# initial checks
if(NOT WIN32)
  message(WARNING "Building on platforms other than Windows is experimental. You're on your own, good luck.")
endif()

# version suffix
if (DEFINED ENV{VERSION_SUFFIX})
  set(MUPEN64RR_VERSION_SUFFIX "$ENV{VERSION_SUFFIX}")
else()
  set(MUPEN64RR_VERSION_SUFFIX "")
endif()

message(STATUS "Full version name: ${PROJECT_VERSION}${MUPEN64RR_VERSION_SUFFIX}")

# build output
set(MUPEN64RR_OUT_DIR "${PROJECT_BINARY_DIR}/out/")
set(MUPEN64RR_PLUGIN_OUT_DIR "${PROJECT_BINARY_DIR}/out/plugin/")
set(MUPEN64RR_TEST_OUT_DIR "${PROJECT_BINARY_DIR}/test/out/")

# default build type
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(NOT CMAKE_BUILD_TYPE AND NOT is_multi_config)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "The build type to use with CMake. [Debug;Release;MinSizeRel;RelWithDebInfo]")
endif()

# Libraries
# =======================================

# pkg-config libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(speexdsp IMPORTED_TARGET GLOBAL "speexdsp")

# cmake libraries
find_package(libdeflate CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

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

# CMake modules
# =======================================
include(CTest)
include(Catch)

# Extra config options for specific compilers/OSes
# ===========================================

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # Include linker options to:
  # - disable Address-Space Layout Randomization (ASLR)
  # - disable Windows Data Execution Prevention (DEP)
  # - enable the program to use pointers larger than 2GB
  # - disable the generation of a manifest file (since it's pulled in internally via the resource script)
  add_link_options(/DYNAMICBASE:NO /NXCOMPAT:NO /LARGEADDRESSAWARE /MANIFEST:NO)

  # Include compile options to enable AVX2 on x64 builds
  if ("${VCPKG_TARGET_TRIPLET}" MATCHES "^x64")
    add_compile_options(/arch:AVX2)
  endif()
  
  # Enable UTF-8 in Windows-adjacent APIs where possible
  add_compile_options(/utf-8)
endif()
if (WIN32)
  # Use UTF-16 versions of functions
  add_compile_definitions(UNICODE _UNICODE)
endif()

# The dynamic recompiler will not work correctly on x86_64, so it is completely disabled for safety's sake.
set(MUPEN64RR_ENABLE_DYNAREC ON CACHE BOOL "If on, enables the dynamic recompiler.")
if (NOT "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86|i[346]86)$")
  set(MUPEN64RR_ENABLE_DYNAREC OFF CACHE BOOL "If on, enables the dynamic recompiler. (FORCE-DISABLED)" FORCE)
endif()

# define _DEBUG for debug builds
add_compile_definitions("$<$<CONFIG:Debug>:_DEBUG>")



# Subdirectories
# ===========================================

# vendored libraries
add_subdirectory(vendor)

# subprojects
add_subdirectory(src)

# tests
add_subdirectory(test)