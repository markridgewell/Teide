
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
