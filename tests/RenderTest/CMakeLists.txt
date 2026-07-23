include(FileLists.cmake)

if(TEIDE_TEST_VERBOSE)
    list(APPEND extra_args --verbose)
endif()

if(TEIDE_RENDER_TEST_SW_RENDER)
    list(APPEND extra_deps SwiftShader::SwiftShader)
    list(APPEND extra_args --sw-render)
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
    DEPENDENCIES
        Teide
        GTest::gtest
        GTest::gmock
        spdlog::spdlog
        RenderDoc::RenderDoc
        SDL3::SDL3
        SDL3_image::SDL3_image
        ${extra_deps}
    TEST_ARGS -r "${TEIDE_RENDER_TEST_REFERENCE_DIR}" ${extra_args})
