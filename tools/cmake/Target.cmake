function(get_local_dependencies out_var target)
    get_target_property(target_deps ${target} LINK_LIBRARIES)
    if(target_deps)
        foreach(dep IN LISTS target_deps)
            get_target_property(is_imported ${dep} IMPORTED)
            if(NOT is_imported)
                set(${out_var}
                    ${${out_var}} ${dep}
                    PARENT_SCOPE)
                list(REMOVE_DUPLICATES ${out_var})
                get_local_dependencies(${out_var} ${dep})
            endif()
        endforeach()
    endif()
endfunction()

function(get_target_sources outvar)
    list(
        SUBLIST
        ARGV
        1
        -1
        targets)
    foreach(tgt IN LISTS targets)
        get_target_property(tgt_sources ${tgt} SOURCES)
        get_target_property(tgt_source_dir ${tgt} SOURCE_DIR)
        get_target_property(tgt_binary_dir ${tgt} BINARY_DIR)
        foreach(source IN LISTS tgt_sources)
            cmake_path(IS_ABSOLUTE source is_absolute)
            if(IS_ABSOLUTE)
                list(APPEND out_sources "${source}")
            elseif(source MATCHES "^\$<")
                # Source starts with generator expression, must be absolute
                list(APPEND out_sources "${source}")
            elseif(EXISTS "${tgt_source_dir}/${source}")
                list(APPEND out_sources "${tgt_source_dir}/${source}")
            elseif(EXISTS "${tgt_binary_dir}/${source}")
                list(APPEND out_sources "${tgt_binary_dir}/${source}")
            else()
                message(WARNING "Unable to resolve source file of target ${target}: ${source}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES out_sources)
    set(${outvar}
        "${out_sources}"
        PARENT_SCOPE)
endfunction()
