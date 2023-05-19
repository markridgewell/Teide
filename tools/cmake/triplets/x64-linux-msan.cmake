set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

message("VCPKG_CXX_FLAGS: ${VCPKG_CXX_FLAGS}")
message("VCPKG_LINKER_FLAGS: ${VCPKG_LINKER_FLAGS}")
set(VCPKG_CXX_FLAGS "-fsanitize=memory -ggdb -fno-omit-frame-pointer")
set(VCPKG_C_FLAGS "-fsanitize=memory -ggdb -fno-omit-frame-pointer")
set(VCPKG_LINKER_FLAGS "-fsanitize=memory")

message("VCPKG_CXX_FLAGS: ${VCPKG_CXX_FLAGS}")
message("VCPKG_LINKER_FLAGS: ${VCPKG_LINKER_FLAGS}")
