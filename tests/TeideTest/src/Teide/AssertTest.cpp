
#include "Teide/Assert.h"

#ifdef TEIDE_ASSERTS_ENABLED
#    include <gtest/gtest.h>
#    include <spdlog/spdlog.h>

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
    spdlog::info("..1");
    s_lastFailureMessage = msg;
    spdlog::info("..2");
    auto e = TestAssertException{};
    spdlog::info("..3");
    throw e;
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

#    ifndef _WIN32
TEST(AssertDeathTest, FailedAssertionWithNoHandler)
{
    EXPECT_DEATH(TEIDE_ASSERT(False(), "fooey!"), "");
}
#    endif

TEST(AssertTest, FailedAssertionWithNoMessage)
{
    spdlog::info("1");
    PushAssertHandler(TestAssertHandler);
    spdlog::info("2");
    s_lastFailureMessage.clear();
    spdlog::info("3a");
    try
    {
        TestAssertHandler("", "", SOURCE_LOCATION_CURRENT());
    }
    catch (TestAssertException)
    {
        spdlog::info("3b");
    }
    spdlog::info("4a");
    try
    {
        TEIDE_ASSERT(False());
    }
    catch (TestAssertException)
    {
        spdlog::info("4b");
    }
    spdlog::info("5");
    EXPECT_THROW(TEIDE_ASSERT(False()), TestAssertException);
    spdlog::info("6");
    EXPECT_EQ(s_lastFailureMessage, "");
    spdlog::info("7");
    PopAssertHandler();
    spdlog::info("8");
    // EXPECT_BREAK(TEIDE_ASSERT(False()), "");
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
