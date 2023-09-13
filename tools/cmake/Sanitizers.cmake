function(target_enable_sanitizer target sanitizer)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if(sanitizer STREQUAL "ASAN")
            target_compile_options(${target} PRIVATE -fsanitize=address -ggdb -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        elseif(sanitizer STREQUAL "UBSAN")
            set(checks undefined,float-divide-by-zero,implicit-conversion,nullability)
            target_compile_options(${target} PRIVATE -g -fsanitize=${checks} -fno-sanitize-recover=${checks})
            target_link_options(${target} PRIVATE -fsanitize=${checks} -fno-sanitize-recover=${checks})
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
