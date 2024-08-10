set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/swiftshader
    REF 01b188e5647745195e0cc5b47a0305aff74146bd
    SHA512
        90fb54e97fd5403135142fbfa720822b96cec43d8df5b2743bb6866201ba9106664e58b5da02c6674520d7f390234e1c0c4c540aeeb79e533f46de75effac579
    HEAD_REF master
    PATCHES buildfixes.patch)

file(MAKE_DIRECTORY ${SOURCE_PATH}/.git/hooks)
file(TOUCH ${SOURCE_PATH}/.git/hooks/commit-msg)
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DSWIFTSHADER_BUILD_TESTS=OFF -DSWIFTSHADER_WARNINGS_AS_ERRORS=OFF
                                         -DSWIFTSHADER_ENABLE_ASTC=OFF -DREACTOR_BACKEND=LLVM-Submodule)
vcpkg_cmake_install()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/SwiftShaderConfig.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
