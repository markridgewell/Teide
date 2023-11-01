
#include "Teide/VulkanParameterBlock.h"

#include <gtest/gtest.h>

using namespace Teide;

TEST(ParameterBlockTest, LayoutDoesNotHaveDescriptors)
{
    VulkanParameterBlockLayout layout;
    layout.descriptorTypeCounts = {};
    layout.pushConstantRange = {};

    EXPECT_FALSE(layout.HasDescriptors());
    EXPECT_TRUE(layout.IsEmpty());
}

TEST(ParameterBlockTest, LayoutHasDescriptors)
{
    VulkanParameterBlockLayout layout;
    layout.descriptorTypeCounts = {{
        .type = vk::DescriptorType::eUniformBuffer,
        .count = 1,
    }};
    layout.pushConstantRange = {};

    EXPECT_TRUE(layout.HasDescriptors());
    EXPECT_FALSE(layout.IsEmpty());
}

TEST(ParameterBlockTest, LayoutDoesNotHavePushConstants)
{
    VulkanParameterBlockLayout layout;
    layout.descriptorTypeCounts = {};
    layout.pushConstantRange = {};

    EXPECT_FALSE(layout.HasPushConstants());
    EXPECT_TRUE(layout.IsEmpty());
}

TEST(ParameterBlockTest, LayoutHasPushConstants)
{
    VulkanParameterBlockLayout layout;
    layout.descriptorTypeCounts = {};
    layout.pushConstantRange = vk::PushConstantRange{};

    EXPECT_TRUE(layout.HasPushConstants());
    EXPECT_FALSE(layout.IsEmpty());
}

TEST(ParameterBlockTest, LayoutHasDescriptorsAndPushConstants)
{
    VulkanParameterBlockLayout layout;
    layout.descriptorTypeCounts = {{
        .type = vk::DescriptorType::eUniformBuffer,
        .count = 1,
    }};
    layout.pushConstantRange = vk::PushConstantRange{};

    EXPECT_TRUE(layout.HasDescriptors());
    EXPECT_TRUE(layout.HasPushConstants());
    EXPECT_FALSE(layout.IsEmpty());
}
