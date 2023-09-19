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
if(DEFINED ENV{CPPCHECK_SOURCES})
    # Add source files from environment variable
    message(STATUS "Taking sources from CPPCHECK_SOURCES environment variable")
    list(APPEND cppcheck_args $ENV{CPPCHECK_SOURCES})
else()
    # Add source files passed into script
    list(APPEND cppcheck_args ${SOURCES})
endif()

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
message(STATUS "${cppcheck} --version")
execute_process(COMMAND "${cppcheck}" --version)
message(STATUS "\"${cppcheck}\" \"${cppcheck_arg_str}\"")
execute_process(COMMAND "${cppcheck}" ${cppcheck_args} COMMAND_ERROR_IS_FATAL ANY)
