#[=============================================================================[
MIT License

Copyright (c) 2024 Stephen Karavos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
#]=============================================================================]
#
# Source:
#   <https://github.com/skaravos/CMakeWinDeployQt>
#
# Acknowlegements:
#   Original idea and concept was taken from nitroshare-desktop's DeployQt.cmake
#   <https://github.com/nitroshare/nitroshare-desktop/blob/f4feebef29d9d3985d1699ab36f0fac59d3df7da/cmake/DeployQt.cmake>
#
cmake_minimum_required(VERSION 3.15...4.2)

#[==[

  Sets up commands to automatically call the 'windeployqt.exe' deployment tool.

  This will automatically bundle the Qt runtime dlls into to the target's output
  directory after compilation. It will also automatically bundle the Qt dlls
  into the given directory during the install step.

  This function uses cmake's own functionality for 'installing' the runtime
  files so it should work properly with CPack.

  NOTE: This function only works with windeployqt.exe from Qt5
        For a function that works with Qt6 see: windeployqt6()

  NOTE: On non-windows platforms, this function does nothing

  NOTE: If the provided target is not directly linked to any shared Qt libraries
        this function will fail during build-time with the error:
          "<target> does not seem to be a Qt executable"
        This can happen if you link against static Qt libraries.
        To guard against potential problems, you can wrap the call like so:
          get_target_property(_qt5_type Qt5::Core TYPE)
          if (${_qt5_type} MATCHES "SHARED")
            windeployqt5(TARGET ${PROJECT_NAME})
          endif()

  function signature:

      windeployqt5(TARGET <target>
        [DIRECTORY <directory>]
        [COMPONENT <component>]
        [VERBOSE]
        [DEPLOY_TO_BUILD_DIR]
        [INCLUDE_REGEXES <regexes>...]
        [EXCLUDE_REGEXES <regexes>...]
        [ARGS <args>...]
      )

  required parameters:

    TARGET <target>
      - Name of a valid existing cmake target, the path to this compiled target
        is passed to windeployqt.exe to compute the Qt5 dependencies.

  optional parameters:

    DIRECTORY <directory>
      - A custom path to install the dependencies.
      - Must be a relative path (treated as relative to CMAKE_INSTALL_PREFIX)
      - [default: .]

    COMPONENT <component>
      - Forwarded verbatim to the install() rule of the computed dependencies
      - [default: Unspecified]

    VERBOSE
      - If provided, windeployqt.exe will be run in verbose mode (--verbose 1)

    DEPLOY_TO_BUILD_DIR
      - If provided, a POST_BUILD cmake custom_command will be added to run
        windeployqt and deploy the libraries to the target's binary directory.
      - This can be useful if you wish to run or test the target executable
        directly from the project's build directory prior to installation.
      - [default: disabled, windeployqt is only invoked during install step]

    INCLUDE_REGEXES <regexes>...
      - A list of regular expressions that will be matched against all the
        files that would be deployed by windeployqt with the current
        user-provided ARGS. If a file matches any include regex it will
        always be deployed, regardless if it matches an exclude regex.
      - Can be used alongside EXCLUDE_REGEXES to select specific plugins or
        libraries to deploy, without relying on windeployqt's command-line.

    EXCLUDE_REGEXES <regexes>...
      - A list of regular expressions that will be matched against all the
        files that would be deployed by windeployqt with the current
        user-provided ARGS. If a file matches any exclude regex it will
        not be deployed, unless it matches a provided include regex.
      - Can be used alongside INCLUDE_REGEXES to select specific plugins or
        libraries to deploy, without relying on windeployqt's command-line.

    ARGS <args>...
      - A list of additional args passed directly to windeployqt.exe

