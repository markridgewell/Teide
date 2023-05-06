
#include "Teide/Format.h"

#include "Teide/Definitions.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

#ifdef NDEBUG
#    define EXPECT_UNREACHABLE(statement) if (false) { statement; }
#else
#    define EXPECT_UNREACHABLE(statement) EXPECT_DEATH(statement, UnreachableMessage)
#endif

TEST(TextureTest, InvalidFormat_Death)
{
    const auto invalidFormat = static_cast<Format>(-1);
    EXPECT_UNREACHABLE(GetFormatElementSize(invalidFormat));
}
