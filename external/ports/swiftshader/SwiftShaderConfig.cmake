add_library(SwiftShader::SwiftShader INTERFACE IMPORTED)
set(BIN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../bin")
set_property(
    TARGET SwiftShader::SwiftShader #
    PROPERTY INSTALL_FILES "${BIN_DIR}/vk_swiftshader${CMAKE_SHARED_LIBRARY_SUFFIX}"
             "${BIN_DIR}/vk_swiftshader_icd.json")
