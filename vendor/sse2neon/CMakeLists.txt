cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(sse2neon
    DESCRIPTION "Translates SSE/SSE2/SSSE3 intrinsics to their ARM NEON equivalents."
    LANGUAGES CXX
)

add_library(sse2neon INTERFACE)
target_sources(sse2neon INTERFACE
    "sse2neon.h"
)
target_include_directories(sse2neon INTERFACE ".")
