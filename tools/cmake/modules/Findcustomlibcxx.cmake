include(FetchContent)
include(FindPackageHandleStandardArgs)

message("Finding custom build of libc++...")
set(PREFIX "${CMAKE_BINARY_DIR}/libcxx")

FetchContent_Declare(
    customlibcxx
    SYSTEM
    GIT_REPOSITORY https://github.com/llvm/llvm-project
    GIT_TAG llvmorg-17.0.2
    GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_GetProperties(customlibcxx)

if(NOT customlibcxx_POPULATED)
    # Fetch the content using previously declared details
    FetchContent_Populate(customlibcxx)
    set(SOURCE_DIR "${customlibcxx_SOURCE_DIR}")

    FetchContent_GetProperties(
        customlibcxx
        SOURCE_DIR srcDirVar
        BINARY_DIR binDirVar
        POPULATED doneVar)
    message(STATUS "SOURCE_DIR: ${srcDirVar}")
    message(STATUS "BINARY_DIR: ${binDirVar}")
    message(STATUS "POPULATED: ${doneVar}")

    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDERR)
    message("## CONFIGURE ##")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -G Ninja -S "${srcDirVar}/runtimes" -B "${binDirVar}" "-DLLVM_ENABLE_RUNTIMES=libcxx;libcxxabi;libunwind" "-DCMAKE_INSTALL_PREFIX=${PREFIX}" -DCMAKE_CXX_COMPILER=clang++
        COMMAND_ERROR_IS_FATAL ANY)
    message("## BUILD ##")
    execute_process(
        COMMAND ninja -C "${binDirVar}" cxx cxxabi unwind
        COMMAND_ERROR_IS_FATAL ANY)
    message("## TEST ##")
    execute_process(
        COMMAND ninja -C "${binDirVar}" check-cxx check-cxxabi check-unwind
        COMMAND_ERROR_IS_FATAL ANY)
    message("## INSTALL ##")
    execute_process(
        COMMAND ninja -C "${binDirVar}" install-cxx install-cxxabi install-unwind
        COMMAND_ERROR_IS_FATAL ANY)

    set(LIBCXX_DIR
        "${PREFIX}"
        CACHE PATH "Install location of libc++")
    mark_as_advanced(LIBCXX_DIR)

    message(STATUS "libc++ found at ${PREFIX}")
endif()

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
