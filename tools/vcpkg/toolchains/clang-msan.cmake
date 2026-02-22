set(CMAKE_ASM_COMPILER clang-18)
set(CMAKE_C_COMPILER clang-18)
set(CMAKE_CXX_COMPILER clang++-18)

message("Using clang-msan toolchain")

set(CMAKE_SYSTEM_NAME
    Linux
    CACHE STRING "")
message("CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME}")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_CROSSCOMPILING
        OFF
        CACHE BOOL "")
endif()

set(LIBCXX_INCLUDE_DIR $ENV{LIBCXX_INCLUDE_DIR})
set(LIBCXX_LIB_DIR $ENV{LIBCXX_LIB_DIR})
message("LIBCXX_INCLUDE_DIR: ${LIBCXX_INCLUDE_DIR}")
message("LIBCXX_LIB_DIR: ${LIBCXX_LIB_DIR}")

set(EXTRA_CXX_FLAGS
    " -nostdinc++ -nostdlib++ -isystem ${LIBCXX_INCLUDE_DIR}/c++/v1 -L ${LIBCXX_LIB_DIR} -Wl,-rpath,${LIBCXX_LIB_DIR} -lc++ -fuse-ld=lld -Wno-unused-command-line-argument"
)

string(APPEND CMAKE_C_FLAGS " -fsanitize=memory -fsanitize-memory-track-origins -ggdb -fno-omit-frame-pointer")
string(
    APPEND
    CMAKE_CXX_FLAGS
    " -fsanitize=memory -fsanitize-memory-track-origins -ggdb -fno-omit-frame-pointer -D_LIBCPP_ENABLE_EXPERIMENTAL ${EXTRA_CXX_FLAGS}"
)
string(APPEND CMAKE_LINKER_FLAGS
       " -fsanitize=memory -fsanitize-memory-track-origins -ggdb -fno-omit-frame-pointer ${EXTRA_CXX_FLAGS}")
