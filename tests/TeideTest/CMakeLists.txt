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
    DEPENDENCIES
        GTest::gtest
        GTest::gmock
        spdlog::spdlog
        VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
        cpptrace::cpptrace
        ${extra_deps}
    DEFINITIONS ${vulkan_configuration_definitions}
    TEST_ARGS ${extra_args})

if(TEIDE_PRECOMPILED_HEADERS)
    target_precompile_headers(
        TeideTest
        PRIVATE
        <fmt/format.h>
        <gmock/gmock.h>
        <gtest/gtest.h>
        <spdlog/spdlog.h>
        <vulkan/vulkan.hpp>
        <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>
        <vulkan/vulkan_extension_inspection.hpp>
        <algorithm>
        <bitset>
        <optional>
        <ranges>
        <span>
        <string>
        <string_view>
        <unordered_map>
        <vector>)
endif()
