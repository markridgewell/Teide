set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

if(PORT MATCHES "glslang")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DENABLE_RTTI=ON")
endif()
