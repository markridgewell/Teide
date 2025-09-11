macro(_enable_ccache_msvc)
    find_program(ccache_exe ccache)
    if(ccache_exe)
        message(STATUS "ccache executable found at: ${ccache_exe}")
        message(STATUS "ChocolateyInstall = \"$ENV{ChocolateyInstall}\"")
        file(RELATIVE_PATH relative_path $ENV{ChocolateyInstall} ${ccache_exe})
        message(STATUS "path relative to ChocolateyInstall: \"${relative_path}\"")
        message(STATUS ${ccache_exe} --shimgen-usetargetworkingdirectory)
        execute_process(COMMAND ${ccache_exe} --shimgen-usetargetworkingdirectory)
        message(STATUS ${ccache_exe} --shimgen-noop)
        execute_process(COMMAND ${ccache_exe} --shimgen-noop)
        file(COPY_FILE ${ccache_exe} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)
        message(STATUS ${CMAKE_BINARY_DIR}/cl.exe /?)
        execute_process(COMMAND ${CMAKE_BINARY_DIR}/cl.exe /?)

        # By default Visual Studio generators will use /Zi which is not compatible with ccache, so tell Visual Studio to
        # use /Z7 instead.
        message(STATUS "Setting MSVC debug information format to 'Embedded'")
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
            "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
            PARENT_SCOPE)

        set(CMAKE_VS_GLOBALS
            "CLToolExe=cl.exe" "CLToolPath=${CMAKE_BINARY_DIR}" "UseMultiToolTask=true"
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
