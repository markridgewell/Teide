set(swiftshader_repo_url "https://github.com/google/gfbuild-swiftshader")
set(swiftshader_releases_url "${swiftshader_repo_url}/releases/download/github/google/gfbuild-swiftshader")

function(install_swiftshader)
    include(FetchContent)
    include(ExternalProject)

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
    set(url "${swiftshader_releases_url}/${version}/gfbuild-swiftshader-${version}-${build}.zip")

    FetchContent_Declare(
        swiftshader
        URL ${url}
        URL_HASH SHA1=${hash})

    FetchContent_MakeAvailable(swiftshader)
    set(swiftshader_library
        "${swiftshader_SOURCE_DIR}/lib/${library}"
        PARENT_SCOPE)
endfunction()

function(activate_swiftshader target)
    if(DEFINED swiftshader_library)
        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${swiftshader_library}" $<TARGET_FILE_DIR:${target}>)
    else()
        message(SEND_ERROR "Swiftshader not found")
    endif()
endfunction()