#]==]
function(windeployqt5)
  if (NOT WIN32)
    return()
  endif()

  # --------------------------
  #  locate windeployqt (Qt5)
  # --------------------------

  set(_qt_bin_dir)
  if (TARGET Qt5::qmake)
    get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
  endif()
  find_program(WINDEPLOYQT5_EXE windeployqt HINTS "${_qt_bin_dir}")
  mark_as_advanced(WINDEPLOYQT5_EXE)

  if(NOT WINDEPLOYQT5_EXE)
    message(FATAL_ERROR "windeployqt.exe (Qt5) not found")
  endif()

  # --------------------------
  #  parse arguments
  # --------------------------

  set(_options  VERBOSE DEPLOY_TO_BUILD_DIR)
  set(_args     TARGET DIRECTORY COMPONENT)
  set(_listargs ARGS INCLUDE_REGEXES EXCLUDE_REGEXES)
  cmake_parse_arguments(PARSE_ARGV 0 arg "${_options}" "${_args}" "${_listargs}")

  message(STATUS "windeployqt5('${arg_TARGET}')")

  foreach(_arg IN LISTS arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "unknown argument: ${_arg}")
  endforeach()

  foreach(_arg IN LISTS arg_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "invalid argument: ${_arg} cannot be provided without a value")
  endforeach()

  # --------------------------
  #  call implementation func
  # --------------------------

  __windeployqt_impl(
    WINDEPLOYQT_EXECUTABLE
      ${WINDEPLOYQT5_EXE}
    QT_BIN_DIR
      ${_qt_bin_dir}
    TARGET
      ${arg_TARGET}
    DIRECTORY
      ${arg_DIRECTORY}
    COMPONENT
      ${arg_COMPONENT}
    VERBOSE
      ${arg_VERBOSE}
    DEPLOY_TO_BUILD_DIR
      ${arg_DEPLOY_TO_BUILD_DIR}
    INCLUDE_REGEXES
      ${arg_INCLUDE_REGEXES}
    EXCLUDE_REGEXES
      ${arg_EXCLUDE_REGEXES}
    ARGS
      ${arg_ARGS}
  )
  message(STATUS "windeployqt5('${arg_TARGET}') - success")
endfunction()



#[==[
  Sets up commands to automatically call the 'windeployqt6.exe' deployment tool.

  This will automatically bundle the Qt runtime dlls into to the target's output
  directory after compilation. It will also automatically bundle the Qt dlls
  into the given directory during the install step.

  This function uses cmake's own functionality for 'installing' the runtime
  files so it should work properly with CPack.

  NOTE: This function only works with windeployqt6.exe included with Qt6
        For a function that works with Qt5 see: windeployqt5()

  NOTE: On non-windows platforms, this function does nothing

  NOTE: If the provided target is not directly linked to any shared Qt libraries
        this function will fail during build-time with the error:
          "<target> does not seem to be a Qt executable"
        This can happen if you link against static Qt libraries.
        To guard against potential problems, you can wrap the call like so:
          get_target_property(_qt6_type Qt6::Core TYPE)
          if (${_qt6_type} MATCHES "SHARED")
            windeployqt6(TARGET ${PROJECT_NAME})
          endif()

  function signature:

      windeployqt6(TARGET <target>
        [DIRECTORY <directory>]
        [COMPONENT <component>]
        [VERBOSE]
        [DEPLOY_TO_BUILD_DIR]
        [INCLUDE_REGEXES <regexes>...]
        [EXCLUDE_REGEXES <regexes>...]
        [ARGS <args>...]
      )

  required parameters:

    TARGET <target>
      - Name of a valid existing cmake target, the path to this compiled target
        is passed to windeployqt6.exe to compute the Qt6 dependencies.

  optional parameters:

    DIRECTORY <directory>
      - A custom path to install the dependencies.
      - Must be a relative path (treated as relative to CMAKE_INSTALL_PREFIX)
      - [default: .]

    COMPONENT <component>
      - Forwarded verbatim to the install rule of the computed dependencies
      - [default: Unspecified]

    VERBOSE
      - If provided, windeployqt6.exe will be run in verbose mode (--verbose 1)

    DEPLOY_TO_BUILD_DIR
      - If provided, a POST_BUILD cmake custom_command will be added to run
        windeployqt and deploy the libraries to the target's binary directory.
      - This can be useful if you wish to run or test the target executable
        directly from the project's build directory prior to installation.
      - [default: disabled, windeployqt is only invoked during install step]

    INCLUDE_REGEXES <regexes>...
      - A list of regular expressions that will be matched against all the
        files that would be deployed by windeployqt with the current
        user-provided ARGS. If a file matches any include regex it will
        always be deployed, regardless if it matches an exclude regex.
      - Can be used alongside EXCLUDE_REGEXES to select specific plugins or
        libraries to deploy, without relying on windeployqt's command-line.

    EXCLUDE_REGEXES <regexes>...
      - A list of regular expressions that will be matched against all the
        files that would be deployed by windeployqt with the current
        user-provided ARGS. If a file matches any exclude regex it will
        not be deployed, unless it matches a provided include regex.
      - Can be used alongside INCLUDE_REGEXES to select specific plugins or
        libraries to deploy, without relying on windeployqt's command-line.

    ARGS <args>...
      - A list of additional args passed directly to windeployqt6.exe
