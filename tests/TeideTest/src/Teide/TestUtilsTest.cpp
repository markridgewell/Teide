
#include "TestUtils.h"

#include "Teide/VulkanBuffer.h"
#include "Teide/VulkanDevice.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

namespace
{

MATCHER_P(MatchesColorTarget, renderTarget, "")
{
    (void)result_listener;
    return arg.size
        == renderTarget.size
        && arg.format
        == renderTarget.framebufferLayout.colorFormat
        && arg.mipLevelCount
        == 1
        && arg.sampleCount
        == renderTarget.framebufferLayout.sampleCount;
}

auto BytesEq(std::string_view bytesStr)
{
    return ContainerEq(HexToBytes(bytesStr));
}

TEST(TestUtilsTest, GetPixelsSync)
{
    auto device = CreateTestDevice();
    auto renderer = device->CreateRenderer(nullptr);

    const RenderTargetInfo renderTarget = {
        .size = {2, 2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Srgb,
            .captureColor = true,
        },
    };
    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };

    const Texture texture = renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();

    auto textureState = TextureState{
        .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .lastPipelineStageUsage = vk::PipelineStageFlagBits::eFragmentShader,
    };
    const auto& textureImpl = device->GetImpl(texture);

    const auto pixels
        = GetPixelsSync(textureImpl.image.get(), textureState, texture.GetSize(), texture.GetFormat(), *device);
    EXPECT_THAT(pixels, Each(0xff0000ff));
}

TEST(TestUtilsTest, GetPixelsSyncReference)
{
    auto device = CreateTestDevice();
    auto renderer = device->CreateRenderer(nullptr);

    const RenderTargetInfo renderTarget = {
        .size = {2, 2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Srgb,
            .captureColor = true,
        },
    };
    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };

    const Texture texture = renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();

    const TextureData outputData = renderer->CopyTextureData(texture).get();
    EXPECT_THAT(outputData, MatchesColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("ff 00 00 ff ff 00 00 ff ff 00 00 ff ff 00 00 ff"));
}

TEST(TestUtilsTest, GetPixelsSyncInlined)
{
    auto device = CreateTestDevice();
    auto renderer = device->CreateRenderer(nullptr);

    const RenderTargetInfo renderTarget = {
        .size = {2, 2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Srgb,
            .captureColor = true,
        },
    };
    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };

    const Texture texture = renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();

    renderer->WaitForCpu();

    // const TextureData outputData = renderer->CopyTextureData(texture).get();
    const VulkanTexture& textureImpl = device->GetImpl(texture);

    const TextureData textureData = {
        .size = textureImpl.properties.size,
        .format = textureImpl.properties.format,
        .mipLevelCount = textureImpl.properties.mipLevelCount,
        .sampleCount = textureImpl.properties.sampleCount,
    };

    const auto bufferSize = GetByteSize(textureData);

    auto buffer = device->CreateBufferUninitialized(
        bufferSize, vk::BufferUsageFlagBits::eTransferDst,
        vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom);

    device->ExecCommandsSync([&device, &texture, &buffer](vk::CommandBuffer commandBuffer) {
        const VulkanTexture& textureImpl = device->GetImpl(texture);

        TextureState textureState = {
            .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .lastPipelineStageUsage = vk::PipelineStageFlagBits::eFragmentShader,
        };
        textureImpl.TransitionToTransferSrc(textureState, commandBuffer);
        const vk::Extent3D extent = {
            .width = textureImpl.properties.size.x,
            .height = textureImpl.properties.size.y,
            .depth = 1,
        };
        CopyImageToBuffer(
            commandBuffer, textureImpl.image.get(), buffer.buffer.get(), textureImpl.properties.format, extent,
            textureImpl.properties.mipLevelCount);
    });

    const auto& data = buffer.mappedData;

    TextureData ret = textureData;
    ret.pixels.resize(data.size());
    std::ranges::copy(data, ret.pixels.data());
    const TextureData outputData = ret;

    EXPECT_THAT(outputData, MatchesColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("ff 00 00 ff ff 00 00 ff ff 00 00 ff ff 00 00 ff"));
}

} // namespace
