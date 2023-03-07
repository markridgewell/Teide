include(FileLists.cmake)

td_add_test(
    TeideTest
    SOURCES "${sources}"
    DEPS Teide
         SDL2::SDL2
         Vulkan::Vulkan
         GTest::gtest
         GTest::gmock)
target_compile_definitions(TeideTest PRIVATE ${vulkan_options})
target_include_directories(TeideTest PRIVATE "${CMAKE_SOURCE_DIR}/src")
