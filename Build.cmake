function(td_add_library target_name)
    cmake_parse_arguments(
        "ARG"
        "" # Flags
        "" # Single-value arguments
        "SOURCES" # Multi-value arguments
        ${ARGN})

    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    set(include_dir "${CMAKE_CURRENT_SOURCE_DIR}/Include")

    set(lib_type INTERFACE)
    set(interface INTERFACE)
    if(EXISTS ${source_dir} AND IS_DIRECTORY "${source_dir}")
        set(lib_type PUBLIC)
        set(interface "")
    endif()

    add_library(${target_name} ${interface} ${ARG_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_compile_features(${target_name} ${lib_type} ${cxx_standard})

    if(EXISTS ${include_dir} AND IS_DIRECTORY "${include_dir}")
        target_include_directories(${target_name} ${lib_type} ${include_dir})
    endif()
endfunction()

function(td_add_application target_name)
    cmake_parse_arguments(
        "ARG"
        "" # Flags
        "" # Single-value arguments
        "SOURCES" # Multi-value arguments
        ${ARGN})

    add_executable(${target_name} WIN32 ${ARG_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_compile_features(${target_name} PRIVATE ${cxx_standard})
endfunction()

function(td_add_test target_name)
    cmake_parse_arguments(
        "ARG"
        "" # Flags
        "" # Single-value arguments
        "SOURCES" # Multi-value arguments
        ${ARGN})

    add_executable(${target_name} ${ARG_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_SOURCES})
    target_compile_features(${target_name} PRIVATE ${cxx_standard})
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()
