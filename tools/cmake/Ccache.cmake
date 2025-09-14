include(tools/cmake/Utils.cmake)

function(_unshim exe_var)
    set(exe ${${exe_var}})
    if(NOT exe)
        return()
    endif()

    set(choco_install $ENV{ChocolateyInstall})
    cmake_path(
        IS_PREFIX
        choco_install
        ${exe}
        NORMALIZE
        is_chocolatey)
    if(is_chocolatey)
        file(GLOB_RECURSE glob_result "${choco_install}/lib/ccache.exe")
        if(NOT glob_result)
            return()
        endif()
        list(GET glob_result 0 ${exe_var})
    endif()
endfunction()

macro(_enable_ccache_msvc)
    find_program(ccache_exe ccache)
    _unshim(ccache_exe)

    if(ccache_exe)
        message(STATUS "ccache executable found at: ${ccache_exe}")

        set(compiler_exe "cl.exe")
        if(CMAKE_GENERATOR_TOOLSET STREQUAL "ClangCL")
            set(compiler_exe "clang-cl.exe")
        endif()

        file(COPY_FILE ${ccache_exe} ${CMAKE_BINARY_DIR}/${compiler_exe} ONLY_IF_DIFFERENT)

        # By default Visual Studio generators will use /Zi which is not compatible with ccache, so tell Visual Studio to
        # use /Z7 instead.
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
            "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
            PARENT_SCOPE)
        remove_arg(CMAKE_CXX_FLAGS "/Zi")
        remove_arg(CMAKE_CXX_FLAGS_DEBUG "/Zi")

        set(CMAKE_VS_GLOBALS
            "CLToolExe=${compiler_exe}" "CLToolPath=${CMAKE_BINARY_DIR}" "UseMultiToolTask=true"
            "DebugInformationFormat=OldStyle"
            PARENT_SCOPE)
    else()
        message(ERROR "ccache executable not found!")
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
