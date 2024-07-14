set(CMAKE_ASM_COMPILER clang-18)
set(CMAKE_C_COMPILER clang-18)
set(CMAKE_CXX_COMPILER clang++-18)

message("Using clang-msan toolchain")

set(CMAKE_SYSTEM_NAME
    Linux
    CACHE STRING "")
message("CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME}")
message("CMAKE_HOST_SYSTEM_PROCESSOR: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR)
    set(CMAKE_CROSSCOMPILING
        OFF
        CACHE BOOL "")
    message("Disabling crosscompiling")
endif()
message("CMAKE_CROSSCOMPILING: ${CMAKE_CROSSCOMPILING}")

set(LIBCXX_INCLUDE_DIR $ENV{LIBCXX_INCLUDE_DIR})
set(LIBCXX_LIB_DIR $ENV{LIBCXX_LIB_DIR})
message("LIBCXX_INCLUDE_DIR: ${LIBCXX_INCLUDE_DIR}")
message("LIBCXX_LIB_DIR: ${LIBCXX_LIB_DIR}")

string(APPEND CMAKE_C_FLAGS " -fsanitize=memory -ggdb -fno-omit-frame-pointer")
string(APPEND CMAKE_CXX_FLAGS " -fsanitize=memory -ggdb -fno-omit-frame-pointer")
string(APPEND CMAKE_LINKER_FLAGS " -fsanitize=memory")

string(
    APPEND
    CMAKE_CXX_FLAGS
    " -nostdinc++ -nostdlib++ -isystem \"${LIBCXX_INCLUDE_DIR}/c++/v1\" -L \"${LIBCXX_LIB_DIR}\" \"-Wl,-rpath,${LIBCXX_LIB_DIR}\" -lc++ -lc++abi -lunwind -Wno-unused-command-line-argument"
)
