
#include "Teide/Scheduler.h"

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

class SchedulerTest : public testing::Test
{
public:
    SchedulerTest() : m_instance{CreateInstance(m_loader)}, m_physicalDevice{FindPhysicalDevice(m_instance.get())} {}

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
    vk::Queue GetQueue() const { return m_queue; }
    vma::Allocator GetAllocator() const { return m_allocator.get(); }

    Scheduler CreateScheduler() { return {2, GetDevice(), GetQueue(), m_physicalDevice.queueFamilies.transferFamily}; }

    VulkanBuffer CreateHostVisibleBuffer(vk::DeviceSize size)
    {
        return VulkanBuffer{CreateBufferUninitialized(
            size, vk::BufferUsageFlagBits::eTransferDst,
            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom,
            vma::MemoryUsage::eAuto, m_device.get(), m_allocator.get())};
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


TEST_F(SchedulerTest, Schedule)
{
    auto scheduler = CreateScheduler();
    const auto task = scheduler.Schedule([]() { return 42; });

    ASSERT_THAT(task.valid(), IsTrue());
    task.wait();
    EXPECT_THAT(task.wait_for(0s), Eq(std::future_status::ready));

    const auto result = task.get();
    EXPECT_THAT(result, Eq(42));
}

TEST_F(SchedulerTest, ScheduleChain)
{
    auto scheduler = CreateScheduler();

    int interm[3] = {0, 0, 0};
    const auto task0 = scheduler.Schedule([&] { interm[0] = 42; });
    const auto task1 = scheduler.ScheduleAfter(task0, [&] { interm[1] = interm[0] / 2; });
    const auto task2 = scheduler.ScheduleAfter(task1, [&] { return interm[2] = interm[1] * 4; });
    const auto task3 = scheduler.ScheduleAfter(task2, [&](int prev) { return prev - 60; });

    scheduler.WaitForCpu();

    const auto result = task3.get();
    EXPECT_THAT(result, Eq(24));
    EXPECT_THAT(interm, ElementsAre(42, 21, 84));
}

TEST_F(SchedulerTest, ScheduleGpu)
{
    auto buffer = CreateHostVisibleBuffer(12);

    auto scheduler = CreateScheduler();
    const auto task = scheduler.ScheduleGpu(
        [&buffer](CommandBuffer& cmdBuffer) { cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 12, 0x01010101); });

    ASSERT_THAT(task.valid(), IsTrue());
    task.wait();
    EXPECT_THAT(task.wait_for(0s), Eq(std::future_status::ready));

    InvalidateAllocation(buffer.allocation);
    EXPECT_THAT(buffer.mappedData, Each(Eq(std::byte{1})));
}

TEST_F(SchedulerTest, ScheduleGpuWithReturn)
{
    auto scheduler = CreateScheduler();
    const auto task = scheduler.ScheduleGpu([this](CommandBuffer& cmdBuffer) {
        auto buffer = CreateHostVisibleBuffer(12);
        cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 12, 0x01010101);
        return std::make_shared<VulkanBuffer>(std::move(buffer));
    });

    const auto& result = task.get();
    const auto& buffer = *result;

    InvalidateAllocation(buffer.allocation);
    EXPECT_THAT(buffer.mappedData, Each(Eq(byte{1})));
}

TEST_F(SchedulerTest, ScheduleGpuAcrossMultipleFrames)
{
    auto scheduler = CreateScheduler();

    constexpr uint32 numFrames = 10;
    for (uint32 i = 0; i < numFrames; i++)
    {
        auto buffer = CreateHostVisibleBuffer(4);

        const auto task = scheduler.ScheduleGpu([&](CommandBuffer& cmdBuffer) {
            cmdBuffer->fillBuffer(buffer.buffer.get(), 0u, 4u, i | (i << 8) | (i << 16) | (i << 24));
        });

        task.wait();

        InvalidateAllocation(buffer.allocation);
        EXPECT_THAT(buffer.mappedData, Each(Eq(static_cast<byte>(i))));

        scheduler.NextFrame();
    }
}

} // namespace
