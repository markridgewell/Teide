set(source_extensions ".cpp;.c;.cc;.cxx")

function(_get_all_targets var)
    set(targets)
    _get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(${var}
        ${targets}
        PARENT_SCOPE)
endfunction()

macro(_get_all_targets_recursive targets dir)
    get_property(
        subdirectories
        DIRECTORY ${dir}
        PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirectories)
        _get_all_targets_recursive(${targets} ${subdir})
    endforeach()

    get_property(
        current_targets
        DIRECTORY ${dir}
        PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${current_targets})
endmacro()

macro(_gather_system_include_dirs target out_var)
    # Gather system include directories
    get_target_property(is_imported ${target} IMPORTED)
    if(is_imported)
        # Target is imported, add interface include directories
        get_target_property(target_system_include_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
        if(target_system_include_dirs)
            list(APPEND ${out_var} ${target_system_include_dirs})
            list(REMOVE_DUPLICATES ${out_var})
        endif()
    endif()

    # Recurse into dependencies
    get_target_property(dependencies ${target} LINK_LIBRARIES)
    if(dependencies)
        foreach(dependency IN LISTS dependencies)
            _gather_system_include_dirs(${dependency} ${out_var})
        endforeach()
    endif()
endmacro()

function(dump_targets filename)
    _get_all_targets(all_targets)
    set(content "")
    foreach(target IN LISTS all_targets)
        get_target_property(target_type ${target} TYPE)
        string(APPEND content "${target} ${target_type}\n")
    endforeach()
    file(WRITE "${filename}" "${content}")
endfunction()

function(compile_commands)
    _get_all_targets(all_targets)
    foreach(target IN LISTS all_targets)
        get_target_property(target_source_dir ${target} SOURCE_DIR)
        get_target_property(target_sources ${target} SOURCES)
        get_target_property(target_include_directories ${target} INCLUDE_DIRECTORIES)
        get_target_property(target_compile_definitions ${target} COMPILE_DEFINITIONS)
        _gather_system_include_dirs(${target} target_system_include_directories)

        if(target_sources)
            foreach(source IN LISTS target_sources)
                cmake_path(
                    GET
                    source
                    EXTENSION
                    LAST_ONLY
                    extension)
                if(extension IN_LIST source_extensions)
                    cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY ${target_source_dir})
                    list(APPEND all_sources ${source})
                endif()
            endforeach()
            list(REMOVE_DUPLICATES all_sources)
        endif()
        if(target_include_directories)
            list(APPEND all_include_directories ${target_include_directories})
            list(REMOVE_DUPLICATES all_include_directories)
        endif()
        if(target_system_include_directories)
            list(APPEND all_system_include_directories ${target_system_include_directories})
            list(REMOVE_DUPLICATES all_system_include_directories)
        endif()
        if(target_compile_definitions)
            list(APPEND all_compile_definitions ${target_compile_definitions})
            list(REMOVE_DUPLICATES all_compile_definitions)
        endif()
    endforeach()

    foreach(dir IN LISTS all_system_include_directories)
        list(REMOVE_ITEM all_include_directories "${dir}")
    endforeach()

    set(project_sources
        ${all_sources}
        PARENT_SCOPE)
    set(project_include_directories
        ${all_include_directories}
        PARENT_SCOPE)
    set(project_system_include_directories
        ${all_system_include_directories}
        PARENT_SCOPE)
    set(project_compile_definitions
        ${all_compile_definitions}
        PARENT_SCOPE)
endfunction()
