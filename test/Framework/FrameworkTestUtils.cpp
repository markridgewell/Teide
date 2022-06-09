
#include "FrameworkTestUtils.h"

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

vk::PhysicalDevice FindPhysicalDevice(vk::Instance instance)
{
	// Look for a discrete GPU
	auto physicalDevices = instance.enumeratePhysicalDevices();
	if (physicalDevices.empty())
	{
		return {};
	}

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
	if (it == physicalDevices.end())
	{
		return {};
	}

	return *it;
}
