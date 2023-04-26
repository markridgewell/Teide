if(TEIDE_TEST_COVERAGE)
    message(STATUS "Writing ${TEST_BINARY} into file ${COVERAGE_DIR}/test_binaries.txt")
    file(APPEND "${COVERAGE_DIR}/test_binaries.txt" "${TEST_BINARY}\n")
endif()

execute_process(COMMAND ${TEST_BINARY} ${TEST_ARGS} COMMAND_ECHO STDERR COMMAND_ERROR_IS_FATAL ANY)
