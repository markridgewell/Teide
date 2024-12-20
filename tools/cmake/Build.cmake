include(tools/cmake/Coverage.cmake)
include(tools/cmake/Sanitizers.cmake)
include(tools/cmake/Target.cmake)

set(options)
set(oneValueArgs)

set(CMAKE_LINK_LIBRARIES_ONLY_TARGETS ON)

macro(_gather_install_files target)
    get_target_property(target_install_files ${target} INSTALL_FILES)
    if(target_install_files)
        list(APPEND install_files ${target_install_files})
        list(REMOVE_DUPLICATES install_files)
    endif()

    get_target_property(dependencies ${target} LINK_LIBRARIES)
    if(dependencies)
        foreach(dependency IN LISTS dependencies)
            _gather_install_files(${dependency})
        endforeach()
    endif()
endmacro()

function(_copy_install_files target)
    _gather_install_files(${target})
    if(install_files)
        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${install_files} $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS)
    endif()
    if(WIN32) # Install DLLs
        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${target}> $<TARGET_RUNTIME_DLLS:${target}>
            COMMAND_EXPAND_LISTS)
    endif()
endfunction()

function(td_add_library target)
    set(multiValueArgs SOURCES PUBLIC_DEPS PRIVATE_DEPS)
    cmake_parse_arguments(
        "ARG"
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set(include_dir "${CMAKE_CURRENT_SOURCE_DIR}/include")

    set(lib_type INTERFACE)
    set(interface INTERFACE)
    if(EXISTS ${source_dir} AND IS_DIRECTORY "${source_dir}")
        set(lib_type PUBLIC)
        set(interface "")
    endif()

    add_library(${target} ${interface} ${ARG_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_link_libraries(
        ${target}
        PUBLIC ${ARG_PUBLIC_DEPS}
        PRIVATE ${ARG_PRIVATE_DEPS})
    if(TEIDE_TEST_COVERAGE)
        target_enable_coverage(${target})
    endif()
    target_enable_sanitizer(${target} ${TEIDE_SANITIZER})

    if(EXISTS "${source_dir}" AND IS_DIRECTORY "${source_dir}")
        target_include_directories(${target} PRIVATE ${source_dir})
    endif()
    if(EXISTS ${include_dir} AND IS_DIRECTORY "${include_dir}")
        target_include_directories(${target} ${lib_type} ${include_dir})
    endif()
endfunction()

function(td_add_application target)
    set(local_options WIN32 DPI_AWARE)
    set(multiValueArgs SOURCES DEPS)
    cmake_parse_arguments(
        "ARG"
        "${local_options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")

    add_executable(${target} ${ARG_SOURCES})
    if(${ARG_WIN32})
        set_property(TARGET ${target} PROPERTY WIN32_EXECUTABLE ON)
    endif()
    if(${ARG_DPI_AWARE})
        set_property(TARGET ${target} PROPERTY VS_DPI_AWARE "PerMonitor")
    endif()
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_include_directories(${target} PRIVATE ${source_dir})
    target_link_libraries(${target} PRIVATE ${ARG_DEPS})
    if(TEIDE_TEST_COVERAGE)
        target_enable_coverage(${target})
    endif()
    target_enable_sanitizer(${target} ${TEIDE_SANITIZER})

    _copy_install_files(${target})
endfunction()

function(td_add_test target)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES DEPS TEST_ARGS)
    cmake_parse_arguments(
        "ARG"
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")

    add_executable(${target} ${ARG_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE ${ARG_TARGET} ${ARG_DEPS})
    target_include_directories(${target} PRIVATE ${source_dir})
    target_include_directories(${target} PRIVATE ${test_dir})
    if(TARGET ${ARG_TARGET})
        target_include_directories(${target} PRIVATE $<TARGET_PROPERTY:${ARG_TARGET},INCLUDE_DIRECTORIES>)
    endif()
    if(TEIDE_TEST_COVERAGE)
        target_enable_coverage(${target})
    endif()
    target_enable_sanitizer(${target} ${TEIDE_SANITIZER})

    get_local_dependencies(dependencies ${target})
    get_target_sources(dep_sources ${dependencies})

    if(TEIDE_TEST_COVERAGE)
        add_test(
            NAME ${target}
            COMMAND ${CMAKE_COMMAND} "-DTEST_BINARY=$<TARGET_FILE:${target}>" "-DTEST_ARGS=${ARG_TEST_ARGS}"
                    "-DCOMPILER=${CMAKE_CXX_COMPILER_ID}" "-DSOURCES=${dep_sources}" -P "${SCRIPTS_DIR}/RunTest.cmake")
        test_enable_coverage(${target})
    else()
        add_test(NAME ${target} COMMAND ${target} ${ARG_TEST_ARGS})
    endif()
    test_enable_sanitizer(${target} ${TEIDE_SANITIZER})

    list(JOIN ARG_TEST_ARGS " " debugger_args)
    set_property(TARGET ${target} PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "${debugger_args}")

    _copy_install_files(${target})
endfunction()
