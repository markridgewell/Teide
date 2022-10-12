td_add_library(
    Types
    SOURCES Include/Types/PipelineData.h
            Include/Types/ShaderData.h
            Include/Types/StaticMap.h
            Include/Types/TextureData.h
            Source/ShaderData.cpp
            Source/TextureData.cpp)

target_link_libraries(Types PUBLIC GeoLib Vulkan::Vulkan)

target_compile_definitions(Types PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Test
td_add_test(TypesTest SOURCES Test/ShaderDataTest.cpp Test/TextureDataTest.cpp Test/Main.cpp)

target_link_libraries(TypesTest PRIVATE Types GTest::gtest GTest::gmock)
