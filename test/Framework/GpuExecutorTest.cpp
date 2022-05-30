
#include "Framework/Internal/GpuExecutor.h"

#include "Framework/Buffer.h"
#include "Framework/Internal/Vulkan.h"

#include <gmock/gmock.h>

#include <algorithm>
#include <memory>

using namespace testing;
using namespace std::chrono_literals;

namespace
{
class GpuExecutorTest : public testing::Test
{
public:
	void SetUp()
	{
		m_instance = CreateInstance();
		ASSERT_TRUE(m_instance);
		m_physicalDevice = FindPhysicalDevice(m_instance.get());
		ASSERT_TRUE(m_physicalDevice);
		auto transferQueue = GetTransferQueueIndex(m_physicalDevice);
		ASSERT_TRUE(transferQueue.has_value());
		m_queueFamilyIndex = transferQueue.value();
		const auto queueFamilies = std::array{m_queueFamilyIndex};
		m_device = CreateDevice(m_physicalDevice, queueFamilies);
		ASSERT_TRUE(m_device);
		m_queue = m_device->getQueue(m_queueFamilyIndex, 0);
		ASSERT_TRUE(m_queue);
		m_commandPool = m_device->createCommandPoolUnique({.queueFamilyIndex = m_queueFamilyIndex});
		ASSERT_TRUE(m_commandPool);

		m_allocator = std::make_unique<MemoryAllocator>(m_device.get(), m_physicalDevice);
	}

protected:
	vk::Instance GetInstance() const { return m_instance.get(); }
	vk::PhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
	vk::Device GetDevice() const { return m_device.get(); }
	std::uint32_t GetQueueFamilyIndex() const { return m_queueFamilyIndex; }
	vk::Queue GetQueue() const { return m_queue; }

	vk::UniqueCommandBuffer CreateCommandBuffer()
	{
		auto list = m_device->allocateCommandBuffersUnique({.commandPool = m_commandPool.get(), .commandBufferCount = 1});
		return std::move(list.front());
	}

	Buffer CreateHostVisibleBuffer(vk::DeviceSize size)
	{
		return CreateBufferUninitialized(
		    size, vk::BufferUsageFlagBits::eTransferDst,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_device.get(),
		    *m_allocator);
	}

private:
	std::optional<std::uint32_t> GetTransferQueueIndex(vk::PhysicalDevice physicalDevice)
	{
		std::uint32_t i = 0;
		for (const auto& queueFamily : physicalDevice.getQueueFamilyProperties())
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
			{
				return i;
			}
			++i;
		}
		return std::nullopt;
	}

	vk::PhysicalDevice FindPhysicalDevice(vk::Instance instance)
	{
		// Look for a discrete GPU
		auto physicalDevices = instance.enumeratePhysicalDevices();
		if (physicalDevices.empty())
		{
			return {};
		}

		// Prefer discrete GPUs to integrated GPUs
		std::ranges::sort(physicalDevices, [](vk::PhysicalDevice a, vk::PhysicalDevice b) {
			return (a.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			    > (b.getProperties().deviceType == vk::PhysicalDeviceType::eIntegratedGpu);
		});

		// Find the first device that supports all the queue families we need
		const auto it = std::ranges::find_if(physicalDevices, [&](vk::PhysicalDevice device) {
			// Check that all required queue families are supported
			return GetTransferQueueIndex(device).has_value();
		});
		if (it == physicalDevices.end())
		{
			return {};
		}

		return *it;
	}

	vk::DynamicLoader m_loader;
	vk::UniqueInstance m_instance;
	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_device;
	std::uint32_t m_queueFamilyIndex;
	vk::Queue m_queue;
	vk::UniqueCommandPool m_commandPool;
	std::unique_ptr<MemoryAllocator> m_allocator;
};


TEST_F(GpuExecutorTest, OneCommandBuffer)
{
	auto executor = GpuExecutor(GetDevice(), GetQueue());
	auto cmdBuffer = CreateCommandBuffer();
	auto buffer = CreateHostVisibleBuffer(12);

	cmdBuffer->begin(vk::CommandBufferBeginInfo{});
	cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 12, 0x01010101);

