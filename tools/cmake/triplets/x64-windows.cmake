set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

if(PORT EQUAL "swiftshader")
    set(VCPKG_LIBRARY_LINKAGE static)
endif()
