add_library(SwiftShader::SwiftShader INTERFACE IMPORTED)
set(INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
if(WIN32)
    set(LIBRARY_DIR "${INSTALL_DIR}/bin")
else()
    set(LIBRARY_DIR "${INSTALL_DIR}/lib")
endif()
set_property(
    TARGET SwiftShader::SwiftShader #
    PROPERTY INSTALL_FILES "${LIBRARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}vk_swiftshader${CMAKE_SHARED_LIBRARY_SUFFIX}"
             "${CMAKE_CURRENT_LIST_DIR}/vk_swiftshader_icd.json")
