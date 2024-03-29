include(tools/cmake/CompileCommands.cmake)

function(td_add_clang_tidy)
    compile_commands()

    list(APPEND compiler_args "-xc++" "-std=c++${CMAKE_CXX_STANDARD}")
    foreach(def IN LISTS project_compile_definitions)
        list(APPEND compiler_args "-D${def}")
    endforeach()

    set(clang_tidy_args "--fix" "--export-fixes=${CMAKE_BINARY_DIR}/clang-tidy-fixes.yaml")

    add_custom_target(
        ClangTidy
        COMMAND
            ${CMAKE_COMMAND} #
            "-DSOURCES='${project_sources}'" #
            "-DINCLUDE_DIRS='${project_include_directories}'" #
            "-DSYSTEM_INCLUDE_DIRS='${project_system_include_directories}'" #
            "-DCLANG_TIDY_ARGS='${clang_tidy_args}'" #
            "-DCOMPILER_ARGS='${compiler_args}'" #
            -P "${SCRIPTS_DIR}/RunClangTidy.cmake"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    set_target_properties(ClangTidy PROPERTIES FOLDER StaticAnalysis)
endfunction()
