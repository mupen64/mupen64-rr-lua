cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(lrucache11
    DESCRIPTION "A header only C++11 LRU Cache template class that allows you to define key, value and optionally the Map type."
    LANGUAGES CXX
)

add_library(lrucache11 INTERFACE)
target_sources(lrucache11 INTERFACE
    "LRUCache11.h"
)
target_include_directories(lrucache11 INTERFACE ".")