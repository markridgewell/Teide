set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")

function(_find_opencppcoverage)
    find_program(OPENCPPCOVERAGE_PATH "OpenCppCoverage" HINTS "C:\\Program Files\\OpenCppCoverage" REQUIRED)
    mark_as_advanced(OPENCPPCOVERAGE_PATH)
endfunction()

function(setup_coverage)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        _find_opencppcoverage()
        message(STATUS "OpenCppCoverage found at ${OPENCPPCOVERAGE_PATH}")
    endif()
    add_test(NAME ClearCoverage COMMAND ${CMAKE_COMMAND} -E rm -rf ${COVERAGE_DIR})
    add_test(NAME InitCoverage COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR})
    set_tests_properties(InitCoverage PROPERTIES DEPENDS ClearCoverage)
    set_tests_properties(ClearCoverage InitCoverage PROPERTIES FIXTURES_SETUP CppCoverage)

    add_test(NAME ReportCoverage
             COMMAND ${CMAKE_COMMAND} -DCOMPILER=${CMAKE_CXX_COMPILER_ID} "-DCOVERAGE_DIR=${COVERAGE_DIR}"
                     "-DOPENCPPCOVERAGE=${OPENCPPCOVERAGE_PATH}" -P "${SCRIPTS_DIR}/ReportCoverage.cmake")
    set_tests_properties(ReportCoverage PROPERTIES FIXTURES_CLEANUP CppCoverage)
endfunction()

function(target_enable_coverage target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    endif()
endfunction()

function(test_enable_coverage test)
    set(environment "TEST_COVERAGE=1" "COVERAGE_DIR=${COVERAGE_DIR}")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        _find_opencppcoverage()
        list(APPEND environment "OPENCPPCOVERAGE=${OPENCPPCOVERAGE_PATH}")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        list(APPEND environment "LLVM_PROFILE_FILE=${COVERAGE_DIR}/${target}-%p.profraw")
    endif()
    set_property(
        TEST ${test}
        APPEND
        PROPERTY ENVIRONMENT "${environment}")
    set_property(TEST ${test} PROPERTY FIXTURES_REQUIRED CppCoverage)
endfunction()
