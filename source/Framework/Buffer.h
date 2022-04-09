
#pragma once

#include "Framework/BytesView.h"
#include "Framework/Internal/MemoryAllocator.h"
#include "Framework/Internal/Vulkan.h"

struct BufferData
{
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags memoryFlags;
	BytesView data;
};

struct Buffer
{
	vk::DeviceSize size = 0;
	vk::UniqueBuffer buffer;
	MemoryAllocation allocation;
};

struct DynamicBuffer
{
	vk::DeviceSize size = 0;
	std::array<Buffer, 2> buffers;

	void SetData(int currentFrame, BytesView data);
};

Buffer CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, vk::Device device,
    MemoryAllocator& allocator);
