
#include "TestUtils.h"

#include "Teide/DescriptorPool.h"
#include "Teide/VulkanParameterBlock.h"

#include <gmock/gmock.h>

using namespace Teide;
using namespace testing;

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

TEST(ParameterBlockTest, ConstructUniformBufferWithoutData)
{
    auto device = CreateTestGraphicsDevice();

    // Create a pblock layout with a 4-byte uniform buffer
    const Teide::ParameterBlockLayoutData layout = {
        .uniformsSize = 4,
    };
    const auto layoutPtr = Teide::MakeHandle(VulkanParameterBlockLayout(layout, device->GetVulkanDevice()));

    auto pool = Teide::DescriptorPool(device->GetVulkanDevice(), *layoutPtr, 1);

    // Don't specify any uniforms in the pblock data
    const ParameterBlockData data = {
        .layout = layoutPtr,
        .parameters = {},
    };

    // Created pblock should have a uniform buffer containing 4 zero-bytes
    const auto pblock = device->CreateTransientParameterBlock(data, "Pblock", pool);
    EXPECT_THAT(pblock.descriptorSet, IsValidVkHandle());
    EXPECT_THAT(pblock.textures, IsEmpty());
    ASSERT_THAT(pblock.uniformBuffer, NotNull());

    const auto& ubuffer = *pblock.uniformBuffer;
    EXPECT_THAT(ubuffer.buffer, IsValidVkHandle());
    EXPECT_THAT(ubuffer.allocation, IsValidVkHandle());
    EXPECT_THAT(ubuffer.size, Eq(4));
}
