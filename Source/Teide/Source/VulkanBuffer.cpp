
#include "VulkanBuffer.h"

#include "MemoryAllocator.h"

#include <cassert>

namespace
{
const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;
}

void DynamicBuffer::SetData(int currentFrame, BytesView data)
{
	const auto& mappedData = buffers[currentFrame % buffers.size()].mappedData;
	assert(mappedData.size() >= data.size());
	memcpy(mappedData.data(), data.data(), data.size());
}

Buffer CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, vk::Device device,
    MemoryAllocator& allocator)
{
	Buffer ret{};

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
