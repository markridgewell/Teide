add_library(RenderDoc::RenderDoc INTERFACE IMPORTED GLOBAL)

if(NOT RENDERDOC_INCLUDE_DIR)
    find_path(RENDERDOC_INCLUDE_DIR "renderdoc_app.h")
endif()

if(RENDERDOC_INCLUDE_DIR)
    message(STATUS "RenderDoc found at ${RENDERDOC_INCLUDE_DIR}")
    target_include_directories(RenderDoc::RenderDoc INTERFACE ${RENDERDOC_INCLUDE_DIR})
endif()
