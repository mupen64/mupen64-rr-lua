add_library(Mupen64RR.Lua.TestLib MODULE "main.cpp")
target_compile_features(Mupen64RR.Lua.TestLib PRIVATE cxx_std_23)
set_target_properties(Mupen64RR.Lua.TestLib PROPERTIES
    OUTPUT_NAME "luatestlib"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_TEST_OUT_DIR}"
)
target_link_libraries(Mupen64RR.Lua.TestLib PRIVATE Lua::Lua)