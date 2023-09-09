include(FetchContent)
include(FindPackageHandleStandardArgs)

set(BUILD_COMMAND ninja -C build cxx cxxabi unwind)
set(TEST_COMMAND ninja -C build check-cxx check-cxxabi check-unwind)
set(INSTALL_COMMAND ninja -C build install-cxx install-cxxabi install-unwind)

set(PREFIX "${CMAKE_BINARY_DIR}/libcxx")

FetchContent_Declare(
    libcxx
    GIT_REPOSITORY https://github.com/llvm/llvm-project
    GIT_TAG llvmorg-16.0.4
    GIT_SHALLOW TRUE
    PREFIX
    "${PREFIX}"
    SOURCE_SUBDIR
    runtimes
    CMAKE_ARGS
    "-DLLVM_ENABLE_RUNTIMES=libcxx;libcxxabi;libunwind"
    BUILD_COMMAND
    ${BUILD_COMMAND}
    TEST_COMMAND
    ${TEST_COMMAND}
    INSTALL_COMMAND
    ${INSTALL_COMMAND})

FetchContent_MakeAvailable(libcxx)

find_package_handle_standard_args(libcxx DEFAULT_MSG libcxx_POPULATED)

if(libcxx_POPULATED
   AND EXISTS "${PREFIX}/include"
   AND EXISTS "${PREFIX}/lib")
    message(STATUS "libc++ found at ${PREFIX}")
    set(LIBCXX_INCLUDE_DIR
        "${PREFIX}/include"
        PARENT_SCOPE)
    set(LIBCXX_LIB_DIR
        "${PREFIX}/lib"
        PARENT_SCOPE)
endif()
