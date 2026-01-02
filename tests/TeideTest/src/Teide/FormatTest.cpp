
#include "Teide/Format.h"

#include "Teide/Definitions.h"
#include "Teide/TestUtils.h"
#include "Teide/Vulkan.h"

#include <gmock/gmock.h>
#include <vulkan/vulkan_format_traits.hpp>

using namespace testing;
using namespace Teide;

TEST(FormatTest, InvalidFormat)
{
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    const auto invalidFormat = static_cast<Format>(-1);
    EXPECT_UNREACHABLE(GetFormatElementSize(invalidFormat));
}

class FormatValueTest : public TestWithParam<usize>
{};

TEST_P(FormatValueTest, GetFormatElementSize)
{
    const auto format = static_cast<Format>(GetParam());
    const auto vkformat = ToVulkan(format);
    const uint32 expectedSize = vk::blockSize(vkformat);

    EXPECT_THAT(GetFormatElementSize(format), Eq(expectedSize)) << vk::to_string(vkformat);
}
INSTANTIATE_TEST_SUITE_P(AllFormats, FormatValueTest, Range(usize{1}, FormatCount));
