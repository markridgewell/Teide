message(STATUS "Using custom vcpkg triplet")

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

if(TEIDE_SANITIZER STREQUAL "UBSAN")
    message(STATUS "UBSan detected; enabling RTTI")
    set(ENABLE_RTTI ON)
endif()