#]==]
function(windeployqt6)
  if (NOT WIN32)
    return()
  endif()

  # --------------------------
  #  locate windeployqt6
  # --------------------------

  set(_qt_bin_dir_hints)
  if (TARGET Qt6::qmake)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    list(APPEND _qt_bin_dir_hints "${_qt_bin_dir}")
  endif()
  find_program(WINDEPLOYQT6_EXE NAMES windeployqt6 windeployqt HINTS "${_qt_bin_dir_hints}")
  mark_as_advanced(WINDEPLOYQT6_EXE)

  if(NOT WINDEPLOYQT6_EXE)
    message(FATAL_ERROR "windeployqt6.exe (Qt6) not found")
  endif()

  # --------------------------
  #  parse arguments
  # --------------------------

  set(_options  VERBOSE DEPLOY_TO_BUILD_DIR)
  set(_args     TARGET DIRECTORY COMPONENT)
  set(_listargs ARGS INCLUDE_REGEXES EXCLUDE_REGEXES)
  cmake_parse_arguments(PARSE_ARGV 0 arg "${_options}" "${_args}" "${_listargs}")

  message(STATUS "windeployqt6('${arg_TARGET}')")

  foreach(_arg IN LISTS arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "unknown argument: ${_arg}")
  endforeach()

  foreach(_arg IN LISTS arg_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "invalid argument: ${_arg} cannot be provided without a value")
  endforeach()

  # --------------------------
  #  call implementation func
  # --------------------------

  __windeployqt_impl(
    WINDEPLOYQT_EXECUTABLE
      ${WINDEPLOYQT6_EXE}
    QT_BIN_DIR
      ${_qt_bin_dir}
    TARGET
      ${arg_TARGET}
    DIRECTORY
      ${arg_DIRECTORY}
    COMPONENT
      ${arg_COMPONENT}
    VERBOSE
      ${arg_VERBOSE}
    DEPLOY_TO_BUILD_DIR
      ${arg_DEPLOY_TO_BUILD_DIR}
    INCLUDE_REGEXES
      ${arg_INCLUDE_REGEXES}
    EXCLUDE_REGEXES
      ${arg_EXCLUDE_REGEXES}
    ARGS
      ${arg_ARGS}
  )
  message(STATUS "windeployqt6('${arg_TARGET}') - success")
endfunction()


#[==[
  Versionless wrapper for the windeployqt5() and windeployqt6() functions
  useful for projects that still support compiling with either Qt5 or Qt6

  Detects the current version of windeployqt.exe using the --version param and
  then invokes the appropriate versioned function based on the result

  function signature:

      windeployqt(args...
        [WINDEPLOYQT5 args...]
        [WINDEPLOYQT6 args...]
      )

  All args provided to this function are forwarded verbatim to one of the
  abovementioned functions depending on the version of windeployqt.exe that is
  detected. See windeployqt() and/or windeployqt6() for supported arguments.

  NOTE: only supports windeployqt.exe version 5 and version 6

  optional parameters:

    WINDEPLOYQT5 args...
      - Arguments that should be only be forwarded to windeployqt5()

    WINDEPLOYQT6 args...
      - Arguments that should be only be forwarded to windeployqt6()

  example:

    windeployqt(TARGET MyTarget
      DIRECTORY .
      COMPONENT QtLibraries
      INCLUDE_REGEXES "imageformats/qjpeg.*"
      EXCLUDE_REGEXES "imageformats/.*"
      ARGS --no-translations
      WINDEPLOYQT5 ARGS --no-angle
      WINDEPLOYQT6 ARGS --skip-plugin-types "generic"
    )
