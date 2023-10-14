vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/swiftshader
    REF 400ac3a175a658d8157f8a363271ae7ab013c2ee
    SHA512 0
    HEAD_REF master)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
