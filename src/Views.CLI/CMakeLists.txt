add_executable(Mupen64RR.Views.CLI
    "Main.hpp"
    "Main.cpp"
)
target_compile_features(Mupen64RR.Views.CLI PRIVATE cxx_std_23)
set_target_properties(Mupen64RR.Views.CLI PROPERTIES
    # output location
    OUTPUT_NAME "mupen64-cli"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
target_link_libraries(Mupen64RR.Views.CLI PRIVATE
    Mupen64RR.Common
    Mupen64RR.Core
)