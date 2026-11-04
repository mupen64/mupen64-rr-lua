cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(nontype_functional
    DESCRIPTION "Polyfill for std::move_only_function, from https://github.com/zhihaoy/nontype_functional v1.0.2."
    LANGUAGES CXX
)

add_library(nontype_functional INTERFACE)
target_sources(nontype_functional INTERFACE
    "include/std23/__functional_base.h"
    "include/std23/move_only_function.h"
)
target_include_directories(nontype_functional INTERFACE "include")
