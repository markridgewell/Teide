set(CMAKE_ASM_COMPILER clang-17)
set(CMAKE_C_COMPILER clang-17)
set(CMAKE_CXX_COMPILER clang++-17)

if(DEFINED ENV{LIBCXX_INCLUDE_DIR})
    # Environment variable is set.
    set(LIBCXX_INCLUDE_DIR $ENV{LIBCXX_INCLUDE_DIR})
else()
    # Environment variable is not set.
    if(LIBCXX_INCLUDE_DIR)
        # But CMake variable is set. Store it into the environment and use it.
        set(ENV{LIBCXX_INCLUDE_DIR} ${LIBCXX_INCLUDE_DIR})
    else()
        # Neither environment nor CMake variable is set.
        message(SEND_ERROR "LIBCXX_INCLUDE_DIR variable not set")
    endif()
endif()

if(DEFINED ENV{LIBCXX_LIB_DIR})
    # Environment variable is set.
    set(LIBCXX_LIB_DIR $ENV{LIBCXX_LIB_DIR})
else()
    # Environment variable is not set.
    if(LIBCXX_LIB_DIR)
        # But CMake variable is set. Store it into the environment and use it.
        set(ENV{LIBCXX_LIB_DIR} ${LIBCXX_LIB_DIR})
    else()
        # Neither environment nor CMake variable is set.
        message(SEND_ERROR "LIBCXX_LIB_DIR variable not set")
    endif()
endif()

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
