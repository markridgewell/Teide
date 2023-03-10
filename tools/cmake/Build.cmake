set(options)
set(oneValueArgs)

set(CMAKE_LINK_LIBRARIES_ONLY_TARGETS ON)

macro(_gather_install_files target_name)
    get_target_property(target_install_files ${target_name} INSTALL_FILES)
    if(target_install_files)
        list(APPEND install_files ${target_install_files})
        list(REMOVE_DUPLICATES install_files)
    endif()

    get_target_property(dependencies ${target_name} LINK_LIBRARIES)
    if(dependencies)
        foreach(dependency IN LISTS dependencies)
            _gather_install_files(${dependency})
        endforeach()
    endif()
endmacro()

function(_copy_install_files target_name)
    _gather_install_files(${target_name})
    if(install_files)
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${install_files} $<TARGET_FILE_DIR:${target_name}>
            COMMAND_EXPAND_LISTS)
    endif()

endfunction()

function(td_add_library target_name)
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

    add_library(${target_name} ${interface} ${ARG_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_link_libraries(
        ${target_name}
        PUBLIC ${ARG_PUBLIC_DEPS}
        PRIVATE ${ARG_PRIVATE_DEPS})

    if(EXISTS ${source_dir} AND IS_DIRECTORY "${source_dir}")
        target_include_directories(${target_name} PRIVATE ${source_dir})
    endif()
    if(EXISTS ${include_dir} AND IS_DIRECTORY "${include_dir}")
        target_include_directories(${target_name} ${lib_type} ${include_dir})
    endif()
endfunction()

function(td_add_application target_name)
    set(local_options WIN32 DPI_AWARE)
    set(multiValueArgs SOURCES DEPS)
    cmake_parse_arguments(
        "ARG"
        "${local_options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")

    add_executable(${target_name} ${ARG_SOURCES})
    if(${ARG_WIN32})
        set_property(TARGET ${target_name} PROPERTY WIN32_EXECUTABLE ON)
    endif()
    if(${ARG_DPI_AWARE})
        set_property(TARGET ${target_name} PROPERTY VS_DPI_AWARE "PerMonitor")
    endif()
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_include_directories(${target_name} PRIVATE ${source_dir})
    target_link_libraries(${target_name} PRIVATE ${ARG_DEPS})

    _copy_install_files(${target_name})
endfunction()

function(td_add_test target_name)
    set(multiValueArgs SOURCES DEPS TEST_ARGS)
    cmake_parse_arguments(
        "ARG"
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set(test_dir "${CMAKE_CURRENT_SOURCE_DIR}/tests")

    add_executable(${target_name} ${ARG_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_link_libraries(${target_name} PRIVATE ${ARG_DEPS})
    target_include_directories(${target_name} PRIVATE ${source_dir})
    target_include_directories(${target_name} PRIVATE ${test_dir})
    add_test(NAME ${target_name} COMMAND ${target_name} ${ARG_TEST_ARGS})

    _copy_install_files(${target_name})
endfunction()
