vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    REF 2f382df218d7e8516dee3b3caccb819a62b571a2
    SHA512
        689c7d48687c1ebdba86850310ff95d7d5fae79455af90e88aed2a6702968e1b48923fab39f2fee0f4725b255033b3efc135e8c17d2369d930db2737aaf28943
    HEAD_REF master)

set(VCPKG_BUILD_TYPE release) # header-only port
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
