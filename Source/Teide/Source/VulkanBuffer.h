
#pragma once

#include "Teide/Buffer.h"
#include "Vulkan.h"

class MemoryAllocator;

struct VulkanBuffer : public Buffer
{
	vk::DeviceSize size = 0;
	vk::UniqueBuffer buffer;
	std::span<std::byte> mappedData;

	std::size_t GetSize() const override { return size; }
	std::span<std::byte> GetData() const override { return mappedData; }
};

template <>
struct VulkanImpl<Buffer>
{
	using type = VulkanBuffer;
};

struct VulkanDynamicBuffer : public DynamicBuffer
{
	vk::DeviceSize size = 0;
	std::array<VulkanBuffer, 2> buffers;

	std::size_t GetSize() const override { return size; }
	std::span<std::byte> GetData(int frame) const override { return buffers[frame % 2].mappedData; }
	void SetData(int frame, std::span<const std::byte> data);
};

template <>
struct VulkanImpl<DynamicBuffer>
{
	using type = VulkanDynamicBuffer;
};

VulkanBuffer CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, vk::Device device,
    MemoryAllocator& allocator);
