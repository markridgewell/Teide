function(vcpkg_determine_triplet)
    if(NOT DEFINED TEIDE_SANITIZER)
        message(SEND_ERROR "Triplet determination requires cache variable TEIDE_SANITIZER, which was not defined.")
    endif()

    if(LINUX)
        # Detect host architecture
        find_program(UNAME_PATH uname REQUIRED)
        execute_process(COMMAND "${UNAME_PATH}" -m OUTPUT_VARIABLE UNAME_OUTPUT)
        string(STRIP "${UNAME_OUTPUT}" UNAME_OUTPUT)
        if(UNAME_OUTPUT STREQUAL x86_64)
            set(arch x64)
        elseif(UNAME_OUTPUT STREQUAL aarch64)
            set(arch arm64)
        else()
            message(FATAL_ERROR "Unrecognised architecture \"${UNAME_OUTPUT}\"")
        endif()

        # Detect termux
        set(environment linux)
        find_program(TERMUX termux-setup-storage NO_CACHE)
        if(TERMUX)
            set(environment termux)
        endif()

        set(triplet "${arch}-${environment}")
    elseif(WIN32)
        set(triplet "x64-windows-static-md")
    endif()

    set(VCPKG_TARGET_TRIPLET
        ${triplet}
        CACHE STRING "Triplet respresenting target machine for vcpkg")
    set(VCPKG_HOST_TRIPLET
        ${triplet}
        CACHE STRING "Triplet respresenting host machine for vcpkg")
endfunction()

function(vcpkg_init)
    if(DEFINED VCPKG_TARGET_TRIPLET)
        message(STATUS "Using pre-defined vcpkg target triplet ${VCPKG_TARGET_TRIPLET}")
    else()
        vcpkg_determine_triplet()
        message(STATUS "Target triplet for vcpkg determined to be ${VCPKG_TARGET_TRIPLET}")
    endif()

    if(DEFINED ENV{VCPKG_ROOT})
        message(STATUS "vcpkg found at $ENV{VCPKG_ROOT}")
        set(vcpkg_root "$ENV{VCPKG_ROOT}")
    elseif(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/vcpkg")
        message(STATUS "vcpkg subdirectory found")
        set(vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")
    else()
        message(STATUS "vcpkg not found, installing...")
        set(vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")
        execute_process(COMMAND git clone https://github.com/microsoft/vcpkg "${vcpkg_root}")
    endif()

    set(CMAKE_TOOLCHAIN_FILE
        "${vcpkg_root}/scripts/buildsystems/vcpkg.cmake"
        PARENT_SCOPE)
endfunction()
