
#pragma once

#include "Teide/BytesView.h"

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <span>

struct BufferData
{
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags memoryFlags;
	BytesView data;
};

class Buffer
{
public:
	virtual ~Buffer() = default;

	virtual std::size_t GetSize() const = 0;
	virtual std::span<std::byte> GetData() const = 0;
};
