
#pragma once

#include "Framework/BytesView.h"
#include "Framework/MemoryAllocator.h"
#include "Framework/Vulkan.h"

struct Buffer
{
	vk::UniqueBuffer buffer;
	MemoryAllocation allocation;

	void SetData(BytesView data);
};

Buffer CreateBuffer(
    vk::Device device, MemoryAllocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags memoryFlags);

Buffer CreateBufferWithData(
    vk::Device device, MemoryAllocator& allocator, vk::CommandPool commandPool, vk::Queue queue, BytesView data,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags);
