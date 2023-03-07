include(FileLists.cmake)

td_add_test(
    RenderTest
    SOURCES "${sources}"
    DEPS Teide GTest::gtest GTest::gmock
    TEST_ARGS -r "${CMAKE_CURRENT_SOURCE_DIR}/references")

target_include_directories(RenderTest PRIVATE ${STB_INCLUDE_DIRS})
activate_swiftshader(RenderTest)

if(RENDERDOC_FOUND)
    target_include_directories(RenderTest PRIVATE ${RENDERDOC_INCLUDE_DIR})
endif()
