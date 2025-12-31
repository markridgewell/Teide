
#include "Teide/Queue.h"

#include "TestUtils.h"

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
class QueueTest : public testing::Test
{
public:
    QueueTest() : m_instance{CreateInstance(m_loader)}, m_physicalDevice{FindPhysicalDevice(m_instance.get())} {}

    void SetUp() override
    {
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

    Queue CreateQueue() { return Queue(GetDevice(), m_queue); }

    vk::UniqueCommandBuffer CreateCommandBuffer(const char* debugName = nullptr)
    {
        auto list = m_device->allocateCommandBuffersUnique({.commandPool = m_commandPool.get(), .commandBufferCount = 1});
        if (debugName)
        {
            Teide::SetDebugName(list.front(), debugName);
        }
        return std::move(list.front());
    }

    VulkanBufferData CreateHostVisibleBuffer(vk::DeviceSize size)
    {
        return CreateBufferUninitialized(
            size, vk::BufferUsageFlagBits::eTransferDst,
            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eAutoPreferHost, m_device.get(), m_allocator.get());
    }

    static std::future<void> SubmitCommandBuffer(GpuExecutor& executor, std::uint32_t index, vk::CommandBuffer commandBuffer)
    {
        std::promise<void> promise;
        std::future<void> future = promise.get_future();
        executor.SubmitCommandBuffer(
            index, commandBuffer, [promise = std::move(promise)]() mutable { promise.set_value(); });
        return future;
    }

    void InvalidateAllocation(const vma::UniqueAllocation& allocation) const
    {
        m_allocator->invalidateAllocation(allocation.get(), 0, VK_WHOLE_SIZE);
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

template <class T>
auto One(T& thing) -> std::span<T>
{
    return std::span<T>(&thing, 1);
}

TEST_F(QueueTest, OneCommandBuffer)
{
    auto queue = CreateQueue();
    auto cmdBuffer = CreateCommandBuffer();
    auto buffer = CreateHostVisibleBuffer(12);

    cmdBuffer->begin(vk::CommandBufferBeginInfo{});
    cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 12, 0x01010101);
    cmdBuffer->end();

    auto future = queue.LazySubmit(One(cmdBuffer.get()));
    EXPECT_NO_THROW(ex::sync_wait(future));

    InvalidateAllocation(buffer.allocation);
    const auto result = std::vector(buffer.mappedData.begin(), buffer.mappedData.end());
    const auto expected = HexToBytes("01 01 01 01 01 01 01 01 01 01 01 01");
    EXPECT_THAT(result, Eq(expected));
}

TEST_F(QueueTest, TwoCommandBuffers)
{
    auto queue = CreateQueue();
    auto cmdBuffer1 = CreateCommandBuffer("cmdBuffer1");
    auto cmdBuffer2 = CreateCommandBuffer("cmdBuffer2");
    auto buffer = CreateHostVisibleBuffer(12);

    cmdBuffer1->begin(vk::CommandBufferBeginInfo{});
    cmdBuffer1->fillBuffer(buffer.buffer.get(), 0, 8, 0x01010101);
    cmdBuffer1->pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, // srcStageMask
        vk::PipelineStageFlagBits::eTransfer, // dstStageMask
        {},                                   // dependencyFlags
        {},                                   // memoryBarriers
        vk::BufferMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = buffer.buffer.get(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        {} // imageMemoryBarriers
    );
    cmdBuffer1->end();
    cmdBuffer2->begin(vk::CommandBufferBeginInfo{});
    cmdBuffer2->fillBuffer(buffer.buffer.get(), 4, 8, 0x02020202);
    cmdBuffer2->end();

    auto future1 = queue.LazySubmit(One(cmdBuffer1.get()));
    auto future2 = queue.LazySubmit(One(cmdBuffer2.get()));
    EXPECT_NO_THROW(ex::sync_wait(future1));
    EXPECT_NO_THROW(ex::sync_wait(future2));

    InvalidateAllocation(buffer.allocation);
    const auto result = std::vector(buffer.mappedData.begin(), buffer.mappedData.end());
    const auto expected = HexToBytes("01 01 01 01 02 02 02 02 02 02 02 02");
    EXPECT_THAT(result, Eq(expected));
}

TEST_F(QueueTest, SubmitMultipleCommandBuffers)
{
    auto queue = CreateQueue();
    for (int i = 0; i < 10; i++)
    {
        auto cmdBuffer = CreateCommandBuffer();
        auto buffer = CreateHostVisibleBuffer(12);
        cmdBuffer->begin(vk::CommandBufferBeginInfo{});
        cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 12, 0x01010101);
        cmdBuffer->end();

        auto future = queue.LazySubmit(One(cmdBuffer.get()));
        EXPECT_NO_THROW(ex::sync_wait(future));

        InvalidateAllocation(buffer.allocation);
        const auto result = std::vector(buffer.mappedData.begin(), buffer.mappedData.end());
        const auto expected = HexToBytes("01 01 01 01 01 01 01 01 01 01 01 01");
        EXPECT_THAT(result, Eq(expected));
    }
}

} // namespace
