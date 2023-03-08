include(FileLists.cmake)

td_add_test(
    RenderTest
    SOURCES "${sources}"
    DEPS Teide GTest::gtest GTest::gmock RenderDoc::RenderDoc
    TEST_ARGS -r "${CMAKE_CURRENT_SOURCE_DIR}/references")

activate_swiftshader(RenderTest)
