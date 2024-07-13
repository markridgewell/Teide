set(CMAKE_ASM_COMPILER clang-18)
set(CMAKE_C_COMPILER clang-18)
set(CMAKE_CXX_COMPILER clang++-18)

message("Using clang-msan toolchain")

if(DEFINED ENV{CMAKE_MODULE_PATH})
    message("Environment variable CMAKE_MODULE_PATH is set.")
    set(CMAKE_MODULE_PATH $ENV{CMAKE_MODULE_PATH})
else()
    message(FATAL_ERROR "Environment variable CMAKE_MODULE_PATH is not set.")
endif()

find_package(customlibcxx REQUIRED)
set(LIBCXX_INCLUDE_DIR "${LIBCXX_DIR}/include")
set(LIBCXX_LIB_DIR "${LIBCXX_DIR}/lib")
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
