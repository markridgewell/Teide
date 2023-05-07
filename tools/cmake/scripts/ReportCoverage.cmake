function(exec)
    execute_process(
        ${ARGV}
        COMMAND_ECHO
        STDERR
        COMMAND_ERROR_IS_FATAL
        ANY)
endfunction()

if(COMPILER STREQUAL "MSVC")
    # Gather binary coverage data
    file(GLOB_RECURSE cov_files "${COVERAGE_DIR}/*.cov")
    list(TRANSFORM cov_files PREPEND "--input_coverage=")

    # Combine the coverage data and generate output
    find_program(OPENCPPCOVERAGE_PATH "OpenCppCoverage" REQUIRED)
    mark_as_advanced(OPENCPPCOVERAGE_PATH)
    set(output_dir "${COVERAGE_DIR}/output")
    file(MAKE_DIRECTORY "${output_dir}")
    exec(COMMAND ${OPENCPPCOVERAGE_PATH} --export_type "html:${output_dir}" --export_type
                 "cobertura:${output_dir}/coverage.xml" ${cov_files})

elseif(COMPILER STREQUAL "Clang")
    # Gather raw profile data
    file(GLOB_RECURSE profraw_files "${COVERAGE_DIR}/*.profraw")

    # Combine raw data into indexed file
    find_program(profdata llvm-profdata REQUIRED)
    exec(COMMAND "${profdata}" merge ${profraw_files} -o ${COVERAGE_DIR}/coverage.profdata)

    find_program(cov llvm-cov REQUIRED)
    file(STRINGS "${COVERAGE_DIR}/test_binaries.txt" binaries)
    set(output_dir "${COVERAGE_DIR}/output")
    file(MAKE_DIRECTORY "${output_dir}")

    set(ignore_patterns
        /tests/
        /usr/include/
        /VULKAN_SDK/
        /vcpkg_installed/
        ${IGNORE_PATTERNS})
    list(JOIN ignore_patterns "|" ignore_regex)
    set(common_args "-ignore-filename-regex=${ignore_regex}" -instr-profile "${COVERAGE_DIR}/coverage.profdata")

    # Generate lcov txt file for uploading to Codecov
    exec(COMMAND ${cov} export -format=lcov ${common_args} -object ${binaries} #
         OUTPUT_FILE "${output_dir}/lcov.info")

    # Generate HTML report for local inspection
    exec(COMMAND ${cov} show -format=html -use-color -Xdemangler c++filt ${common_args} "-output-dir=${output_dir}"
                 -object ${binaries})

    # Add a little extra css to highlight covered lines
    file(APPEND "${output_dir}/style.css"
         "td.covered-line + td > pre { background-color: #d0ffd0 }\npre:has(span.red) { background-color: #ffd0d0; }")
endif()
