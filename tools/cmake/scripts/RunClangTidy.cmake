cmake_minimum_required(VERSION 3.19)

find_program(clang_tidy "clang-tidy" REQUIRED)
mark_as_advanced(clang_tidy)

message("Running ClangTidy script")

# Remove empty arguments from the lists
# cmake-format: off
list(FILTER SOURCES INCLUDE REGEX ".+")
list(FILTER INCLUDE_DIRS INCLUDE REGEX ".+")
list(FILTER SYSTEM_INCLUDE_DIRS INCLUDE REGEX ".+")
list(FILTER CLANG_TIDY_ARGS INCLUDE REGEX ".+")
# cmake-format: on

# Build clang-tidy command
if(DEFINED ENV{CLANG_TIDY_SOURCES})
    # Add source files from environment variable
    message(STATUS "Taking sources from CLANG_TIDY_SOURCES environment variable")
    list(APPEND clang_tidy_args $ENV{CLANG_TIDY_SOURCES})
    list(APPEND clang_tidy_args ${CLANG_TIDY_ARGS} "--")
else()
    # Add source files passed into script
    list(APPEND clang_tidy_args ${SOURCES})
    list(APPEND clang_tidy_args ${CLANG_TIDY_ARGS} "--")
    # Supplied sources will be .cpp files only; use header filter to include warnings from headers
    list(APPEND clang_tidy_args "--header-filter='.*'")
endif()

foreach(dir IN LISTS INCLUDE_DIRS)
    list(APPEND clang_tidy_args "-I${dir}")
endforeach()
foreach(dir IN LISTS SYSTEM_INCLUDE_DIRS)
    list(APPEND clang_tidy_args "-isystem" "${dir}")
endforeach()

# Add the rest of the command-line arguments
list(APPEND clang_tidy_args ${COMPILER_ARGS})

# Run the command and propogate error code
list(JOIN clang_tidy_args "\" \"" clang_tidy_arg_str)
message(STATUS "${clang_tidy} --version")
execute_process(COMMAND "${clang_tidy}" --version)
message(STATUS "\"${clang_tidy}\" \"${clang_tidy_arg_str}\"")
execute_process(COMMAND "${clang_tidy}" ${clang_tidy_args} COMMAND_ERROR_IS_FATAL ANY)
