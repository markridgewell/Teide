include(FileLists.cmake)

if(TEIDE_USE_SWIFTSHADER)
    list(APPEND extra_deps SwiftShader::SwiftShader)
endif()

td_add_application(
    TeideBenchmark
    SOURCES "${sources}"
    DEPS Teide benchmark::benchmark ${extra_deps})
