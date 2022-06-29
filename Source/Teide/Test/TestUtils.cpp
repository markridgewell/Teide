
#include "TestUtils.h"

#include <charconv>

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

std::vector<std::byte> HexToBytes(std::string_view hexString)
{
	std::vector<std::byte> ret;
	ret.reserve(hexString.size() / 2 + 1);

	while (hexString.size() >= 2)
	{
		while (isspace(hexString[0]))
		{
			hexString.remove_prefix(1);
		}

		unsigned int i = 0;
		[[maybe_unused]] const auto result = std::from_chars(hexString.data(), hexString.data() + 2, i, 16);
		assert(result.ec == std::errc{});
		ret.push_back(static_cast<std::byte>(i));
		hexString.remove_prefix(2);
	}

	return ret;
}
