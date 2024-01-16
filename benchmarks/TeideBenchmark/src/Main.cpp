
#include "Teide/Device.h"

#include <benchmark/benchmark.h>

int main(int argc, char** argv)
{
    for (const std::string_view arg : std::span(argv, static_cast<std::size_t>(argc)).subspan<1>())
    {
        if (arg == "-s" || arg == "--sw-render")
        {
            Teide::EnableSoftwareRendering();
        }
    }

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
