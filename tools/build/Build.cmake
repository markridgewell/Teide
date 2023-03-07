set(options)
set(oneValueArgs)

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
    set(local_options DPI_AWARE)
    set(multiValueArgs SOURCES DEPS)
    cmake_parse_arguments(
        "ARG"
        "${local_options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")

    add_executable(${target_name} WIN32 ${ARG_SOURCES})
    if(${ARG_DPI_AWARE})
        set_property(TARGET Application PROPERTY VS_DPI_AWARE "PerMonitor")
    endif()
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_include_directories(${target_name} PRIVATE ${source_dir})
    target_link_libraries(${target_name} PRIVATE ${ARG_DEPS})
endfunction()

function(td_add_console_application target_name)
    set(multiValueArgs SOURCES DEPS)
    cmake_parse_arguments(
        "ARG"
        "${local_options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN})

    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")

    add_executable(${target_name} ${ARG_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_include_directories(${target_name} PRIVATE ${source_dir})
    target_link_libraries(${target_name} PRIVATE ${ARG_DEPS})
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
endfunction()
