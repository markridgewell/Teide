find_program(clang_tidy "clang-tidy" REQUIRED)

# Build clang-tidy command
if(DEFINED ENV{CLANG_TIDY_SOURCES})
    # Add source files from environment variable
    message(STATUS "Taking sources from CLANG_TIDY_SOURCES environment variable")
    list(APPEND clang_tidy_args $ENV{CLANG_TIDY_SOURCES})
    list(APPEND clang_tidy_args "--")
else()
    # Add source files passed into script
    list(APPEND clang_tidy_args ${SOURCES})
    list(APPEND clang_tidy_args "--")
    # Supplied sources will be .cpp files only; use header filter to include warnings from headers
    list(APPEND clang_tidy_args "--header-filter='.*'")
endif()

# Add the rest of the command-line arguments
list(APPEND clang_tidy_args ${CLANG_TIDY_ARGS})
# Remove empty arguments from the list
# cmake-format: off
list(FILTER clang_tidy_args INCLUDE REGEX ".+")
# cmake-format: on

# Run the command and propogate error code
list(JOIN clang_tidy_args "\" \"" clang_tidy_arg_str)
message(STATUS "${clang_tidy} --version")
execute_process(COMMAND "${clang_tidy}" --version)
message(STATUS "\"${clang_tidy}\" \"${clang_tidy_arg_str}\"")
execute_process(COMMAND "${clang_tidy}" ${clang_tidy_args} COMMAND_ERROR_IS_FATAL ANY)
