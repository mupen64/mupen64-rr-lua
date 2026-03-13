cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(microlru
    DESCRIPTION "Basic LRU cache implementation."
    LANGUAGES CXX
)

add_library(microlru INTERFACE)
target_sources(microlru INTERFACE
    "microlru.h"
)
target_include_directories(microlru INTERFACE ".")