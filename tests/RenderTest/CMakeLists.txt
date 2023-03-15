include(FileLists.cmake)

if(TEIDE_TEST_VERBOSE)
    set(extra_args "${extra_args} --verbose")
endif()

if(TEIDE_RENDER_TEST_SW_RENDER)
    set(extra_deps SwiftShader::SwiftShader)
    set(extra_args "${extra_args} --sw-render")
endif()

set(TEIDE_RENDER_TEST_REFERENCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/references"
    CACHE
        PATH
        "Directory to find reference images for render tests. Defaults to the software-rendered references in the repo."
)

td_add_test(
    RenderTest
    SOURCES "${sources}"
    DEPS Teide
         GTest::gtest
         GTest::gmock
         RenderDoc::RenderDoc
         ${extra_deps}
    TEST_ARGS -r "${TEIDE_RENDER_TEST_REFERENCE_DIR}" ${extra_args})
