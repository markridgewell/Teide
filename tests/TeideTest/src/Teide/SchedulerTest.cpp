
#include "Teide/Scheduler.h"

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
auto AsUint8s(std::span<const std::byte> bytes) -> std::span<const std::uint8_t>
{
    return std::span(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
}

class SchedulerTest : public testing::Test
{
public:
    SchedulerTest() : m_loader(IsSoftwareRendering()) {}

    void SetUp()
    {
        m_instance = CreateInstance(m_loader);
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

    Scheduler CreateScheduler() { return Scheduler(2, GetDevice(), GetQueue(), GetQueueFamilyIndex()); }

    VulkanBuffer CreateHostVisibleBuffer(vk::DeviceSize size)
    {
        return CreateBufferUninitialized(
            size, vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_device.get(),
            *m_allocator);
    }

private:
    VulkanLoader m_loader;
    vk::UniqueInstance m_instance;
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_device;
    std::uint32_t m_queueFamilyIndex;
    vk::Queue m_queue;
    vk::UniqueCommandPool m_commandPool;
    std::unique_ptr<MemoryAllocator> m_allocator;
};


TEST_F(SchedulerTest, ScheduleNoArguments)
{
    auto scheduler = CreateScheduler();
    const auto task = scheduler.Schedule([this]() { return 42; });

    ASSERT_THAT(task.valid(), IsTrue());
    task.wait();
    EXPECT_THAT(task.wait_for(0s), Eq(std::future_status::ready));

    const auto& result = task.get();
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, Eq(42));
}

TEST_F(SchedulerTest, ScheduleWorkerIndexArgument)
{
    auto scheduler = CreateScheduler();
    std::uint32_t storage = 0;
    const auto task1 = scheduler.Schedule([&](std::uint32_t workerIndex) { storage = workerIndex; });
    const auto task2 = scheduler.Schedule([&](std::uint32_t workerIndex) { return workerIndex < 2; });

    ASSERT_THAT(task1.valid(), IsTrue());
    ASSERT_THAT(task2.valid(), IsTrue());
    task1.wait();
    task2.wait();
    EXPECT_THAT(task1.wait_for(0s), Eq(std::future_status::ready));
    EXPECT_THAT(task2.wait_for(0s), Eq(std::future_status::ready));

    EXPECT_THAT(storage, Lt(2u));

    const auto& result = task2.get();
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, IsTrue());
}

TEST_F(SchedulerTest, ScheduleChain)
{
    auto scheduler = CreateScheduler();

    int interm[3] = {0, 0, 0};
    const auto task0 = scheduler.Schedule([&] { interm[0] = 42; });
    const auto task1 = scheduler.ScheduleAfter(task0, [&] { interm[1] = interm[0] / 2; });
    const auto task2 = scheduler.ScheduleAfter(task1, [&] { return interm[2] = interm[1] * 4; });
    const auto task3 = scheduler.ScheduleAfter(task2, [&](int prev) { return prev - 60; });

    scheduler.WaitForTasks();

    const auto result = task3.get().value();
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

    const auto resultSpan = std::span(reinterpret_cast<std::uint8_t*>(buffer.mappedData.data()), buffer.size);
    const auto resultData = std::vector(resultSpan.begin(), resultSpan.end());
    const auto expectedData = std::vector<std::uint8_t>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    EXPECT_THAT(resultData, Eq(expectedData));
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
    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& buffer = *result.value();
    const auto resultSpan = AsUint8s(buffer.GetData());
    const auto resultData = std::vector(resultSpan.begin(), resultSpan.end());
    const auto expectedData = std::vector<std::uint8_t>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    EXPECT_THAT(resultData, Eq(expectedData));
}

TEST_F(SchedulerTest, ScheduleGpuAcrossMultipleFrames)
{
    auto scheduler = CreateScheduler();

    constexpr std::uint32_t numFrames = 3;
    for (std::uint8_t i = 0; i < numFrames; i++)
    {
        auto buffer = CreateHostVisibleBuffer(4);

        const auto task = scheduler.ScheduleGpu([&](CommandBuffer& cmdBuffer) {
            cmdBuffer->fillBuffer(buffer.buffer.get(), 0, 4, i | (i << 8) | (i << 16) | (i << 24));
        });

        task.wait();

        const auto resultSpan = AsUint8s(buffer.GetData());
        const auto resultData = std::vector(resultSpan.begin(), resultSpan.end());
        const auto expectedData = std::vector<std::uint8_t>{i, i, i, i};
        EXPECT_THAT(resultData, Eq(expectedData));

        scheduler.NextFrame();
    }
}

} // namespace
