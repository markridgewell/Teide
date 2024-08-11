set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/swiftshader
    REF 65157d32945d9a75fc9a657e878a1b2f61342f03
    SHA512
        fd7e785cadc5832a2e81479d9a8a9b14509e6693effe1caa22f0de6b3574e61bb2bcab0e40c6dcc9a0d1bddda5a190920f93123e6553445e333cc01524386f47
    HEAD_REF master
    PATCHES buildfixes.patch)

file(MAKE_DIRECTORY ${SOURCE_PATH}/.git/hooks)
file(TOUCH ${SOURCE_PATH}/.git/hooks/commit-msg)
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DSWIFTSHADER_BUILD_TESTS=OFF -DSWIFTSHADER_WARNINGS_AS_ERRORS=OFF
                                         -DSWIFTSHADER_ENABLE_ASTC=OFF -DREACTOR_BACKEND=Subzero)
vcpkg_cmake_install()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/SwiftShaderConfig.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
