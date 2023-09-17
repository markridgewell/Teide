

#include "Teide/VulkanUtils.h"

#include <gmock/gmock.h>

using namespace vkex;
using namespace testing;

TEST(VulkanUtilsTest, ArrayDefaultConstruct)
{
    Array<int> a;
    EXPECT_THAT(a.size(), Eq(0u));
    EXPECT_THAT(a.data(), IsNull());
}
