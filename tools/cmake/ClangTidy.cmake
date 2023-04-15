include(tools/cmake/CompileCommands.cmake)

function(td_add_clang_tidy)
    compile_commands()

    list(APPEND clang_tidy_cmd "-std=c++${CMAKE_CXX_STANDARD}")
    foreach(dir IN LISTS project_include_directories)
        list(APPEND clang_tidy_cmd "-I${dir}")
    endforeach()
    foreach(dir IN LISTS project_system_include_directories)
        list(APPEND clang_tidy_cmd "-isystem" "${dir}")
    endforeach()
    foreach(def IN LISTS project_compile_definitions)
        list(APPEND clang_tidy_cmd "-D${def}")
    endforeach()

    add_custom_target(
        ClangTidy
        COMMAND ${CMAKE_COMMAND} "-DSOURCES=${project_sources}" "-DCLANG_TIDY_ARGS=${clang_tidy_cmd}" -P
                ${CMAKE_SOURCE_DIR}/tools/cmake/scripts/RunClangTidy.cmake
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    set_target_properties(ClangTidy PROPERTIES FOLDER StaticAnalysis)
endfunction()
