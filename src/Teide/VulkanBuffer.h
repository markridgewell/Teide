
#pragma once

#include "Vulkan.h"

#include "Teide/Buffer.h"

namespace Teide
{

struct VulkanBufferData
{
    vk::DeviceSize size = 0;
    vk::UniqueBuffer buffer;
    vma::UniqueAllocation allocation;
    std::span<byte> mappedData;
};

struct VulkanBuffer : public Buffer, VulkanBufferData
{
    explicit VulkanBuffer(VulkanBufferData data) : VulkanBufferData{std::move(data)} {}

    usize GetSize() const override { return size; }
    BytesView GetData() const override { return mappedData; }
};

template <>
struct VulkanImpl<Buffer>
{
    using type = VulkanBuffer;
};

VulkanBuffer CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vma::AllocationCreateFlags allocationFlags,
    vma::MemoryUsage memoryUsage, vk::Device device, vma::Allocator& allocator);

vk::BufferUsageFlags GetBufferUsageFlags(BufferUsage usage);

} // namespace Teide
