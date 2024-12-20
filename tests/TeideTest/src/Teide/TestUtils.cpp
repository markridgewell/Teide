
#include "TestUtils.h"

#include <charconv>
#include <ranges>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif

Teide::VulkanDevicePtr CreateTestDevice()
{
    using namespace Teide;

    VulkanLoader loader;
    vk::UniqueInstance instance = CreateInstance(loader);
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

Teide::PhysicalDevice FindPhysicalDevice(vk::Instance instance)
{
    // Look for a discrete GPU
    auto physicalDevices = instance.enumeratePhysicalDevices();
    TEIDE_ASSERT(!physicalDevices.empty());

    // Prefer discrete GPUs to integrated GPUs
    std::ranges::sort(physicalDevices, [](vk::PhysicalDevice a, vk::PhysicalDevice b) {
        return (a.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            > (b.getProperties().deviceType == vk::PhysicalDeviceType::eIntegratedGpu);
    });

    // Find the first device that supports all the queue families we need
    const auto it = std::ranges::find_if(physicalDevices, [&](vk::PhysicalDevice device) {
        // Check that all required queue families are supported
        return GetTransferQueueIndex(device).has_value();
    });
    TEIDE_ASSERT(it != physicalDevices.end());

    const uint32_t transferQueueIndex = GetTransferQueueIndex(*it).value();

    auto ret = Teide::PhysicalDevice(*it, {.transferFamily = transferQueueIndex});
    ret.queueFamilyIndices = {transferQueueIndex};
    return ret;
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
