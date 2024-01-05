add_subdirectory(TeideBenchmark)

add_custom_target(RunBenchmarks COMMAND $<TARGET_FILE:TeideBenchmark>)

install(
    TARGETS TeideBenchmark
    DESTINATION benchmarks
    CONFIGURATIONS Release
    COMPONENT benchmarks
    EXCLUDE_FROM_ALL)
