
#include "VulkanBuffer.h"

#include <cassert>

namespace Teide
{

namespace
{
    const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;
}

VulkanBuffer CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vma::AllocationCreateFlags allocationFlags,
    vma::MemoryUsage memoryUsage, vk::Device device, vma::Allocator& allocator)
{
    auto [buffer, allocation] = allocator.createBufferUnique(
        vk::BufferCreateInfo{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive,
        },
        vma::AllocationCreateInfo{
            .flags = allocationFlags,
            .usage = memoryUsage,
        });

    std::span<byte> mappedData;
    if (allocationFlags & vma::AllocationCreateFlagBits::eMapped)
    {
        const vma::AllocationInfo allocInfo = allocator.getAllocationInfo(allocation.get());
        mappedData = {static_cast<std::byte*>(allocInfo.pMappedData), size};
    }

    return VulkanBuffer({
        .size = size,
        .buffer = vk::UniqueBuffer(buffer.release(), device),
        .allocation = std::move(allocation),
        .mappedData = mappedData,
    });
}

vk::BufferUsageFlags GetBufferUsageFlags(BufferUsage usage)
{
    using enum BufferUsage;
    switch (usage)
    {
        case Generic: return {};
        case Vertex: return vk::BufferUsageFlagBits::eVertexBuffer;
        case Index: return vk::BufferUsageFlagBits::eIndexBuffer;
        case Uniform: return vk::BufferUsageFlagBits::eUniformBuffer;
    }
    return {};
}

} // namespace Teide
