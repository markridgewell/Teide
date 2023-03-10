include(FetchContent)
include(FindPackageHandleStandardArgs)

set(swiftshader_repo_url "https://github.com/google/gfbuild-swiftshader")
set(swiftshader_releases_url "${swiftshader_repo_url}/releases/download/github/google/gfbuild-swiftshader")

# Download
set(version "8f075627d16bdd2a8d861e9d81df541f5cf68e2e")
if(WIN32)
    set(build "Windows_x64_Release")
    set(hash "68fb6e42b28a8df947207461cfd059751709d887")
    set(library "vulkan-1.dll")
else()
    set(build "Linux_x64_Release")
    set(hash "4096fdf838d4bd6cd60dc1133c50dc21a84b46a1")
    set(library "libvulkan.so.1")
endif()

FetchContent_Declare(
    swiftshader
    URL "${swiftshader_releases_url}/${version}/gfbuild-swiftshader-${version}-${build}.zip"
    URL_HASH SHA1=${hash})

FetchContent_MakeAvailable(swiftshader)

find_package_handle_standard_args(Swiftshader DEFAULT_MSG swiftshader_POPULATED swiftshader_SOURCE_DIR)

# Create target
if(swiftshader_POPULATED)
    set(swiftshader_library "${swiftshader_SOURCE_DIR}/lib/${library}")
    add_library(SwiftShader::SwiftShader INTERFACE IMPORTED)
    set_property(TARGET SwiftShader::SwiftShader PROPERTY INSTALL_FILES ${swiftshader_library})
endif()
