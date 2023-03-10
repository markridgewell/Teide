include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
include(${CMAKE_ROOT}/Modules/SelectLibraryConfigurations.cmake)

if(NOT Stb_INCLUDE_DIR)
    find_path(
        Stb_INCLUDE_DIR
        NAMES stb_image.h
        PATHS ${Stb_DIR}
        PATH_SUFFIXES include)
endif()

find_package_handle_standard_args(Stb DEFAULT_MSG Stb_INCLUDE_DIR)
mark_as_advanced(Stb_INCLUDE_DIR)

if(NOT TARGET Stb::Stb)
    add_library(Stb::Stb INTERFACE IMPORTED GLOBAL)
    target_include_directories(Stb::Stb INTERFACE ${Stb_INCLUDE_DIR})
endif()
