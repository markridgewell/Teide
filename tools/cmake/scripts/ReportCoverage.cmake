function(exec)
    execute_process(
        ${ARGV}
        COMMAND_ECHO
        STDERR
        COMMAND_ERROR_IS_FATAL
        ANY)
endfunction()

if(COMPILER STREQUAL "Clang")
    # Gather raw profile data
    file(GLOB_RECURSE profraw_files "${COVERAGE_DIR}/*.profraw")

    # Combine raw data into indexed file
    find_program(profdata llvm-profdata REQUIRED)
    exec(COMMAND "${profdata}" merge ${profraw_files} -o ${COVERAGE_DIR}/coverage.profdata)

    find_program(cov llvm-cov REQUIRED)
    file(STRINGS "${COVERAGE_DIR}/test_binaries.txt" binaries)
    set(output_dir "${COVERAGE_DIR}/output")
    file(MAKE_DIRECTORY "${output_dir}")

    # Generate lcov txt file for uploading to Codecov
    exec(COMMAND ${cov} export -format=lcov "-ignore-filename-regex=/tests/" -instr-profile
                 "${COVERAGE_DIR}/coverage.profdata" -object ${binaries} #
         OUTPUT_FILE "${output_dir}/lcov.info")

    # Generate HTML report for local inspection
    exec(COMMAND ${cov} show -format=html "-ignore-filename-regex=/tests/" "-output-dir=${output_dir}" -instr-profile
                 "${COVERAGE_DIR}/coverage.profdata" -object ${binaries})
endif()
