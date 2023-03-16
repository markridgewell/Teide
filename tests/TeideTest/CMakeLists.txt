include(FileLists.cmake)

if(TEIDE_TEST_VERBOSE)
    list(APPEND extra_args --verbose)
endif()

if(TEIDE_UNIT_TEST_SW_RENDER)
    list(APPEND extra_deps SwiftShader::SwiftShader)
    list(APPEND extra_args --sw-render)
endif()

td_add_test(
    TeideTest
    SOURCES "${sources}"
    DEPS Teide
         SDL2::SDL2
         Vulkan::Vulkan
         GTest::gtest
         GTest::gmock
         ${extra_deps}
    TEST_ARGS ${extra_args})
target_compile_definitions(TeideTest PRIVATE ${vulkan_options})
target_include_directories(TeideTest PRIVATE "${CMAKE_SOURCE_DIR}/src")
