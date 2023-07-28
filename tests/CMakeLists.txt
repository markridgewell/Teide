find_package(GTest REQUIRED)
find_package(SwiftShader REQUIRED)

add_subdirectory(RenderTest)
add_subdirectory(TeideTest)

if(TEIDE_TEST_COVERAGE)
    setup_coverage()
endif()
