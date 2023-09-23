

#include "Teide/VulkanUtils.h"

#include <gmock/gmock.h>

#include <ranges>
#include <type_traits>

using namespace vkex;
using namespace testing;

TEST(VulkanUtilsTest, ArrayDefaultConstruct)
{
    const Array<int> a;
    EXPECT_THAT(a.size(), Eq(0u));
    EXPECT_THAT(a.data(), IsNull());
}

TEST(VulkanUtilsTest, ArrayConstructFromRange)
{
    const Array<int> a = std::views::iota(0, 5);
    EXPECT_THAT(a.size(), Eq(5u));
    EXPECT_THAT(a.data(), NotNull());
    EXPECT_THAT(a, ElementsAre(0, 1, 2, 3, 4));
}

TEST(VulkanUtilsTest, ArrayConstructSingleElement)
{
    const Array<int> a = 5;
    EXPECT_THAT(a.size(), Eq(1u));
    EXPECT_THAT(a.data(), NotNull());
    EXPECT_THAT(a, ElementsAre(5));
}

TEST(VulkanUtilsTest, ArrayBeginEnd)
{
    const Array<int> a = std::views::iota(0, 5);
    EXPECT_THAT(a.begin(), Eq(a.data()));
    EXPECT_THAT(a.end(), Eq(std::next(a.data(), a.size())));
}

TEST(VulkanUtilsTest, ArrayConstBeginEnd)
{
    const Array<int> a = std::views::iota(0, 5);
    EXPECT_THAT(a.begin(), Eq(a.data()));
    EXPECT_THAT(a.end(), Eq(std::next(a.data(), a.size())));
}

TEST(VulkanUtilsTest, ArrayCbeginCend)
{
    const Array<int> a = std::views::iota(0, 5);
    EXPECT_THAT(a.begin(), Eq(a.data()));
    EXPECT_THAT(a.end(), Eq(std::next(a.data(), a.size())));
}

TEST(VulkanUtilsTest, ArrayMutableAlgorithm)
{
    Array<int> a = std::views::iota(0, 5);
    std::ranges::fill(a, 42);
    EXPECT_THAT(a, ElementsAre(42, 42, 42, 42, 42));
}

TEST(VulkanUtilsTest, ArrayReset)
{
    Array<int> a;
    a.reset(10);
    EXPECT_THAT(a.size(), Eq(10u));
    EXPECT_THAT(a.data(), NotNull());
}

template <class T>
T MakeHandle(std::uintptr_t value)
{
    using NativeType = T::NativeType;
    return T(NativeType(value)); // NOLINT(performance-no-int-to-ptr)
}

MATCHER_P(Handle, v, "handle with value")
{
    (void)result_listener;
    using T = std::remove_cvref_t<decltype(arg)>;
    using NativeType = T::NativeType;
    return NativeType(arg) == NativeType(static_cast<std::uintptr_t>(v)); // NOLINT(performance-no-int-to-ptr)
}

TEST(VulkanUtilsTest, ConvertSubmitInfo)
{
    const SubmitInfo from = {
        .waitSemaphores = MakeHandle<vk::Semaphore>(1),
        .waitDstStageMask = vk::PipelineStageFlags{2},
        .commandBuffers = MakeHandle<vk::CommandBuffer>(3),
        .signalSemaphores = MakeHandle<vk::Semaphore>(4),
    };

    const vk::SubmitInfo to = from;
    EXPECT_THAT(to.waitSemaphoreCount, Eq(1u));
    EXPECT_THAT(to.pWaitSemaphores, Pointee(Handle(1)));
    EXPECT_THAT(to.pWaitDstStageMask, Pointee(vk::PipelineStageFlags{2}));
    EXPECT_THAT(to.commandBufferCount, Eq(1u));
    EXPECT_THAT(to.pCommandBuffers, Pointee(Handle(3)));
    EXPECT_THAT(to.signalSemaphoreCount, Eq(1u));
    EXPECT_THAT(to.pSignalSemaphores, Pointee(Handle(4)));
}

TEST(VulkanUtilsTest, ConvertPresentInfoKHR)
{
    const PresentInfoKHR from = {
        .waitSemaphores = MakeHandle<vk::Semaphore>(1),
        .swapchains = MakeHandle<vk::SwapchainKHR>(2),
        .imageIndices = 3u,
    };

    const vk::PresentInfoKHR to = from;
    EXPECT_THAT(to.waitSemaphoreCount, Eq(1u));
    EXPECT_THAT(to.pWaitSemaphores, Pointee(Handle(1)));
    EXPECT_THAT(to.swapchainCount, Eq(1u));
    EXPECT_THAT(to.pSwapchains, Pointee(Handle(2)));
    EXPECT_THAT(to.pImageIndices, Pointee(3u));
    EXPECT_THAT(to.pResults, IsNull());
}

TEST(VulkanUtilsTest, ConvertPresentInfoKHRWithResults)
{
    Array<vk::Result> results;
    const PresentInfoKHR from = {
        .waitSemaphores = MakeHandle<vk::Semaphore>(1),
        .swapchains = MakeHandle<vk::SwapchainKHR>(2),
        .imageIndices = 3u,
        .results = &results,
    };

    const vk::PresentInfoKHR to = from;
    EXPECT_THAT(to.waitSemaphoreCount, Eq(1u));
    EXPECT_THAT(to.pWaitSemaphores, Pointee(Handle(1)));
    EXPECT_THAT(to.swapchainCount, Eq(1u));
    EXPECT_THAT(to.pSwapchains, Pointee(Handle(2)));
    EXPECT_THAT(to.pImageIndices, Pointee(3u));
    EXPECT_THAT(to.pResults, results.data());
    EXPECT_THAT(results.size(), Eq(1u));
}
