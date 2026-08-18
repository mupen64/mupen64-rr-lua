block()

# Linux: read off /etc/os-release
if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
    file(STRINGS "/etc/os-release" os_release_lines)
    foreach(line IN LISTS os_release_lines)
        if (NOT "${line}" MATCHES "^([A-Za-z0-9_]+)=(.+)$")
            continue()
        endif()
        set(var_name "OS_RELEASE_${CMAKE_MATCH_1}")

        set(var_value "${CMAKE_MATCH_2}")
        if ("${var_value}" MATCHES "^\"(.+)\"$")
            # additionally handle escapes
            set(var_value "${CMAKE_MATCH_1}")
            string(REGEX REPLACE "\\\\([\$\'\"`\\\\])" "\\1" var_value "${var_value}")
        endif()

        set("${var_name}" "${var_value}" PARENT_SCOPE)
        # message("${var_name} = ${var_value}")
    endforeach()
endif()
endblock()

# workaround for https://github.com/mupen64/mupen64-rr-lua/issues/970
# provide spdlog via FetchContent
if (
    ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux") 
    AND ("${OS_RELEASE_ID} ${OS_RELEASE_ID_LIKE}" MATCHES "ubuntu")
)
    block()
    include(FetchContent)
    message(STATUS "Applying workaround for issue #970 (fetching spdlog 1.11.0)")
    set(CMAKE_CXX_STANDARD 17)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG ad0e89cbfb4d0c1ce4d097e134eb7be67baebb36 # v1.11.0
        OVERRIDE_FIND_PACKAGE
    )
    endblock()
endif()