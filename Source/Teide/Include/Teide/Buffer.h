
#pragma once

#include "Teide/BytesView.h"

#include <vulkan/vulkan.hpp>

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
	std::span<std::byte> mappedData;
};

struct DynamicBuffer
{
	vk::DeviceSize size = 0;
	std::array<Buffer, 2> buffers;

	void SetData(int currentFrame, BytesView data);
};
