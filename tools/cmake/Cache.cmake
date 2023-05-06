function(mark_as_advanced_pattern pattern)
    get_directory_property(cache_vars CACHE_VARIABLES)
    foreach(entry IN LISTS cache_vars)
        get_property(
            is_advanced
            CACHE ${entry}
            PROPERTY ADVANCED)
        if(NOT is_advanced AND entry MATCHES "${pattern}")
            mark_as_advanced("${entry}")
        endif()
    endforeach()
endfunction()
