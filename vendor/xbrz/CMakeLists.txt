cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(xbrz
    DESCRIPTION "xBRZ image scaling library"
    LANGUAGES C
)

add_library(xbrz STATIC "xbrz.h" "xbrz_tools.h" "xbrz_config.h" "xbrz.cpp")
target_include_directories(xbrz PUBLIC ".")