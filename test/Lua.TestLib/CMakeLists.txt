add_library(Mupen64RR.Lua.TestLib MODULE "main.cpp")
set_target_properties(Mupen64RR.Lua.TestLib PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(Mupen64RR.Lua.TestLib PRIVATE Lua::Lua)