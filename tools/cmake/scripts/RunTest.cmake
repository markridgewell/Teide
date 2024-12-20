set(coverage_dir "$ENV{COVERAGE_DIR}")

if("$ENV{TEST_COVERAGE}")
    if(COMPILER STREQUAL "MSVC")
        cmake_path(GET TEST_BINARY STEM test_name)
        foreach(path IN LISTS SOURCES)
            cmake_path(NATIVE_PATH path native_path)
            list(APPEND sources "--source=${native_path}")
        endforeach()
        set(RUNNER_COMMAND "$ENV{OPENCPPCOVERAGE}" "--export_type=binary:${coverage_dir}/${test_name}.cov"
                           "--modules=*.exe" ${sources} --cover_children --)
    elseif(COMPILER STREQUAL "Clang")
        file(APPEND "$ENV{COVERAGE_DIR}/test_binaries.txt" "${TEST_BINARY}\n")
        list(JOIN SOURCES "\n" source_list)
        file(APPEND "$ENV{COVERAGE_DIR}/test_sources.txt" "${source_list}\n")
    endif()
endif()

execute_process(COMMAND ${RUNNER_COMMAND} ${TEST_BINARY} ${TEST_ARGS} RESULT_VARIABLE result COMMAND_ERROR_IS_FATAL ANY)
