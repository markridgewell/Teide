
#include "VulkanBuffer.h"

#include "MemoryAllocator.h"

#include <cassert>

namespace
{
const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;
}

VulkanBuffer CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, vk::Device device,
    MemoryAllocator& allocator)
{
	VulkanBuffer ret{};

	const auto createInfo = vk::BufferCreateInfo{
	    .size = size,
	    .usage = usage,
	    .sharingMode = vk::SharingMode::eExclusive,
	};
	ret.size = size;
	ret.buffer = device.createBufferUnique(createInfo, s_allocator);
	const auto allocation = allocator.Allocate(device.getBufferMemoryRequirements(ret.buffer.get()), memoryFlags);
	device.bindBufferMemory(ret.buffer.get(), allocation.memory, allocation.offset);
	ret.mappedData = {static_cast<std::byte*>(allocation.mappedData), size};

	return ret;
}

vk::BufferUsageFlags GetBufferUsageFlags(BufferUsage usage)
{
	switch (usage)
	{
		using enum BufferUsage;
		case Generic:
			return {};
		case Vertex:
			return vk::BufferUsageFlagBits::eVertexBuffer;
		case Index:
			return vk::BufferUsageFlagBits::eIndexBuffer;
		case Uniform:
			return vk::BufferUsageFlagBits::eUniformBuffer;
	}
	return {};
}
