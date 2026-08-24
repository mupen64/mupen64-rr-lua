project(glad
    DESCRIPTION "Automatically-generated OpenGL bindings (OpenGL 4.6, OpenGL ES 3.2)"
    LANGUAGES C
)

add_library(glad STATIC
    "include/glad/glad.h"
    "include/KHR/khrplatform.h"
    "src/glad.c"
)
target_include_directories(glad PUBLIC "include")