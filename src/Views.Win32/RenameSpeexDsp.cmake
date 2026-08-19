#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

if(EXISTS "${MUPEN64RR_OUT_DIR}/liblibspeexdsp.dll")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${MUPEN64RR_OUT_DIR}/liblibspeexdsp.dll"
            "${MUPEN64RR_OUT_DIR}/libspeexdsp.dll"
        RESULT_VARIABLE _speexdsp_copy_result
    )
endif()
