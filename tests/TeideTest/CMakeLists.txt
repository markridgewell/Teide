include(FileLists.cmake)

if(TEIDE_TEST_VERBOSE)
    list(APPEND extra_args --verbose)
endif()
if(TEIDE_TEST_WINDOWLESS)
    list(APPEND extra_args --gtest_filter=-SurfaceTest.*)
endif()

if(TEIDE_UNIT_TEST_SW_RENDER)
    list(APPEND extra_deps SwiftShader::SwiftShader)
    list(APPEND extra_args --sw-render)
endif()

td_add_test(
    TeideTest
    TARGET Teide
    SOURCES "${sources}"
    DEPS SDL2::SDL2 GTest::gtest GTest::gmock ${extra_deps}
    TEST_ARGS ${extra_args})
