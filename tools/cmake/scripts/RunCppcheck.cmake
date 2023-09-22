cmake_minimum_required(VERSION 3.19)

find_program(cppcheck "cppcheck" REQUIRED)
mark_as_advanced(cppcheck)

message("Running Cppcheck script")

# Remove empty arguments from the lists
# cmake-format: off
list(FILTER SOURCES INCLUDE REGEX ".+")
list(FILTER INCLUDE_DIRS INCLUDE REGEX ".+")
list(FILTER DEFINITIONS INCLUDE REGEX ".+")
list(FILTER CPPCHECK_ARGS INCLUDE REGEX ".+")
# cmake-format: on

# Build cppcheck command
list(APPEND cppcheck_args ${SOURCES})

# Set the format for message output
if(DEFINED ENV{CI})
    # For CI runs, use GitHub Actions annotations
    set(_location "file={file},line={line},col={column}")
    set(_location2 "{file}({line},{column})")
    set(template_location "::notice ${_location}::{message}\\n${_location2}\\n{code}")
    set(template "::error ${_location},title=Cppcheck {severity} [{id}]::{message}\\n${_location2}\\n{code}")
else()
    # For local runs, use colour-highlighted output
    string(ASCII 27 esc)
    set(_location "{file}({line},{column})")
    set(template_location "${esc}[96m${_location}): note: {info}${esc}[0m\\n{code}")
    set(template "${esc}[31m${_location}: {severity}: {message} [{id}]${esc}[0m\\n{code}")
endif()

# Remove system include dirs - Cppcheck works better without them
list(REMOVE_ITEM INCLUDE_DIRS ${SYSTEM_INCLUDE_DIRS})

foreach(dir IN LISTS INCLUDE_DIRS)
    list(APPEND cppcheck_args "-I${dir}")
endforeach()
foreach(def IN LISTS DEFINITIONS)
    list(APPEND cppcheck_args "-D${def}")
endforeach()

# Add the rest of the command-line arguments
list(APPEND cppcheck_args ${CPPCHECK_ARGS})

# Run the command and propogate error code
list(JOIN cppcheck_args "\" \"" cppcheck_arg_str)
execute_process(COMMAND "${cppcheck}" --version)
execute_process(COMMAND "${cppcheck}" ${cppcheck_args} "--template=${template}"
                        "--template-location=${template_location}" COMMAND_ERROR_IS_FATAL ANY)
