
#include "Framework/Buffer.h"

#include <cassert>

namespace
{
const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;
}

void DynamicBuffer::SetData(int currentFrame, BytesView data)
{
	const auto& allocation = buffers[currentFrame % buffers.size()].allocation;
	assert(allocation.mappedData);
	memcpy(allocation.mappedData, data.data(), data.size());
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
	ret.allocation = allocator.Allocate(device.getBufferMemoryRequirements(ret.buffer.get()), memoryFlags);
	device.bindBufferMemory(ret.buffer.get(), ret.allocation.memory, ret.allocation.offset);

	return ret;
}
