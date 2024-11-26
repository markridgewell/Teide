include(tools/cmake/CompileCommands.cmake)

function(td_add_cppcheck)
    compile_commands()

    file(STRINGS "${CMAKE_SOURCE_DIR}/.cppcheck" cppcheck_args)
    list(APPEND cppcheck_args --error-exitcode=1 "--cppcheck-build-dir=${CMAKE_BINARY_DIR}/cppcheck")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/cppcheck")

    add_custom_target(
        Cppcheck
        COMMAND
            ${CMAKE_COMMAND} #
            "-DSOURCES='${project_sources}'" #
            "-DINCLUDE_DIRS='${project_include_directories}'" #
            "-DSYSTEM_INCLUDE_DIRS='${project_system_include_directories}'" #
            "-DDEFINITIONS='${project_compile_definitions}'" #
            "-DCPPCHECK_ARGS='${cppcheck_args}'" #
            -P "${SCRIPTS_DIR}/RunCppcheck.cmake"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    set_target_properties(Cppcheck PROPERTIES FOLDER StaticAnalysis)
endfunction()
