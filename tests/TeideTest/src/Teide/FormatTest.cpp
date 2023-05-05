
#include "Teide/Format.h"

#include "Teide/Definitions.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

constexpr int ExitCode = 42;
void TerminateGracefully()
{
    std::exit(ExitCode);
}

TEST(TextureTest, InvalidFormat_Death)
{
    EXPECT_EXIT(
        {
            std::set_terminate(TerminateGracefully);
            GetFormatElementSize(static_cast<Format>(-1));
        },
        ExitedWithCode(ExitCode), UnreachableMessage);
}
