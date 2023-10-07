include(FileLists.cmake)

if(TEIDE_TEST_VERBOSE)
    list(APPEND extra_args --verbose)
endif()
if(TEIDE_TEST_WINDOWLESS)
    list(APPEND extra_args --windowless)
endif()

if(TEIDE_UNIT_TEST_SW_RENDER)
    list(APPEND extra_deps SwiftShader::SwiftShader)
    list(APPEND extra_args --sw-render)
endif()

td_add_test(
    TeideTest
    TARGET Teide
    SOURCES "${sources}"
    DEPS GTest::gtest GTest::gmock VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp ${extra_deps}
    TEST_ARGS ${extra_args})

if(TEIDE_PRECOMPILED_HEADERS)
    target_precompile_headers(
        Teide
        PRIVATE
        ../../src/Teide/VulkanConfig.h
        <algorithm>
        <ranges>
        <string>
        <vector>)
endif()
