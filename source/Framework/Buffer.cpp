
#include "Framework/Buffer.h"

#include <cassert>

namespace
{
static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

vk::UniqueBuffer CreateBuffer(vk::Device device, vk::DeviceSize size, vk::BufferUsageFlags usage)
{
	const auto createInfo = vk::BufferCreateInfo{
	    .size = size,
	    .usage = usage,
	    .sharingMode = vk::SharingMode::eExclusive,
	};
	return device.createBufferUnique(createInfo, s_allocator);
}

void SetBufferData(const MemoryAllocation& allocation, BytesView data)
{
	assert(allocation.mappedData);
	memcpy(allocation.mappedData, data.data(), data.size());
}

void CopyBuffer(vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Buffer destination, vk::DeviceSize size)
{
	const auto copyRegion = vk::BufferCopy{
	    .srcOffset = 0,
	    .dstOffset = 0,
	    .size = size,
	};
	cmdBuffer.copyBuffer(source, destination, copyRegion);
}

} // namespace

Buffer CreateBuffer(
    vk::Device device, MemoryAllocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags memoryFlags)
{
	Buffer ret{};

	ret.buffer = CreateBuffer(device, size, usage);
	ret.allocation = allocator.Allocate(device.getBufferMemoryRequirements(ret.buffer.get()), memoryFlags);
	device.bindBufferMemory(ret.buffer.get(), ret.allocation.memory, ret.allocation.offset);

	return ret;
}

Buffer CreateBufferWithData(
    vk::Device device, MemoryAllocator& allocator, vk::CommandPool commandPool, vk::Queue queue, BytesView data,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags)
{
	Buffer ret{};

	if ((memoryFlags & vk::MemoryPropertyFlagBits::eHostCoherent) == vk::MemoryPropertyFlags{})
	{
		// Create staging buffer
		const auto stagingBuffer = CreateBuffer(device, data.size(), vk::BufferUsageFlagBits::eTransferSrc);
		const auto stagingAlloc = allocator.Allocate(
		    device.getBufferMemoryRequirements(stagingBuffer.get()),
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		device.bindBufferMemory(stagingBuffer.get(), stagingAlloc.memory, stagingAlloc.offset);
		SetBufferData(stagingAlloc, data);

		// Create device-local buffer
		ret.buffer = CreateBuffer(device, data.size(), usage | vk::BufferUsageFlagBits::eTransferDst);
		ret.allocation = allocator.Allocate(device.getBufferMemoryRequirements(ret.buffer.get()), memoryFlags);
		device.bindBufferMemory(ret.buffer.get(), ret.allocation.memory, ret.allocation.offset);
		CopyBuffer(OneShotCommandBuffer(device, commandPool, queue), stagingBuffer.get(), ret.buffer.get(), data.size());
	}
	else
	{
		ret.buffer = CreateBuffer(device, data.size(), vk::BufferUsageFlagBits::eTransferSrc);
		ret.allocation = allocator.Allocate(device.getBufferMemoryRequirements(ret.buffer.get()), memoryFlags);
		device.bindBufferMemory(ret.buffer.get(), ret.allocation.memory, ret.allocation.offset);
		SetBufferData(ret.allocation, data);
	}
	return ret;
}

void Buffer::SetData(BytesView data)
{
	SetBufferData(allocation, data);
}
