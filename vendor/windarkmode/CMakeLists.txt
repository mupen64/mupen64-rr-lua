cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(windarkmode
    DESCRIPTION "Single-header Win32 dark mode library with a sane and modern API surface"
    LANGUAGES CXX
)

add_library(windarkmode INTERFACE)
target_sources(windarkmode INTERFACE
    "WinDarkMode.h"
)
target_include_directories(windarkmode INTERFACE ".")