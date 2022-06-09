
#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <optional>

std::optional<std::uint32_t> GetTransferQueueIndex(vk::PhysicalDevice physicalDevice);
vk::PhysicalDevice FindPhysicalDevice(vk::Instance instance);
