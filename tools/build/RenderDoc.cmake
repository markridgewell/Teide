function(find_renderdoc)
    find_path(RENDERDOC_INCLUDE_DIR "renderdoc_app.h")
    if("${RENDERDOC_INCLUDE_DIR}" EQUAL "RENDERDOC_INCLUDE_DIR-NOTFOUND")
        set(RENDERDOC_FOUND FALSE)
    else()
        set(RENDERDOC_FOUND TRUE)
        message("RenderDoc found at ${RENDERDOC_INCLUDE_DIR}")
    endif()
endfunction()
