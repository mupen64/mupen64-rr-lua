cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(hqx
    DESCRIPTION "hqx image scaling library"
    LANGUAGES C
)

add_library(hqx STATIC "common.h" "hqx.h" "init.c" "hq2x.c" "hq3x.c" "hq4x.c")
target_include_directories(hqx PUBLIC ".")

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hqx PRIVATE -w)
endif()
