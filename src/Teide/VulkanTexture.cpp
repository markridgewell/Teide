
#include "VulkanTexture.h"

#include "Vulkan.h"

#include "Teide/TextureData.h"

namespace Teide
{

void VulkanTexture::GenerateMipmaps(TextureState& state, vk::CommandBuffer cmdBuffer)
{
    const auto makeBarrier = [&](vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                                 vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevel) {
        return vk::ImageMemoryBarrier{
            .srcAccessMask = srcAccessMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image.get(),
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = mipLevel,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }};
    };

    const auto origin = vk::Offset3D{.x = 0, .y = 0, .z = 0};
    auto prevMipSize = vk::Offset3D{
        .x = static_cast<int32>(properties.size.x),
        .y = static_cast<int32>(properties.size.y),
        .z = static_cast<int32>(1),
    };

    // Iterate all mip levels starting at 1
    for (uint32_t i = 1; i < properties.mipLevelCount; i++)
    {
        const vk::Offset3D currMipSize = {
            .x = prevMipSize.x > 1 ? prevMipSize.x / 2 : 1,
            .y = prevMipSize.y > 1 ? prevMipSize.y / 2 : 1,
            .z = prevMipSize.z > 1 ? prevMipSize.z / 2 : 1,
        };

        // Transition previous mip level to be a transfer source
        const auto beforeBarrier = makeBarrier(
            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal, i - 1);

        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, beforeBarrier);

        // Blit previous mip to current mip
        const vk::ImageBlit blit = {
            .srcSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffsets = {{origin, prevMipSize}},
            .dstSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = {{origin, currMipSize}},
        };

        cmdBuffer.blitImage(
            image.get(), vk::ImageLayout::eTransferSrcOptimal, image.get(), vk::ImageLayout::eTransferDstOptimal, blit,
            vk::Filter::eLinear);

        // Transition previous mip level to be ready for reading in shader
        const auto afterBarrier = makeBarrier(
            vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal, i - 1);

        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, afterBarrier);

        prevMipSize = currMipSize;
    }

    // Transition final mip level to be ready for reading in shader
    const auto finalBarrier = makeBarrier(
        vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal, properties.mipLevelCount - 1);

    cmdBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, finalBarrier);

    state.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

void VulkanTexture::TransitionToShaderInput(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
    auto newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    if (state.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        newLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    }
    DoTransition(state, cmdBuffer, newLayout, vk::PipelineStageFlagBits::eFragmentShader);
}

void VulkanTexture::TransitionToTransferSrc(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
    DoTransition(state, cmdBuffer, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer);
}

void VulkanTexture::DoTransition(
    TextureState& state, vk::CommandBuffer cmdBuffer, vk::ImageLayout newLayout,
    vk::PipelineStageFlags newPipelineStageFlags) const
{
    if (newLayout == state.layout && newPipelineStageFlags == state.lastPipelineStageUsage)
    {
        return;
    }

    TransitionImageLayout(
        cmdBuffer, image.get(), properties.format, properties.mipLevelCount, state.layout, newLayout,
        state.lastPipelineStageUsage, newPipelineStageFlags);

    state.layout = newLayout;
    state.lastPipelineStageUsage = newPipelineStageFlags;
}

void VulkanTexture::TransitionToRenderTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
    if (HasDepthOrStencilComponent(properties.format))
    {
        TransitionToDepthStencilTarget(state, cmdBuffer);
    }
    else
    {
        TransitionToColorTarget(state, cmdBuffer);
    }
}

void VulkanTexture::TransitionToColorTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
    TEIDE_ASSERT(!HasDepthOrStencilComponent(properties.format));

    DoTransition(state, cmdBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput);
}

void VulkanTexture::TransitionToDepthStencilTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
    TEIDE_ASSERT(HasDepthOrStencilComponent(properties.format));

    DoTransition(
        state, cmdBuffer, vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests);
}

void VulkanTexture::TransitionToPresentSrc(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
    DoTransition(state, cmdBuffer, vk::ImageLayout::ePresentSrcKHR, vk::PipelineStageFlagBits::eColorAttachmentOutput);
}

} // namespace Teide
