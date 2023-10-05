add_subdirectory(TeideBenchmark)

add_custom_target(RunBenchmarks COMMAND $<TARGET_FILE:TeideBenchmark>)
