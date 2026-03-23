
#include "TestUtils.h"

#include "Teide/Util/SafeMemCpy.h"
#include "Teide/Vulkan.h"
#include "Teide/VulkanInstance.h"
#include "Teide/VulkanSurface.h"
#include "vkex/vkex.hpp"

#include <charconv>
#include <ranges>

using namespace Teide;

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif

constexpr Teide::InstanceExtensionName OptionalExtensions[] = {
    "VK_EXT_debug_utils",
    "VK_EXT_validation_features",
};

vk::UniqueInstance CreateTestVulkanInstance(Teide::VulkanLoader& loader)
{
    return Teide::CreateInstance(loader, {.optionalExtensions = OptionalExtensions});
}

Teide::VulkanDevicePtr CreateTestDevice()
{
    VulkanLoader loader;
    vk::UniqueInstance instance = CreateTestVulkanInstance(loader);
    auto physicalDevice = FindPhysicalDevice(instance.get());

    return std::make_unique<VulkanDevice>(std::move(loader), std::move(instance), std::move(physicalDevice));
}

std::optional<std::uint32_t> GetTransferQueueIndex(vk::PhysicalDevice physicalDevice)
{
    std::uint32_t i = 0;
    for (const auto& queueFamily : physicalDevice.getQueueFamilyProperties())
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
        {
            return i;
        }
        ++i;
    }
    return std::nullopt;
}

std::vector<std::byte> HexToBytes(std::string_view hexString)
{
    std::vector<std::byte> ret;
    ret.reserve(hexString.size() / 2 + 1);

    using namespace std::literals;
    for (const auto byte : std::views::split(hexString, " "sv))
    {
        unsigned int i = 0;
        [[maybe_unused]] const auto result = std::from_chars(
            // The from_chars interface effectively forces pointer arithmetic
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::ranges::data(byte), std::ranges::data(byte) + std::ranges::size(byte), i, 16);
        TEIDE_ASSERT(result.ec == std::errc{});
        ret.push_back(static_cast<std::byte>(i));
    }

    return ret;
}

auto GetPixelsSync(vk::Image image, Teide::TextureState state, Geo::Size2i size, Teide::Format format, Teide::VulkanDevice& device)
    -> std::vector<Teide::uint32>
{
    TEIDE_ASSERT(sizeof(uint32) == GetFormatElementSize(format));

    const auto pixelCount = size.x * size.y;
    const auto byteCount = pixelCount * GetFormatElementSize(format);

    // Create buffer
    auto [allocation, buffer] = device.GetAllocator().createBufferUnique(
        vk::BufferCreateInfo{
            .size = byteCount,
            .usage = vk::BufferUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive,
        },
        vma::AllocationCreateInfo{
            .flags = vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom,
            .usage = vma::MemoryUsage::eGpuToCpu,
        });

    const vma::AllocationInfo allocInfo = device.GetAllocator().getAllocationInfo(allocation.get());
    const std::span<std::byte> mappedData = {static_cast<std::byte*>(allocInfo.pMappedData), byteCount};

    device.ExecCommandsSync([&](vk::CommandBuffer cmd) {
        // Record pipeline barrier for image -> TransferSrc
        TransitionImageLayout(
            cmd, image, format, 1, state.layout, vk::ImageLayout ::eTransferSrcOptimal, state.lastPipelineStageUsage,
            vk::PipelineStageFlagBits::eTransfer);

        // Record image to buffer copy
        cmd.copyImageToBuffer(
        image, vk::ImageLayout::eTransferSrcOptimal, buffer.get(),
        vk::BufferImageCopy{
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .layerCount = 1,
            },
            .imageExtent = {
                .width = size.x,
                .height = size.y,
                .depth = 1,
            },
        });
    });

    // Copy pixels from buffer
    auto ret = std::vector<uint32>(pixelCount);
    SafeMemCpy(ret, mappedData);
    return ret;
}

auto GetPixelsSync(Device& device, Surface& surface) -> std::optional<std::vector<uint32>>
{
    const auto& surfaceImpl = dynamic_cast<const VulkanSurface&>(surface);
    const auto image = surfaceImpl.GetLastPresentedImage();
    if (!image)
    {
        return std::nullopt;
    }

    auto& deviceImpl = dynamic_cast<VulkanDevice&>(device);
    const auto state = TextureState{
        .layout = vk::ImageLayout::ePresentSrcKHR,
        .lastPipelineStageUsage = vk::PipelineStageFlagBits::eColorAttachmentOutput,
    };
    return GetPixelsSync(image, state, surfaceImpl.GetExtent(), surfaceImpl.GetColorFormat(), deviceImpl);
}
