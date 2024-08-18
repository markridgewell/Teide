include(FetchContent)
include(FindPackageHandleStandardArgs)
include(CMakePrintHelpers)

message("Finding custom build of libc++...")

# Determine LLVM version by running clang++ --version
execute_process(COMMAND "${CMAKE_CXX_COMPILER}" --version OUTPUT_VARIABLE VERSION_OUTPUT)
string(REGEX MATCH "version ([0-9.]+)" VERSION_STR "${VERSION_OUTPUT}")
string(REPLACE "version " "" LLVM_VERSION "${VERSION_STR}")
message("Version: ${LLVM_VERSION}")
set(PREFIX "${CMAKE_BINARY_DIR}/libcxx")

find_library(
    LIBCXX
    NAMES libc++ c++
    PATHS "${PREFIX}/lib"
    NO_DEFAULT_PATH)
mark_as_advanced(LIBCXX)
if(LIBCXX)
    set(customlibcxx_POPULATED
        TRUE
        CACHE BOOL "Located custom libc++")
    mark_as_advanced(customlibcxx_POPULATED)
else()
    FetchContent_Declare(
        customlibcxx
        SYSTEM
        GIT_REPOSITORY https://github.com/llvm/llvm-project
        GIT_TAG llvmorg-${LLVM_VERSION}
        GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
    FetchContent_GetProperties(customlibcxx)
endif()

if(NOT customlibcxx_POPULATED)
    # Fetch the content using previously declared details
    FetchContent_MakeAvailable(customlibcxx)
    set(SOURCE_DIR "${customlibcxx_SOURCE_DIR}")
    set(BINARY_DIR "${customlibcxx_BINARY_DIR}")

    string(REGEX MATCH "[0-9]+" COMPILER_VERSION "${LLVM_VERSION}")
    set(C_COMPILER clang-${COMPILER_VERSION})
    set(CXX_COMPILER clang++-${COMPILER_VERSION})
    cmake_print_variables(LLVM_VERSION COMPILER_VERSION C_COMPILER CXX_COMPILER)

    set(CMAKE_ARGS "")
    if(TEIDE_SANITIZER STREQUAL "MSAN")
        list(APPEND CMAKE_ARGS "-DLLVM_USE_SANITIZER=Memory")
    endif()

    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDERR)
    message("## CONFIGURE ##")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -G Ninja -S "${SOURCE_DIR}/runtimes" -B "${BINARY_DIR}"
            "-DLLVM_ENABLE_RUNTIMES=libcxx;libcxxabi;libunwind" "-DCMAKE_INSTALL_PREFIX=${PREFIX}"
            "-DCMAKE_C_COMPILER=${C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}" ${CMAKE_ARGS}
        COMMAND_ERROR_IS_FATAL ANY)
    message("## BUILD ##")
    execute_process(COMMAND ninja -C "${BINARY_DIR}" cxx cxxabi unwind COMMAND_ERROR_IS_FATAL ANY)
    # message("## TEST ##") execute_process(COMMAND ninja -C "${BINARY_DIR}" check-cxx check-cxxabi check-unwind
    # COMMAND_ERROR_IS_FATAL ANY)
    message("## INSTALL ##")
    execute_process(COMMAND ninja -C "${BINARY_DIR}" install-cxx install-cxxabi install-unwind
                    COMMAND_ERROR_IS_FATAL ANY)
endif()

set(LIBCXX_DIR
    "${PREFIX}"
    CACHE PATH "Install location of libc++")
mark_as_advanced(LIBCXX_DIR)

message(STATUS "libc++ found at ${PREFIX}")

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
