
#pragma once

#include "Teide/GraphicsDevice.h"
#include "Teide/Visitable.h"
#include "Teide/Vulkan.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>
#include <gtest/gtest.h>

Teide::GraphicsDevicePtr CreateTestGraphicsDevice();

std::optional<std::uint32_t> GetTransferQueueIndex(vk::PhysicalDevice physicalDevice);
Teide::PhysicalDevice FindPhysicalDevice(vk::Instance instance);

std::vector<std::byte> HexToBytes(std::string_view hexString);

namespace Teide
{
void PrintTo(const Visitable auto& obj, std::ostream* os)
{
    obj.Visit([os]<typename Head, typename... Tail>(const Head& head, const Tail&... tail) {
        *os << '{';
        testing::internal::UniversalPrint(head, os);
        ((*os << ',', testing::internal::UniversalPrint(tail, os)), ...);
        *os << '}';
    });
}
} // namespace Teide
