vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/swiftshader
    REF 400ac3a175a658d8157f8a363271ae7ab013c2ee
    SHA512
        44b46e6b209da195c937f9da08d8954bf64f6019755e0c5afc3e9bedbdd18c4eae4f26e5da1c2c4b3e0577287fb3abcb04aea22fb586d93ad199cc839305f9c3
    HEAD_REF master)

file(MAKE_DIRECTORY ${SOURCE_PATH}/.git/hooks)
file(TOUCH ${SOURCE_PATH}/.git/hooks/commit-msg)
vcpkg_cmake_configure(
    SOURCE_PATH
    "${SOURCE_PATH}"
    OPTIONS
    -DSWIFTSHADER_BUILD_TESTS=OFF
    --parallel)
vcpkg_cmake_install()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
