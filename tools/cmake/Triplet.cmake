if(DEFINED VCPKG_TARGET_TRIPLET)
    return()
endif()

if(NOT DEFINED TEIDE_SANITIZER)
    message(SEND_ERROR "Triplet determination requires cache variable TEIDE_SANITIZER, which was not defined.")
endif()

if(LINUX)
    # Detect host architecture
    find_program(UNAME_PATH uname REQUIRED)
    execute_process(COMMAND "${UNAME_PATH}" -m OUTPUT_VARIABLE UNAME_OUTPUT)
    string(STRIP "${UNAME_OUTPUT}" UNAME_OUTPUT)
    if(UNAME_OUTPUT STREQUAL x86_64)
        set(ARCH x64)
    elseif(UNAME_OUTPUT STREQUAL aarch64)
        set(ARCH arm64)
    else()
        message(FATAL_ERROR "Unrecognised architecture \"${UNAME_OUTPUT}\"")
    endif()

    # Detect termux
    set(ENVIRONMENT linux)
    find_program(TERMUX termux-setup-storage NO_CACHE)
    if(TERMUX)
        message(Termux!)
        set(ENVIRONMENT termux)
    endif()

    # Set triplet taking sanitizer into account
    if(TEIDE_SANITIZER STREQUAL "UBSAN")
        set(VCPKG_TARGET_TRIPLET "${ARCH}-${ENVIRONMENT}-ubsan")
    elseif(TEIDE_SANITIZER STREQUAL "MSAN")
        if(NOT LIBCXX_DIR)
            message(SEND_ERROR "LIBCXX_DIR not set!")
        endif()

        set(ENV{LIBCXX_INCLUDE_DIR} "${LIBCXX_DIR}/include")
        set(ENV{LIBCXX_LIB_DIR} "${LIBCXX_DIR}/lib")
        message("Setting env var LIBCXX_INCLUDE_DIR to $ENV{LIBCXX_INCLUDE_DIR}")
        message("Setting env var LIBCXX_LIB_DIR to $ENV{LIBCXX_LIB_DIR}")

        set(VCPKG_TARGET_TRIPLET "${ARCH}-${ENVIRONMENT}-msan")
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/toolchains/clang-msan.cmake")
    else()
        set(VCPKG_TARGET_TRIPLET "${ARCH}-${ENVIRONMENT}")
    endif()
elseif(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md")
endif()

set(VCPKG_HOST_TRIPLET
    ${VCPKG_TARGET_TRIPLET}
    CACHE STRING "Triplet respresenting host machine for vcpkg")
