

#include "vkex/vkex.hpp"

#include <gmock/gmock.h>

#include <ranges>
#include <type_traits>

using namespace vkex;
using namespace testing;

TEST(VkexUtilsTest, ArrayDefaultConstruct)
{
    const Array<int> a;
    EXPECT_THAT(a.size(), Eq(0u));
    EXPECT_THAT(a.data(), IsNull());
}

TEST(VkexUtilsTest, ArrayConstructFromRange)
{
    const Array<int> a = std::views::iota(0, 5);
    EXPECT_THAT(a.size(), Eq(5u));
    EXPECT_THAT(a.data(), NotNull());
    EXPECT_THAT(a, ElementsAre(0, 1, 2, 3, 4));
}

TEST(VkexUtilsTest, ArrayConstructSingleElement)
{
    const Array<int> a = 5;
    EXPECT_THAT(a.size(), Eq(1u));
    EXPECT_THAT(a.data(), NotNull());
    EXPECT_THAT(a, ElementsAre(5));
}

TEST(VkexUtilsTest, ArrayConstructFromInitializerList)
{
    const Array<int> a = {2, 4, 6, 8};
    EXPECT_THAT(a.size(), Eq(4u));
    EXPECT_THAT(a.data(), NotNull());
    EXPECT_THAT(a, ElementsAre(2, 4, 6, 8));
}

TEST(VkexUtilsTest, ArrayBeginEnd)
{
    const Array<int> a = std::views::iota(0, 5);
    EXPECT_THAT(a.begin(), Eq(a.data()));
    EXPECT_THAT(a.end(), Eq(std::next(a.data(), a.size())));
}

TEST(VkexUtilsTest, ArrayConstBeginEnd)
{
    const Array<int> a = std::views::iota(0, 5);
    EXPECT_THAT(a.begin(), Eq(a.data()));
    EXPECT_THAT(a.end(), Eq(std::next(a.data(), a.size())));
}

TEST(VkexUtilsTest, ArrayCbeginCend)
{
    const Array<int> a = std::views::iota(0, 5);
    EXPECT_THAT(a.begin(), Eq(a.data()));
    EXPECT_THAT(a.end(), Eq(std::next(a.data(), a.size())));
}

TEST(VkexUtilsTest, ArrayMutableAlgorithm)
{
    Array<int> a = std::views::iota(0, 5);
    std::ranges::fill(a, 42);
    EXPECT_THAT(a, ElementsAre(42, 42, 42, 42, 42));
}

TEST(VkexUtilsTest, ArrayReset)
{
    Array<int> a;
    a.reset(10);
    EXPECT_THAT(a.size(), Eq(10u));
    EXPECT_THAT(a.data(), NotNull());
}


TEST(VkexUtilsTest, JoinTwoVectors)
{
    const auto a = std::vector<int>{1, 2, 3};
    const auto b = std::vector<int>{4, 5};
    const auto ab = Join(a, b);
    EXPECT_THAT(ab, ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(ab.size(), Eq(5u));
}

TEST(VkexUtilsTest, JoinTwoEngagedOptionals)
{
    const auto a = std::optional<int>{1};
    const auto b = std::optional<int>{5};
    const auto ab = Join(a, b);
    EXPECT_THAT(ab, ElementsAre(1, 5));
    EXPECT_THAT(ab.size(), Eq(2u));
}

TEST(VkexUtilsTest, JoinTwoDisengagedOptionals)
{
    const auto a = std::optional<int>{};
    const auto b = std::optional<int>{};
    const auto ab = Join(a, b);
    EXPECT_THAT(ab, ElementsAre());
    EXPECT_THAT(ab.size(), Eq(0u));
}

TEST(VkexUtilsTest, JoinEngagedToDisengagedOptional)
{
    const auto a = std::optional<int>{1};
    const auto b = std::optional<int>{};
    const auto ab = Join(a, b);
    EXPECT_THAT(ab, ElementsAre(1));
    EXPECT_THAT(ab.size(), Eq(1u));
}

TEST(VkexUtilsTest, JoinDisengagedToEngagedOptional)
{
    const auto a = std::optional<int>{};
    const auto b = std::optional<int>{5};
    const auto ab = Join(a, b);
    EXPECT_THAT(ab, ElementsAre(5));
    EXPECT_THAT(ab.size(), Eq(1u));
}

TEST(VkexUtilsTest, JoinEngagedOptionalToVector)
{
    const auto a = std::optional<int>{1};
    const auto b = std::vector<int>{4, 5};
    const auto ab = Join(a, b);
    EXPECT_THAT(ab, ElementsAre(1, 4, 5));
    EXPECT_THAT(ab.size(), Eq(3u));
}

TEST(VkexUtilsTest, JoinDisengagedOptionalToVector)
{
    const auto a = std::optional<int>{};
    const auto b = std::vector<int>{4, 5};
    const auto ab = Join(a, b);
    EXPECT_THAT(ab, ElementsAre(4, 5));
    EXPECT_THAT(ab.size(), Eq(2u));
}


template <class T>
static T MakeHandle(std::uintptr_t value)
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

TEST(VkexUtilsTest, ConvertSubmitInfo)
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

TEST(VkexUtilsTest, ConvertPresentInfoKHR)
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

TEST(VkexUtilsTest, ConvertPresentInfoKHRWithResults)
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
