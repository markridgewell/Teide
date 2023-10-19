include(FileLists.cmake)

td_add_application(
    TeideBenchmark
    SOURCES "${sources}"
    DEPS Teide benchmark::benchmark benchmark::benchmark_main)
