include(FileLists.cmake)

td_add_console_application(
    RenderTest
    SOURCES "${sources}"
    DEPS Teide GTest::gtest GTest::gmock)
target_include_directories(RenderTest PRIVATE ${STB_INCLUDE_DIRS})
activate_swiftshader(RenderTest)

if(RENDERDOC_FOUND)
    target_include_directories(RenderTest PRIVATE ${RENDERDOC_INCLUDE_DIR})
endif()
