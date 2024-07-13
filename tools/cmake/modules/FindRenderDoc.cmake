add_library(RenderDoc::RenderDoc INTERFACE IMPORTED GLOBAL)

if(NOT RENDERDOC_INCLUDE_DIR)
    find_path(
        RENDERDOC_INCLUDE_DIR "renderdoc_app.h"
        PATHS ENV "programfiles" ENV "programfiles(x86)"
        PATH_SUFFIXES "RenderDoc")
    mark_as_advanced(RENDERDOC_INCLUDE_DIR)
endif()

if(RENDERDOC_INCLUDE_DIR)
    message(STATUS "RenderDoc found at ${RENDERDOC_INCLUDE_DIR}")
    target_include_directories(RenderDoc::RenderDoc INTERFACE ${RENDERDOC_INCLUDE_DIR})
else()
    message(STATUS "RenderDoc not found")
endif()
