
#include "Teide/Assert.h"

#ifdef TEIDE_ASSERTS_ENABLED
#    include <gtest/gtest.h>

using namespace Teide;

namespace
{
bool True()
{
    return true;
}
bool False()
{
    return false;
}

class TestAssertException
{};

std::string s_lastFailureMessage;

bool TestAssertHandler(std::string_view msg, std::string_view /*expression*/, SourceLocation /*location*/)
{
    s_lastFailureMessage = msg;
    throw TestAssertException{};
}
} // namespace

#    define EXPECT_BREAK(expr, msg)                                                                                    \
        {                                                                                                              \
            PushAssertHandler(TestAssertHandler);                                                                      \
            s_lastFailureMessage.clear();                                                                              \
            EXPECT_THROW(expr, TestAssertException);                                                                   \
            EXPECT_EQ(s_lastFailureMessage, msg);                                                                      \
            PopAssertHandler();                                                                                        \
        }

#    define EXPECT_NO_BREAK(expr)                                                                                      \
        {                                                                                                              \
            PushAssertHandler(TestAssertHandler);                                                                      \
            EXPECT_NO_THROW(expr);                                                                                     \
            PopAssertHandler();                                                                                        \
        }

TEST(AssertDeathTest, FailedAssertionWithNoHandler)
{
    EXPECT_DEATH(TEIDE_ASSERT(False(), "fooey!"), "fooey!");
}

TEST(AssertTest, FailedAssertionWithNoMessage)
{
    EXPECT_BREAK(TEIDE_ASSERT(False()), "");
}

TEST(AssertTest, FailedAssertionWithMessage)
{
    EXPECT_BREAK(TEIDE_ASSERT(False(), "fooey!"), "fooey!");
}

TEST(AssertTest, FailedAssertionWithFormattedMessage)
{
    EXPECT_BREAK(TEIDE_ASSERT(False(), "{}+{}", 1, 2), "1+2");
}

TEST(AssertTest, HeldAssertionWithNoMessage)
{
    EXPECT_NO_BREAK(TEIDE_ASSERT(True()));
}

TEST(AssertTest, HeldAssertionWithMessage)
{
    EXPECT_NO_BREAK(TEIDE_ASSERT(True(), "fooey!"));
}

TEST(AssertTest, HeldAssertionWithFormattedMessage)
{
    EXPECT_NO_BREAK(TEIDE_ASSERT(True(), "{}+{}", 1, 2));
}

TEST(AssertTest, BreakWithNoMessage)
{
    EXPECT_BREAK(TEIDE_BREAK(), "");
}

TEST(AssertTest, BreakWithMessage)
{
    EXPECT_BREAK(TEIDE_BREAK("fooey!"), "fooey!");
}

TEST(AssertTest, BreakWithFormattedMessage)
{
    EXPECT_BREAK(TEIDE_BREAK("{}+{}", 1, 2), "1+2");
}
#endif
