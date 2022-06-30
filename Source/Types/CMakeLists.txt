td_add_library(Types SOURCES Include/Types/TextureData.h Source/TextureData.cpp)

target_link_libraries(Types PUBLIC Vulkan::Vulkan)

target_compile_definitions(Types PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Test
td_add_test(TypesTest SOURCES Test/TextureDataTest.cpp Test/Main.cpp)

target_link_libraries(TypesTest PRIVATE Types GTest::gtest GTest::gmock)