#]==]
function(windeployqt)
  if (NOT WIN32)
    return()
  endif()

  # --------------------------
  #  locate copy of windeployqt.exe
  # --------------------------

  # Try to use any defined qmake target as a location hint
  set(_qt_bin_dir_hints)
  set(_qmake_tgts Qt::qmake Qt5::qmake Qt6::qmake)
  foreach(_tgt IN LISTS _qmake_tgts)
    if (TARGET ${_tgt})
      get_target_property(_qmake_executable ${_tgt} IMPORTED_LOCATION)
      get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
      list(APPEND _qt_bin_dir_hints "${_qt_bin_dir}")
      break()
    endif()
  endforeach()

  find_program(WINDEPLOYQT_EXE windeployqt HINTS "${_qt_bin_dir_hints}")
  mark_as_advanced(WINDEPLOYQT_EXE)

  if(NOT WINDEPLOYQT_EXE)
    message(FATAL_ERROR "windeployqt.exe not found")
  endif()

  # --------------------------
  #  determine version of windeployqt
  # --------------------------

  execute_process(
    COMMAND "${WINDEPLOYQT_EXE}" --version
    OUTPUT_VARIABLE _windeploy_stdout
    ERROR_VARIABLE  _windeploy_stderr
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (_windeploy_stdout MATCHES [[Qt Deploy Tool ([0-9]+(\.[0-9]+(\.[0-9]+)?)?)]])
    set(_windeploy_version ${CMAKE_MATCH_1})
  elseif (_windeploy_stdout MATCHES [[^([0-9]+(\.[0-9]+(\.[0-9]+)?)?)$]])
    set(_windeploy_version ${CMAKE_MATCH_1})
  else()
    message(FATAL_ERROR "failed to extract the version of windeployqt.exe with --version")
  endif()

  # --------------------------
  #  parse arguments
  # --------------------------

  cmake_parse_arguments(PARSE_ARGV 0 arg "" "" "WINDEPLOYQT5;WINDEPLOYQT6")

  # --------------------------
  #  call appropriate func
  # --------------------------

  if (_windeploy_version VERSION_GREATER_EQUAL "6.0.0")
    windeployqt6(${arg_UNPARSED_ARGUMENTS} ${arg_WINDEPLOYQT6})
  elseif(_windeploy_version VERSION_GREATER_EQUAL "5.0.0")
    windeployqt5(${arg_UNPARSED_ARGUMENTS} ${arg_WINDEPLOYQT5})
  else()
    message(FATAL_ERROR "unsupported version of windeployqt.exe [${_windeploy_version}]")
  endif()
endfunction()


#[=============================================================================[
                INTERNAL FUNCTION DO NOT CALL DIRECTLY
#]=============================================================================]
function(__windeployqt_impl)
  if (NOT WIN32)
    return()
  endif()

  # --------------------------
  #  parse arguments
  # --------------------------

  set(_options)
  set(_args
    WINDEPLOYQT_EXECUTABLE
    QT_BIN_DIR
    TARGET
    DIRECTORY
    COMPONENT
    VERBOSE
    DEPLOY_TO_BUILD_DIR
  )
  set(_listargs
    ARGS
    INCLUDE_REGEXES
    EXCLUDE_REGEXES
  )
  cmake_parse_arguments(PARSE_ARGV 0 arg "${_options}" "${_args}" "${_listargs}")

  message(DEBUG "__windeployqt_impl('${arg_TARGET}')")
  message(DEBUG "  arg_DEPLOY_TO_BUILD_DIR:[${arg_DEPLOY_TO_BUILD_DIR}]")
  message(DEBUG "  arg_WINDEPLOYQT_EXECUTABLE:[${arg_WINDEPLOYQT_EXECUTABLE}]")
  message(DEBUG "  arg_QT_BIN_DIR:[${arg_QT_BIN_DIR}]")
  message(DEBUG "  arg_TARGET:[${arg_TARGET}]")
  message(DEBUG "  arg_DIRECTORY:[${arg_DIRECTORY}]")
  message(DEBUG "  arg_COMPONENT:[${arg_COMPONENT}]")
  message(DEBUG "  arg_VERBOSE:[${arg_VERBOSE}]")
  message(DEBUG "  arg_INCLUDE_REGEXES:[${arg_INCLUDE_REGEXES}]")
  message(DEBUG "  arg_EXCLUDE_REGEXES:[${arg_EXCLUDE_REGEXES}]")
  message(DEBUG "  arg_ARGS:[${arg_ARGS}]")

  foreach(_arg IN LISTS arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "unknown argument: ${_arg}")
  endforeach()

  if (NOT arg_WINDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "missing argument: WINDEPLOYQT_EXECUTABLE")
  endif()

  if (NOT arg_TARGET)
    message(FATAL_ERROR "missing argument: TARGET")
  endif()

  if (NOT TARGET "${arg_TARGET}")
    message(FATAL_ERROR "invalid argument: TARGET is not a valid target [${arg_TARGET}]")
  endif()

  if (arg_DIRECTORY)
    get_filename_component(_dir_absolute "${arg_DIRECTORY}" ABSOLUTE)
    if (_dir_absolute STREQUAL arg_DIRECTORY)
      message(FATAL_ERROR "invalid argument: DIRECTORY can't be an absolute path.")
    endif()
    set(_install_directory "\${CMAKE_INSTALL_PREFIX}/${arg_DIRECTORY}")
  else()
    set(_install_directory "\${CMAKE_INSTALL_PREFIX}")
  endif()

  if (arg_COMPONENT)
    set(_install_component "${arg_COMPONENT}")
  else()
    set(_install_component "Unspecified")
  endif()

  if (arg_QT_BIN_DIR)
    set(_qt_bin_dir "${arg_QT_BIN_DIR}")
  else()
    get_filename_component(_qt_bin_dir "${arg_WINDEPLOYQT_EXECUTABLE}" DIRECTORY)
  endif()

  if (arg_VERBOSE)
    set(_arg_verbosity 1)
  else()
    set(_arg_verbosity 0)
  endif()

  # --------------------------
  #  create a staging directory unique to this call of windeployqt
  # --------------------------

  string(SHA256 _argn_hash "${ARGN}")
  string(SUBSTRING "${_argn_hash}" 0 10 _argn_hash_short) # keep first 10 chars
  string(MAKE_C_IDENTIFIER "${arg_TARGET}" _tgt_name_clean)
  set(_base_staging_dir "${PROJECT_BINARY_DIR}/windeployqt_stage")
  set(_staging_directory "${_base_staging_dir}/${_tgt_name_clean}_${_argn_hash_short}")
  file(MAKE_DIRECTORY "${_staging_directory}")

  # --------------------------
  #  generate the windeployqt.cmake script
  # --------------------------

  set(_windeployqt_cmake "${_base_staging_dir}/windeployqt_script.cmake")
  __create_windeployqt_script_cmake(${_windeployqt_cmake})

  # --------------------------
  #  add command to deploy qt dependencies in build directory
  # --------------------------

  if (arg_DEPLOY_TO_BUILD_DIR)
    set(_postbuild_custom_command_cmake "${_staging_directory}_$<CONFIG>_postbuild.cmake")

    if (NOT EXISTS "${_postbuild_custom_command_cmake}")
    file(GENERATE OUTPUT "${_postbuild_custom_command_cmake}" CONTENT "
#
# WARNING: this file is auto-generated by WinDeployQt.cmake, do not edit
#
message(STATUS \"Invoking windeployqt_script.cmake to deploy Qt to build directory of ${arg_TARGET}\")
execute_process(
  RESULT_VARIABLE _exit_code
  COMMAND
    \"${CMAKE_COMMAND}\"
      \"-DCMAKE_MESSAGE_LOG_LEVEL=${CMAKE_MESSAGE_LOG_LEVEL}\"
      \"-D_WINDEPLOYQT_EXECUTABLE=${arg_WINDEPLOYQT_EXECUTABLE}\"
      \"-D_QT_BIN_DIR=${_qt_bin_dir}\"
      \"-D_VERBOSITY=${_arg_verbosity}\"
      \"-D_WINDEPLOY_ARGS=${arg_ARGS}\"
      \"-D_TARGET_FILE=$<TARGET_FILE:${arg_TARGET}>\"
      \"-D_INCLUDE_REGEXES=${arg_INCLUDE_REGEXES}\"
      \"-D_EXCLUDE_REGEXES=${arg_EXCLUDE_REGEXES}\"
      \"-D_STAGING_DIRECTORY=${_staging_directory}\"
      \"-D_INSTALL_DIRECTORY=$<TARGET_FILE_DIR:${arg_TARGET}>\"
      \"-D_INSTALL_MESSAGE=MESSAGE_NEVER\"
      -P \"${_windeployqt_cmake}\"
)
if (NOT _exit_code EQUAL 0)
  message(FATAL_ERROR \"failed\")
endif()
"
    )
    endif()
    # NOTE:
    #  must invoke POST_BUILD command indirectly via a script because ninja.exe uses nested calls
    #  to cmd.exe /c '' when invoking PRE_BUILD, PRE_LINK, or POST_BUILD custom commands.
    #  The re-entrant nested calls obviously cause things to get quite messed up even with VERBATIM.
    #  ex. For a 'POST_BUILD', Ninja might run something that looks like:
    #  cmd.exe /c "cd . && cmake -E link.exe ... && cd . && cmd.exe /c "the_custom_command ...""
    #  ref: <https://discourse.cmake.org/t/add-custom-command-problem-with-quotes-and-spaces/11014>
    add_custom_command(TARGET "${arg_TARGET}" POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -P "${_postbuild_custom_command_cmake}"
      VERBATIM
    )
  endif()

  # --------------------------
  #  add command to deploy qt dependencies in install directory
  # --------------------------

  if (CMAKE_INSTALL_MESSAGE MATCHES "^LAZY$|^ALWAYS$|^NEVER$")
    set(_install_message "MESSAGE_${CMAKE_INSTALL_MESSAGE}")
  endif()

  # runs windeployqt during installation
  install(CODE "
    message(STATUS \"Invoking windeployqt_script.cmake to deploy Qt to install directory\")
    execute_process(
      COMMAND
        \"${CMAKE_COMMAND}\"
          \"-DCMAKE_MESSAGE_LOG_LEVEL=${CMAKE_MESSAGE_LOG_LEVEL}\"
          \"-D_WINDEPLOYQT_EXECUTABLE=${arg_WINDEPLOYQT_EXECUTABLE}\"
          \"-D_QT_BIN_DIR=${_qt_bin_dir}\"
          \"-D_VERBOSITY=${_arg_verbosity}\"
          \"-D_WINDEPLOY_ARGS=${arg_ARGS}\"
          \"-D_TARGET_FILE=$<TARGET_FILE:${arg_TARGET}>\"
          \"-D_INCLUDE_REGEXES=${arg_INCLUDE_REGEXES}\"
          \"-D_EXCLUDE_REGEXES=${arg_EXCLUDE_REGEXES}\"
          \"-D_STAGING_DIRECTORY=${_staging_directory}\"
          \"-D_INSTALL_DIRECTORY=${_install_directory}\"
          \"-D_INSTALL_MESSAGE=${_install_message}\"
          -P \"${_windeployqt_cmake}\"
    )
  "
    COMPONENT "${_install_component}"
  )

  message(VERBOSE "__windeployqt_impl('${arg_TARGET}') - success")
endfunction()



#[=============================================================================[
                            INTERNAL FUNCTION
#]=============================================================================]
function(__create_windeployqt_script_cmake arg_TARGET_FILEPATH)
  if (EXISTS "${arg_TARGET_FILEPATH}")
    return()
  endif()
  file(WRITE ${arg_TARGET_FILEPATH} [==========[
#
# WARNING: this file is auto-generated by WinDeployQt.cmake, do not edit
#
# This file invokes windeployqt.exe on a given target and where first all the
# qt binaries are deployed into a staging directory. Then the files are then
# selectively copied from the staging dir into an install-directory based on
# whether it matches the given exclude or include regular expressions.
#
# This module is NOT designed to be directly included in a CMakeLists.txt file
# It should be called as a COMMAND in execute_process() or add_custom_command()
#

get_filename_component(_list_filename "${CMAKE_CURRENT_LIST_FILE}" NAME)

# --------------------------
#  utility functions
# --------------------------

function(__install_file _src_file _dst_file)
  get_filename_component(_dst_dir "${_dst_file}" DIRECTORY)
  file(INSTALL "${_src_file}"
    DESTINATION "${_dst_dir}"
    FOLLOW_SYMLINK_CHAIN
    ${_INSTALL_MESSAGE}
  )
endfunction()

function(__check_include_regexes _out_var _file)
  foreach(_inc_rgx IN LISTS _INCLUDE_REGEXES)
    message(DEBUG "${_list_filename}:   checking if '${_file}' matches include regex: [${_inc_rgx}]")
    if ("${_file}" MATCHES "${_inc_rgx}")
      message(DEBUG "${_list_filename}:     file matches include regex [${_inc_rgx}]")
      set("${_out_var}" TRUE PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set("${_out_var}" FALSE PARENT_SCOPE)
endfunction()

function(__check_exclude_regexes _out_var _file)
  foreach(_exc_rgx IN LISTS _EXCLUDE_REGEXES)
    message(DEBUG "${_list_filename}:   checking if '${_file}' matches exclude regex: [${_exc_rgx}]")
    if ("${_file}" MATCHES "${_exc_rgx}")
      message(DEBUG "${_list_filename}:     file matches exclude regex [${_exc_rgx}]")
      set("${_out_var}" TRUE PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set("${_out_var}" FALSE PARENT_SCOPE)
endfunction()

# --------------------------
#  check for required vars
# --------------------------

set(_required_variables
  CMAKE_COMMAND
  _WINDEPLOYQT_EXECUTABLE
  _QT_BIN_DIR
  _VERBOSITY
  _WINDEPLOY_ARGS
  _TARGET_FILE
  _INCLUDE_REGEXES
  _EXCLUDE_REGEXES
  _STAGING_DIRECTORY
  _INSTALL_DIRECTORY
  _INSTALL_MESSAGE
)
foreach (_var IN LISTS _required_variables)
  if (NOT DEFINED ${_var})
    message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE} requires variable [${_var}] to be set")
  endif()
  message(DEBUG "${_var}=[${${_var}}]")
endforeach()

# --------------------------
#  run windeployqt, deploy all files to the staging dir
# --------------------------

set(_fwd_cmd_echo)
if ("${CMAKE_MESSAGE_LOG_LEVEL}" MATCHES "([Dd][Ee][Bb][Uu][Gg]|[Tt][Rr][Aa][Cc][Ee])")
  set(_fwd_cmd_echo COMMAND_ECHO STDOUT)
endif()

execute_process(
  ${_fwd_cmd_echo}
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE _wdqt_exit_code
  OUTPUT_VARIABLE _wdqt_stdout
  ERROR_VARIABLE  _wdqt_stderr
  COMMAND
    "${CMAKE_COMMAND}" -E
      env PATH="${_QT_BIN_DIR}"
        "${_WINDEPLOYQT_EXECUTABLE}"
          --dir "${_STAGING_DIRECTORY}"
          --no-compiler-runtime
          --verbose ${_VERBOSITY}
          ${_WINDEPLOY_ARGS}
          "${_TARGET_FILE}"
)

# --------------------------
#  print windeploy stdout
# --------------------------

get_filename_component(_wdqt_filename "${_WINDEPLOYQT_EXECUTABLE}" NAME)

if ((NOT ("${_VERBOSITY}" STREQUAL "")) AND (NOT ("${_VERBOSITY}" EQUAL "0")))
  message(STATUS "${_list_filename}: VERBOSE=ON, printing stdout of ${_wdqt_filename}")
  message("${_wdqt_stdout}")
endif()

# --------------------------
#  error out if windeployqt failed
# --------------------------

if (NOT ("${_wdqt_exit_code}" EQUAL "0"))
  set(_err_msg "${_wdqt_filename} failed with non-zero exit code [${_wdqt_exit_code}]
    ${_wdqt_stderr}"
  )
  message(FATAL_ERROR "${_err_msg}")
endif()

# report any (non-fatal) stderr messages as a CMake warning
if(NOT "${_wdqt_stderr}" STREQUAL "")
  message(WARNING "${_wdqt_filename}: ${_wdqt_stderr}")
endif()

# --------------------------
#  capture list of staged files
# --------------------------

file(GLOB_RECURSE _staged_files_rel
  RELATIVE "${_STAGING_DIRECTORY}"
  "${_STAGING_DIRECTORY}/*"
)

# --------------------------
#  deploy staged files to install dir
# --------------------------

set(_install_list)
foreach (_file IN LISTS _staged_files_rel)
  set(_rel_filepath "${_file}")
  message(DEBUG "${_list_filename}: staged file: ${_rel_filepath}")

  # files matching include regex are _always_ installed
  __check_include_regexes(_included "${_rel_filepath}")
  if (NOT _included)
    # files matching exclude regex are not installed unless matching include
    __check_exclude_regexes(_excluded "${_rel_filepath}")
    if (_excluded)
      continue()
    endif()
  endif()

  # files are always installed by default if not excluded
  list(APPEND _install_list "${_rel_filepath}")
endforeach()

foreach (_rel_filepath IN LISTS _install_list)
  __install_file(
    "${_STAGING_DIRECTORY}/${_rel_filepath}"
    "${_INSTALL_DIRECTORY}/${_rel_filepath}"
  )
endforeach()
]==========]
  )
endfunction() # __create_windeployqt_script_cmake