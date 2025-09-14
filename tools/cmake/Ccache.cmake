include(tools/cmake/Utils.cmake)

macro(_enable_ccache_msvc)
    find_program(ccache_exe ccache PATH_SUFFIXES lib)
    if(ccache_exe)
        message(STATUS "ccache executable found at: ${ccache_exe}")
        set(choco_install $ENV{ChocolateyInstall})
        cmake_path(
            IS_PREFIX
            choco_install
            ${ccache_exe}
            NORMALIZE
            is_chocolatey)
        if(is_chocolatey)
            message(STATUS "ccache was installed via chocolatey")
            file(GLOB_RECURSE glob_result "${choco_install}/lib/ccache.exe")
            if(NOT glob_result)
                message(ERROR "ccache executable not found!")
                return()
            endif()
            list(GET glob_result 0 ccache_exe)
        endif()
        message(STATUS "ccache executable found at: ${ccache_exe}")

        set(compiler_exe "cl.exe")
        if(CMAKE_GENERATOR_TOOLSET STREQUAL "ClangCL")
            set(compiler_exe "clang-cl.exe")
        endif()

        file(COPY_FILE ${ccache_exe} ${CMAKE_BINARY_DIR}/${compiler_exe} ONLY_IF_DIFFERENT)

        # By default Visual Studio generators will use /Zi which is not compatible with ccache, so tell Visual Studio to
        # use /Z7 instead.
        message(STATUS "Setting MSVC debug information format to 'Embedded'")
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
            "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
            PARENT_SCOPE)
        remove_arg(CMAKE_CXX_FLAGS "/Zi")
        remove_arg(CMAKE_CXX_FLAGS_DEBUG "/Zi")

        set(CMAKE_VS_GLOBALS
            "CLToolExe=${compiler_exe}" "CLToolPath=${CMAKE_BINARY_DIR}" "UseMultiToolTask=true"
            "DebugInformationFormat=OldStyle"
            PARENT_SCOPE)
    endif()
endmacro()

function(td_enable_ccache)
    if(CMAKE_GENERATOR MATCHES "Visual Studio .*")
        _enable_ccache_msvc()
    else()
        set(CMAKE_CXX_COMPILER_LAUNCHER
            ccache
            PARENT_SCOPE)
    endif()
endfunction()
