set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_HOST_SYSTEM_NAME=Linux;-DCMAKE_CROSSCOMPILING=OFF")

set(undef_android "-U__ANDROID__ -UANDROID")

if(PORT STREQUAL "sdl2")
    set(VCPKG_C_FLAGS "${undef_android} -DSDL_main=main")
    set(VCPKG_CXX_FLAGS "${VCPKG_C_FLAGS}")
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DSDL_KMSDRM=OFF")
endif()

if(PORT STREQUAL "swiftshader")
    set(VCPKG_CXX_FLAGS "${undef_android} -lX11 -landroid-shmem")
endif()
