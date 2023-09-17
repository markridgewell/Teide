
#include "VulkanConfig.h"

namespace vkex
{
template <typename T>
class Array
{
public:
    Array() = default;

    template <typename U>
        requires(std::ranges::sized_range<U> && !std::same_as<U, Array<T>>)
    Array(const U& range) : m_size{static_cast<std::uint32_t>(range.size())}, m_data{new T[m_size]}
    {
        std::ranges::copy(range, m_data.get());
    }

    template <typename U>
        requires(std::ranges::sized_range<U> && !std::same_as<U, Array<T>>)
    // clang-tidy thinks this might suppress the move constructor, but the requires clause takes care of that.
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    Array(U&& range) : m_size{static_cast<std::uint32_t>(range.size())}, m_data{new T[m_size]}
    {
        std::ranges::move(range, m_data.get());
    }

    template <typename U>
        requires(std::convertible_to<U, T> && !std::same_as<U, Array<T>>)
    // See above.
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    Array(U&& singleElement) : m_size{1}, m_data{new T[1]}
    {
        *m_data.get() = std::forward<U>(singleElement);
    }

    const T* data() const { return m_data.get(); }
    T* data() { return m_data.get(); }
    std::uint32_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }

private:
    std::uint32_t m_size = 0;
    std::unique_ptr<T[]> m_data = nullptr;
};

struct SubmitInfo
{
    Array<vk::Semaphore> waitSemaphores;
    Array<vk::PipelineStageFlags> waitDstStageMask;
    Array<vk::CommandBuffer> commandBuffers;
    Array<vk::Semaphore> signalSemaphores;

    vk::SubmitInfo native() const
    {
        assert(waitDstStageMask.size() == waitSemaphores.size());
        return {
            .waitSemaphoreCount = waitSemaphores.size(),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = waitDstStageMask.data(),
            .commandBufferCount = commandBuffers.size(),
            .pCommandBuffers = commandBuffers.data(),
            .signalSemaphoreCount = signalSemaphores.size(),
            .pSignalSemaphores = signalSemaphores.data(),
        };
    }

    operator vk::SubmitInfo() const { return native(); }
};

struct PresentInfoKHR
{
    Array<vk::Semaphore> waitSemaphores;
    Array<vk::SwapchainKHR> swapchains;
    Array<std::uint32_t> imageIndices;
    Array<vk::Result> results;

    vk::PresentInfoKHR native() const
    {
        assert(imageIndices.size() == swapchains.size());
        assert(results.empty() && "Results cannot be set on a const object!");
        return {
            .waitSemaphoreCount = waitSemaphores.size(),
            .pWaitSemaphores = waitSemaphores.data(),
            .swapchainCount = swapchains.size(),
            .pSwapchains = swapchains.data(),
            .pImageIndices = imageIndices.data(),
            .pResults = nullptr,
        };
    }

    vk::PresentInfoKHR native()
    {
        assert(imageIndices.size() == swapchains.size());
        assert(results.empty() || results.size() == swapchains.size());
        return {
            .waitSemaphoreCount = waitSemaphores.size(),
            .pWaitSemaphores = waitSemaphores.data(),
            .swapchainCount = swapchains.size(),
            .pSwapchains = swapchains.data(),
            .pImageIndices = imageIndices.data(),
            .pResults = results.data(),
        };
    }

    operator vk::PresentInfoKHR() const { return native(); }

    operator vk::PresentInfoKHR() { return native(); }
};
} // namespace vkex
