
#include "Teide/Assert.h"

#include <gtest/gtest.h>

using namespace Teide;

namespace
{
class TestAssertException
{};

std::string s_lastFailureMessage;

bool TestAssertHandler(std::string_view msg, std::string_view expression, std::source_location location)
{
    s_lastFailureMessage = msg;
    throw TestAssertException{};
}
} // namespace

#define EXPECT_ASSERT(expr, msg)                                                                                       \
    {                                                                                                                  \
        PushAssertHandler(TestAssertHandler);                                                                          \
        s_lastFailureMessage.clear();                                                                                  \
        EXPECT_THROW(expr, TestAssertException);                                                                       \
        EXPECT_EQ(s_lastFailureMessage, msg);                                                                          \
        PopAssertHandler();                                                                                            \
    }

#define EXPECT_NO_ASSERT(expr)                                                                                         \
    {                                                                                                                  \
        PushAssertHandler(TestAssertHandler);                                                                          \
        EXPECT_NO_THROW(expr);                                                                                         \
        PopAssertHandler();                                                                                            \
    }

TEST(AssertTest, FailedAssertionWithNoMessage)
{
    EXPECT_ASSERT(TEIDE_ASSERT(false), "");
}

TEST(AssertTest, FailedAssertionWithMessage)
{
    EXPECT_ASSERT(TEIDE_ASSERT(false, "fooey!"), "fooey!");
}

TEST(AssertTest, FailedAssertionWithFormattedMessage)
{
    EXPECT_ASSERT(TEIDE_ASSERT(false, "{}+{}", 1, 2), "1+2");
}

TEST(AssertTest, HeldAssertionWithNoMessage)
{
    EXPECT_NO_ASSERT(TEIDE_ASSERT(true));
}

TEST(AssertTest, HeldAssertionWithMessage)
{
    EXPECT_NO_ASSERT(TEIDE_ASSERT(true, "fooey!"));
}

TEST(AssertTest, HeldAssertionWithFormattedMessage)
{
    EXPECT_NO_ASSERT(TEIDE_ASSERT(true, "{}+{}", 1, 2));
}
