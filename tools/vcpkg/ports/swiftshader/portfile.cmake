set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/swiftshader
    REF 6b8d31709ad185dbd64e80865e830a9dbe8e7559
    SHA512
        7a3e1a1c23e8e5679e74ef828765c60e942355b5e1b1e27d1d4d95db60a1f4b033a1bb04eedcfb5e9931823334be62802056cf59703283f0d3e8f09658548389
    HEAD_REF master
    PATCHES buildfixes.patch)

file(MAKE_DIRECTORY ${SOURCE_PATH}/.git/hooks)
file(TOUCH ${SOURCE_PATH}/.git/hooks/commit-msg)
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DSWIFTSHADER_BUILD_TESTS=OFF
            -DSWIFTSHADER_WARNINGS_AS_ERRORS=OFF
            -DSWIFTSHADER_ENABLE_ASTC=OFF
            -DREACTOR_BACKEND=Subzero
    NO_CHARSET_FLAG)
vcpkg_cmake_install()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/SwiftShaderConfig.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
