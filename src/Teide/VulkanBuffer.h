
#pragma once

#include "Vulkan.h"

#include "Teide/Buffer.h"

namespace Teide
{

class MemoryAllocator;

struct VulkanBuffer : public Buffer
{
    vk::DeviceSize size = 0;
    vk::UniqueBuffer buffer;
    std::span<byte> mappedData;

    usize GetSize() const override { return size; }
    BytesView GetData() const override { return mappedData; }
};

template <>
struct VulkanImpl<Buffer>
{
    using type = VulkanBuffer;
};

VulkanBuffer CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, vk::Device device,
    MemoryAllocator& allocator);

vk::BufferUsageFlags GetBufferUsageFlags(BufferUsage usage);

} // namespace Teide
