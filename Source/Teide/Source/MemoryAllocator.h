
#pragma once

#include <vulkan/vulkan.hpp>

#include <mutex>
#include <vector>

constexpr vk::DeviceSize MemoryBlockSize = 64 * 1024 * 1024;

struct MemoryAllocation
{
	vk::DeviceMemory memory;
	vk::DeviceSize offset;
	vk::DeviceSize size;
	void* mappedData = nullptr;
};

// The world's simplest memory allocator
class MemoryAllocator
{
public:
	explicit MemoryAllocator(vk::Device device, vk::PhysicalDevice physicalDevice) :
	    m_device{device}, m_physicalDevice{physicalDevice}
	{}

	MemoryAllocation Allocate(const vk::MemoryRequirements& requirements, vk::MemoryPropertyFlags flags);

	void DeallocateAll();

private:
	struct MemoryUnmapper
	{
		vk::Device device;
		vk::DeviceMemory memory;

		void operator()(void*) { device.unmapMemory(memory); }
	};

	using MappedMemory = std::unique_ptr<void, MemoryUnmapper>;

	struct MemoryBlock
	{
		vk::DeviceSize capacity;
		vk::DeviceSize consumedSize;
		uint32_t memoryType;
		vk::UniqueDeviceMemory memory;
		MappedMemory mappedData;
		uint32_t id;
	};

	MemoryBlock& FindMemoryBlock(uint32_t memoryType, vk::DeviceSize availableSize, vk::DeviceSize alignment);

	std::mutex m_mutex;
	vk::Device m_device;
	vk::PhysicalDevice m_physicalDevice;
	std::vector<MemoryBlock> m_memoryBlocks;
};
