function(setup_sanitizer sanitizer)
    if(sanitizer STREQUAL "MSAN")
        if(NOT CMAKE_CXX_COMPILER MATCHES "clang")
            message(SEND_ERROR "Memory Sanitizer requires Clang. Detected compiler: ${CMAKE_CXX_COMPILER}")
        endif()

        find_package(customlibcxx REQUIRED)

        set(ENV{LIBCXX_INCLUDE_DIR} "${LIBCXX_DIR}/include")
        set(ENV{LIBCXX_LIB_DIR} "${LIBCXX_DIR}/lib")
        message("Setting env var LIBCXX_INCLUDE_DIR to $ENV{LIBCXX_INCLUDE_DIR}")
        message("Setting env var LIBCXX_LIB_DIR to $ENV{LIBCXX_LIB_DIR}")

        set(msan_toolchain "${CMAKE_CURRENT_SOURCE_DIR}/tools/vcpkg/toolchains/clang-msan.cmake")
        if(TEIDE_USE_VCPKG)
            set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE
                "${msan_toolchain}"
                PARENT_SCOPE)
        else()
            set(CMAKE_TOOLCHAIN_FILE
                "${msan_toolchain}"
                PARENT_SCOPE)
        endif()
    endif()
endfunction()

function(target_enable_sanitizer target sanitizer)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if(sanitizer STREQUAL "ASAN")
            target_compile_options(${target} PRIVATE -fsanitize=address -ggdb -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        elseif(sanitizer STREQUAL "UBSAN")
            set(checks undefined,float-divide-by-zero,implicit-conversion,nullability)
            target_compile_options(${target} PRIVATE -g -fsanitize=${checks} -fno-sanitize-recover=${checks})
            target_link_options(${target} PRIVATE -fsanitize=${checks} -fno-sanitize-recover=${checks})
        elseif(sanitizer STREQUAL "MSAN")
            target_compile_options(${target} PRIVATE -fsanitize=memory -ggdb -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=memory)
        endif()
    endif()
endfunction()

function(test_enable_sanitizer test sanitizer)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if(sanitizer STREQUAL "ASAN")
            # Vulkan on Linux appears to leak memory, so disable leak checks
            set(environment "ASAN_OPTIONS=detect_leaks=0")
        elseif(sanitizer STREQUAL "UBSAN")
            set(environment "UBSAN_OPTIONS=print_stacktrace=1")
        endif()
    endif()
    set_property(
        TEST ${test}
        APPEND
        PROPERTY ENVIRONMENT "${environment}")
endfunction()
