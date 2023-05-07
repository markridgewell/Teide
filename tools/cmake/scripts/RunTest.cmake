if(TEIDE_TEST_COVERAGE)
    if(COMPILER STREQUAL "MSVC")
        find_program(OPENCPPCOVERAGE_PATH "OpenCppCoverage" REQUIRED)
        mark_as_advanced(OPENCPPCOVERAGE_PATH)
        cmake_path(GET TEST_BINARY STEM TEST_NAME)
        set(RUNNER_COMMAND
            ${OPENCPPCOVERAGE_PATH} --export_type "binary:${COVERAGE_DIR}/${TEST_NAME}.cov" --modules "*.exe" --source
            "Teide\\src" --source "Teide\\include" --source "Teide\\examples" --cover_children --)
    elseif(COMPILER STREQUAL "Clang")
        message(STATUS "Writing ${TEST_BINARY} into file ${COVERAGE_DIR}/test_binaries.txt")
        file(APPEND "${COVERAGE_DIR}/test_binaries.txt" "${TEST_BINARY}\n")
    endif()
endif()

execute_process(COMMAND ${RUNNER_COMMAND} ${TEST_BINARY} ${TEST_ARGS} COMMAND_ECHO STDERR COMMAND_ERROR_IS_FATAL ANY)
