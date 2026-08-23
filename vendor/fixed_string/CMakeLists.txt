cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(fixed_string
    DESCRIPTION "C++ library that provides a basic_fixed_string template."
    LANGUAGES CXX
)

add_library(fixed_string INTERFACE)
target_sources(fixed_string INTERFACE
    "fixed_string.hpp"
)
target_include_directories(fixed_string INTERFACE ".")
target_compile_features(fixed_string INTERFACE cxx_std_20)