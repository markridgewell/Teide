
#include "Teide/GraphicsDevice.h"
#include "Teide/Vulkan.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

Teide::GraphicsDevicePtr CreateTestGraphicsDevice();

std::optional<std::uint32_t> GetTransferQueueIndex(vk::PhysicalDevice physicalDevice);
Teide::PhysicalDevice FindPhysicalDevice(vk::Instance instance);

std::vector<std::byte> HexToBytes(std::string_view hexString);
