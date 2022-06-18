
#include "Teide/Internal/MemoryAllocator.h"

#include "Teide/Internal/Vulkan.h"

#include <spdlog/spdlog.h>

#include <algorithm>

namespace
{
static uint32_t s_nextBlockID = 0;

uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags flags)
{
	const auto memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags))
		{
			return i;
		}
	}

	throw VulkanError("Failed to find suitable memory type");
}

} // namespace

MemoryAllocation MemoryAllocator::Allocate(const vk::MemoryRequirements& requirements, vk::MemoryPropertyFlags flags)
{
	const auto lock = std::scoped_lock(m_mutex);

	const uint32_t memoryType = FindMemoryType(m_physicalDevice, requirements.memoryTypeBits, flags);

	MemoryBlock& block = FindMemoryBlock(memoryType, requirements.size, requirements.alignment);
	const auto offset = (((block.consumedSize - 1) / requirements.alignment) + 1) * requirements.alignment;
	if (offset + requirements.size > block.capacity)
	{
		throw VulkanError("Out of memory!");
	}
	block.consumedSize = offset + requirements.size;

	void* const mappedData = block.mappedData.get();

	return {
	    .memory = block.memory.get(),
	    .offset = offset,
	    .size = requirements.size,
	    .mappedData = mappedData ? static_cast<char*>(mappedData) + offset : nullptr,
	};
}

void MemoryAllocator::DeallocateAll()
{
	const auto lock = std::scoped_lock(m_mutex);

	for (MemoryBlock& block : m_memoryBlocks)
	{
		block.consumedSize = 0;
	}
}

MemoryAllocator::MemoryBlock&
MemoryAllocator::FindMemoryBlock(uint32_t memoryType, vk::DeviceSize availableSize, vk::DeviceSize alignment)
{
	const auto it = std::ranges::find_if(m_memoryBlocks, [=](const MemoryBlock& block) {
		if (block.memoryType != memoryType)
		{
			return false;
		}

		// Check if the block has enough space for the allocation
		const auto offset = (((block.consumedSize - 1) / alignment) + 1) * alignment;
		return offset + availableSize <= block.capacity;
	});

	if (it == m_memoryBlocks.end())
	{
		// No compatible memory block found; create a new one

		// Make sure the new block has at least enough space for the allocation
		const auto newBlockSize = std::max(MemoryBlockSize, availableSize);

		const auto allocInfo = vk::MemoryAllocateInfo{
		    .allocationSize = newBlockSize,
		    .memoryTypeIndex = memoryType,
		};

		spdlog::info("Allocating {} bytes of memory in memory type {}", allocInfo.allocationSize, allocInfo.memoryTypeIndex);

		m_memoryBlocks.push_back({
		    .capacity = newBlockSize,
		    .consumedSize = 0,
		    .memoryType = memoryType,
		    .memory = m_device.allocateMemoryUnique(allocInfo),
		    .id = s_nextBlockID++,
		});
		auto& block = m_memoryBlocks.back();

		// Persistently map memory if it's host visible
		const auto memoryProperties = m_physicalDevice.getMemoryProperties().memoryTypes[memoryType];
		if ((memoryProperties.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) != vk::MemoryPropertyFlags{})
		{
			block.mappedData = MappedMemory(
			    m_device.mapMemory(block.memory.get(), 0, block.capacity), MemoryUnmapper{m_device, block.memory.get()});
		}

		const bool isDeviceLocal
		    = (memoryProperties.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags{};
		SetDebugName(
		    block.memory, "{}Memory{} ({} MB)", isDeviceLocal ? "Device" : "Host", block.id, newBlockSize / 1024 / 1024);

		return block;
	}
	return *it;
}
