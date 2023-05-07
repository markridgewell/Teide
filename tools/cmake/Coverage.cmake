set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")

function(setup_coverage ignore_patterns)
    add_test(NAME ClearCoverage COMMAND ${CMAKE_COMMAND} -E rm -rf ${COVERAGE_DIR})
    add_test(NAME InitCoverage COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR})
    set_tests_properties(InitCoverage PROPERTIES DEPENDS ClearCoverage)
    set_tests_properties(ClearCoverage InitCoverage PROPERTIES FIXTURES_SETUP CppCoverage)

    add_test(NAME ReportCoverage
             COMMAND ${CMAKE_COMMAND} -DCOMPILER=${CMAKE_CXX_COMPILER_ID} "-DCOVERAGE_DIR=${COVERAGE_DIR}"
                     "-DIGNORE_PATTERNS=${ignore_patterns}" -P "${SCRIPTS_DIR}/ReportCoverage.cmake")
    set_tests_properties(ReportCoverage PROPERTIES FIXTURES_CLEANUP CppCoverage)
endfunction()

function(target_enable_coverage target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    endif()
endfunction()

function(test_enable_coverage test)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(environment "LLVM_PROFILE_FILE=${COVERAGE_DIR}/${target}-%p.profraw")
    endif()
    set_tests_properties(${test} PROPERTIES ENVIRONMENT "${environment}" FIXTURES_REQUIRED CppCoverage)
endfunction()
