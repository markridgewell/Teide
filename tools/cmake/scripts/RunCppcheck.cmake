cmake_minimum_required(VERSION 3.19)

find_program(
    cppcheck "cppcheck"
    HINTS ENV
          "ProgramFiles"
          ENV
          "ProgramFiles(x86)"
          ENV
          "ProgramW6432"
    PATH_SUFFIXES "Cppcheck" REQUIRED)
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

if(DEFINED ENV{TEIDE_CPPCHECK_ARGS})
    list(APPEND cppcheck_args $ENV{TEIDE_CPPCHECK_ARGS})
endif()

# Set the format for message output
string(ASCII 27 esc)
set(_location "{file}({line},{column})")
set(template_location "${esc}[96m${_location}): note: {info}${esc}[0m\\n{code}")
set(template "${esc}[31m${_location}: {severity}: {message} [{id}]${esc}[0m\\n{code}")

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
execute_process(COMMAND "${cppcheck}" --version)
execute_process(COMMAND "${cppcheck}" ${cppcheck_args} "--template=${template}"
                        "--template-location=${template_location}" COMMAND_ERROR_IS_FATAL ANY)
