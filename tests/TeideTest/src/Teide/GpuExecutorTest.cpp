
#include "Teide/GpuExecutor.h"

#include "TestUtils.h"

#include "Teide/MemoryAllocator.h"
#include "Teide/TestUtils.h"
#include "Teide/Vulkan.h"
#include "Teide/VulkanBuffer.h"
#include "Teide/VulkanLoader.h"

#include <gmock/gmock.h>

#include <algorithm>
#include <memory>

using namespace testing;
using namespace std::chrono_literals;
using namespace Teide;

namespace
{
class GpuExecutorTest : public testing::Test
{
public:
    void SetUp() override
    {
        m_instance = CreateInstance(m_loader);
        ASSERT_TRUE(m_instance);
        m_physicalDevice = FindPhysicalDevice(m_instance.get());
        ASSERT_TRUE(m_physicalDevice.physicalDevice);
        m_device = CreateDevice(m_loader, m_physicalDevice);
        ASSERT_TRUE(m_device);
        m_queue = m_device->getQueue(m_physicalDevice.queueFamilies.transferFamily, 0);
        ASSERT_TRUE(m_queue);
        m_commandPool
            = m_device->createCommandPoolUnique({.queueFamilyIndex = m_physicalDevice.queueFamilies.transferFamily});
        ASSERT_TRUE(m_commandPool);

        m_allocator = CreateAllocator(m_loader, m_instance.get(), m_device.get(), m_physicalDevice.physicalDevice);
    }

protected:
    vk::Instance GetInstance() const { return m_instance.get(); }
    vk::PhysicalDevice GetPhysicalDevice() const { return m_physicalDevice.physicalDevice; }
    vk::Device GetDevice() const { return m_device.get(); }
    vk::Queue GetQueue() const { return m_queue; }

    vk::UniqueCommandBuffer CreateCommandBuffer()
    {
        auto list = m_device->allocateCommandBuffersUnique({.commandPool = m_commandPool.get(), .commandBufferCount = 1});
        return std::move(list.front());
    }

    VulkanBuffer CreateHostVisibleBuffer(vk::DeviceSize size)
    {
        return CreateBufferUninitialized(
            size, vk::BufferUsageFlagBits::eTransferDst, vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eAuto, m_device.get(), m_allocator.get());
    }

    static std::future<void> SubmitCommandBuffer(GpuExecutor& executor, std::uint32_t index, vk::CommandBuffer commandBuffer)
    {
        std::promise<void> promise;
        std::future<void> future = promise.get_future();
        executor.SubmitCommandBuffer(
            index, commandBuffer, [promise = std::move(promise)]() mutable { promise.set_value(); });
        return future;
    }

private:
    VulkanLoader m_loader;
    vk::UniqueInstance m_instance;
    PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_device;
    vk::Queue m_queue;
    vk::UniqueCommandPool m_commandPool;
    vma::UniqueAllocator m_allocator;
};


TEST_F(GpuExecutorTest, OneCommandBuffer)
{
    auto executor = GpuExecutor(GetDevice(), GetQueue());
    auto cmdBuffer = CreateCommandBuffer();
    auto buffer = CreateHostVisibleBuffer(12);

    cmdBuffer->begin(vk::CommandBufferBeginInfo{});
    cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 12, 0x01010101);

    const auto slot = executor.AddCommandBufferSlot();
    const auto future = SubmitCommandBuffer(executor, slot, cmdBuffer.get());
    ASSERT_THAT(future.valid(), IsTrue());
    future.wait();
    EXPECT_THAT(future.wait_for(0s), Eq(std::future_status::ready));

    const auto result = std::vector(buffer.mappedData.begin(), buffer.mappedData.end());
    const auto expected = HexToBytes("01 01 01 01 01 01 01 01 01 01 01 01");
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
    const auto future1 = SubmitCommandBuffer(executor, slot1, cmdBuffer1.get());
    const auto future2 = SubmitCommandBuffer(executor, slot2, cmdBuffer2.get());
    ASSERT_THAT(future1.valid(), IsTrue());
    ASSERT_THAT(future2.valid(), IsTrue());
    future2.wait();
    EXPECT_THAT(future1.wait_for(0s), Eq(std::future_status::ready));
    EXPECT_THAT(future2.wait_for(0s), Eq(std::future_status::ready));
    future1.wait(); // just in case

    const auto result = std::vector(buffer.mappedData.begin(), buffer.mappedData.end());
    const auto expected = HexToBytes("01 01 01 01 02 02 02 02 02 02 02 02");
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
    const auto future2 = SubmitCommandBuffer(executor, slot2, cmdBuffer2.get()); // submission order switched
    const auto future1 = SubmitCommandBuffer(executor, slot1, cmdBuffer1.get());
    ASSERT_THAT(future1.valid(), IsTrue());
    ASSERT_THAT(future2.valid(), IsTrue());
    future2.wait();
    EXPECT_THAT(future1.wait_for(0s), Eq(std::future_status::ready));
    EXPECT_THAT(future2.wait_for(0s), Eq(std::future_status::ready));
    future1.wait(); // just in case

    const auto result = std::vector(buffer.mappedData.begin(), buffer.mappedData.end());
    const auto expected = HexToBytes("01 01 01 01 02 02 02 02 02 02 02 02");
    EXPECT_THAT(result, Eq(expected));
}

TEST_F(GpuExecutorTest, DontWait)
{
    auto buffer = CreateHostVisibleBuffer(12);
    auto cmdBuffer = CreateCommandBuffer();
    {
        auto executor = GpuExecutor(GetDevice(), GetQueue());

        cmdBuffer->begin(vk::CommandBufferBeginInfo{});
        cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 12, 0x01010101);

        const auto slot = executor.AddCommandBufferSlot();
        const auto future = SubmitCommandBuffer(executor, slot, cmdBuffer.get());
        ASSERT_THAT(future.valid(), IsTrue());
    }
}

TEST_F(GpuExecutorTest, SubmitMultipleCommandBuffers)
{
    auto executor = GpuExecutor(GetDevice(), GetQueue());
    for (int i = 0; i < 2; i++)
    {
        auto cmdBuffer = CreateCommandBuffer();
        auto buffer = CreateHostVisibleBuffer(12);

        cmdBuffer->begin(vk::CommandBufferBeginInfo{});
        cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 12, 0x01010101);

        const auto slot = executor.AddCommandBufferSlot();
        const auto future = SubmitCommandBuffer(executor, slot, cmdBuffer.get());
        ASSERT_THAT(future.valid(), IsTrue());
        future.wait();
        EXPECT_THAT(future.wait_for(0s), Eq(std::future_status::ready));

        const auto result = std::vector(buffer.mappedData.begin(), buffer.mappedData.end());
        const auto expected = HexToBytes("01 01 01 01 01 01 01 01 01 01 01 01");
        EXPECT_THAT(result, Eq(expected));
    }
}

} // namespace
