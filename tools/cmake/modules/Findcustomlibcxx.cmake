include(FetchContent)
include(FindPackageHandleStandardArgs)

message("Finding custom build of libc++...")
if(LIBCXX_DIR)
    message(STATUS "libc++ found at ${LIBCXX_DIR} (cached)")
else()
    set(BUILD_COMMAND ninja -C build cxx cxxabi unwind)
    set(TEST_COMMAND ninja -C build check-cxx check-cxxabi check-unwind)
    set(INSTALL_COMMAND ninja -C build install-cxx install-cxxabi install-unwind)

    set(PREFIX "${CMAKE_BINARY_DIR}/libcxx")

    FetchContent_Declare(
        customlibcxx
        GIT_REPOSITORY https://github.com/llvm/llvm-project
        GIT_TAG llvmorg-17.0.2
        GIT_SHALLOW TRUE
        PREFIX "${PREFIX}"
        SOURCE_SUBDIR runtimes
        CMAKE_ARGS "-DLLVM_ENABLE_RUNTIMES=libcxx;libcxxabi;libunwind"
        BUILD_COMMAND ${BUILD_COMMAND}
        TEST_COMMAND ${TEST_COMMAND}
        INSTALL_COMMAND ${INSTALL_COMMAND})

    FetchContent_MakeAvailable(customlibcxx)

    find_package_handle_standard_args(customlibcxx DEFAULT_MSG customlibcxx_POPULATED)

    message(STATUS "customlibcxx_POPULATED: ${customlibcxx_POPULATED}")
    if(EXISTS "${PREFIX}/include")
        message(STATUS "EXISTS \"${PREFIX}/include\": True")
    else()
        message(STATUS "EXISTS \"${PREFIX}/include\": False")
    endif()
    if(EXISTS "${PREFIX}/lib")
        message(STATUS "EXISTS \"${PREFIX}/lib\": True")
    else()
        message(STATUS "EXISTS \"${PREFIX}/lib\": False")
    endif()

    if(customlibcxx_POPULATED
       AND EXISTS "${PREFIX}/include"
       AND EXISTS "${PREFIX}/lib")
        message(STATUS "libc++ found at ${PREFIX}")
        set(LIBCXX_DIR
            "${PREFIX}"
            CACHE PATH "Install location of libc++")
        mark_as_advanced(LIBCXX_DIR)
        set(LIBCXX_INCLUDE_DIR
            "${PREFIX}/include"
            PARENT_SCOPE)
        set(LIBCXX_LIB_DIR
            "${PREFIX}/lib"
            PARENT_SCOPE)
    else()
        message(STATUS "libc++ not found or not bu0lt successfully!")
    endif()
endif()
