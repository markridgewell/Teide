include(tools/cmake/CompileCommands.cmake)

function(td_add_clang_tidy)
    find_program(clang_tidy "clang-tidy" REQUIRED)

    compile_commands()

    set(clang_tidy_cmd "${clang_tidy}")
    foreach(source ${project_sources})
        list(APPEND clang_tidy_cmd ${source})
    endforeach()
    list(APPEND clang_tidy_cmd "--" "-std=c++${CMAKE_CXX_STANDARD}")
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
        COMMAND ${clang_tidy} --version
        COMMAND ${clang_tidy_cmd}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    set_target_properties(ClangTidy PROPERTIES FOLDER StaticAnalysis)
endfunction()