	const auto slot = executor.AddCommandBufferSlot();
	const auto future = executor.SubmitCommandBuffer(slot, cmdBuffer.get());
	ASSERT_THAT(future.valid(), IsTrue());
	future.wait();
	EXPECT_THAT(future.wait_for(0s), Eq(std::future_status::ready));

	const auto resultSpan = std::span(static_cast<std::uint8_t*>(buffer.allocation.mappedData), buffer.size);
	const auto result = std::vector(resultSpan.begin(), resultSpan.end());
	const auto expected = std::vector<std::uint8_t>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	EXPECT_THAT(result, Eq(expected));
}

TEST_F(GpuExecutorTest, TwoCommandBuffers)
{
	auto executor = GpuExecutor(GetDevice(), GetQueue());
	auto cmdBuffer1 = CreateCommandBuffer();
	auto cmdBuffer2 = CreateCommandBuffer();
	auto buffer = CreateHostVisibleBuffer(12);

	cmdBuffer1->begin(vk::CommandBufferBeginInfo{});
	cmdBuffer1->fillBuffer(buffer.buffer.get(), 0, 8, 0x01010101);
	cmdBuffer2->begin(vk::CommandBufferBeginInfo{});
	cmdBuffer2->fillBuffer(buffer.buffer.get(), 4, 8, 0x02020202);

	const auto slot1 = executor.AddCommandBufferSlot();
	const auto slot2 = executor.AddCommandBufferSlot();
	const auto future1 = executor.SubmitCommandBuffer(slot1, cmdBuffer1.get());
	const auto future2 = executor.SubmitCommandBuffer(slot2, cmdBuffer2.get());
	ASSERT_THAT(future1.valid(), IsTrue());
	ASSERT_THAT(future2.valid(), IsTrue());
	future2.wait();
	EXPECT_THAT(future1.wait_for(0s), Eq(std::future_status::ready));
	EXPECT_THAT(future2.wait_for(0s), Eq(std::future_status::ready));
	future1.wait(); // just in case

	const auto resultSpan = std::span(static_cast<std::uint8_t*>(buffer.allocation.mappedData), buffer.size);
	const auto result = std::vector(resultSpan.begin(), resultSpan.end());
	const auto expected = std::vector<std::uint8_t>{1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2};
	EXPECT_THAT(result, Eq(expected));
}

TEST_F(GpuExecutorTest, TwoCommandBuffersOutOfOrder)
{
	auto executor = GpuExecutor(GetDevice(), GetQueue());
	auto cmdBuffer1 = CreateCommandBuffer();
	auto cmdBuffer2 = CreateCommandBuffer();
	auto buffer = CreateHostVisibleBuffer(12);

	cmdBuffer1->begin(vk::CommandBufferBeginInfo{});
	cmdBuffer1->fillBuffer(buffer.buffer.get(), 0, 8, 0x01010101);
	cmdBuffer2->begin(vk::CommandBufferBeginInfo{});
	cmdBuffer2->fillBuffer(buffer.buffer.get(), 4, 8, 0x02020202);

	const auto slot1 = executor.AddCommandBufferSlot();
	const auto slot2 = executor.AddCommandBufferSlot();
	const auto future2 = executor.SubmitCommandBuffer(slot2, cmdBuffer2.get()); // submission order switched
	const auto future1 = executor.SubmitCommandBuffer(slot1, cmdBuffer1.get());
	ASSERT_THAT(future1.valid(), IsTrue());
	ASSERT_THAT(future2.valid(), IsTrue());
	future2.wait();
	EXPECT_THAT(future1.wait_for(0s), Eq(std::future_status::ready));
	EXPECT_THAT(future2.wait_for(0s), Eq(std::future_status::ready));
	future1.wait(); // just in case

	const auto resultSpan = std::span(static_cast<std::uint8_t*>(buffer.allocation.mappedData), buffer.size);
	const auto result = std::vector(resultSpan.begin(), resultSpan.end());
	const auto expected = std::vector<std::uint8_t>{1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2};
	EXPECT_THAT(result, Eq(expected));
}

} // namespace