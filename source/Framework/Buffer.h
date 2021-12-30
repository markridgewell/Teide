
#pragma once

#include "Framework/BytesView.h"
#include "Framework/MemoryAllocator.h"
#include "Framework/Vulkan.h"

struct BufferData
{
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags memoryFlags;
	BytesView data;
};

struct Buffer
{
	vk::UniqueBuffer buffer;
	MemoryAllocation allocation;

	void SetData(BytesView data);
};
