set(CMAKE_ASM_COMPILER clang-16)
set(CMAKE_C_COMPILER clang-16)
set(CMAKE_CXX_COMPILER clang++-16)

string(APPEND CMAKE_C_FLAGS " -fsanitize=memory -ggdb -fno-omit-frame-pointer")
string(APPEND CMAKE_CXX_FLAGS " -fsanitize=memory -ggdb -fno-omit-frame-pointer")
string(APPEND CMAKE_LINKER_FLAGS " -fsanitize=memory")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../modules)
find_package(libcxx)

string(APPEND CMAKE_CXX_FLAGS " -nostdinc++ -nostdlib++ -isystem \"${LIBCXX_INCLUDE_DIR}/c++/v1\"")
string(
    APPEND
    CMAKE_LINKER_FLAGS
    " -nostdinc++ -nostdlib++ -L \"${LIBCXX_LIB_DIR}\" \"-Wl,-rpath,${LIBCXX_LIB_DIR}\" -lc++ -Wno-unused-command-line-argument"
)
