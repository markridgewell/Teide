
#include <vulkan/vulkan.hpp>

#include <concepts>
#include <cstdint>
#include <memory>
#include <ranges>

#ifndef VKEX_ASSERT
#    define VKEX_ASSERT VULKAN_HPP_ASSERT
#endif

namespace vkex
{
template <typename T>
class Array
{
public:
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using iterator = T*;
    using const_iterator = const T*;

    Array() = default;

    template <typename U>
        requires(std::ranges::sized_range<U> && !std::same_as<U, Array<T>>)
    Array(const U& range) : // cppcheck-suppress noExplicitConstructor
        m_size{static_cast<std::uint32_t>(range.size())}, m_data{std::make_unique<T[]>(m_size)}
    {
        std::ranges::copy(range, m_data.get());
    }

    template <typename U>
        requires(std::ranges::sized_range<U> && !std::same_as<U, Array<T>>)
    // clang-tidy thinks this might suppress the move constructor, but the requires clause takes care of that.
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    Array(U&& range) : // cppcheck-suppress noExplicitConstructor
        m_size{static_cast<std::uint32_t>(range.size())}, m_data{new T[m_size]}
    {
        std::ranges::move(range, m_data.get());
    }

    template <typename U>
        requires(std::convertible_to<U, T> && !std::same_as<U, Array<T>>)
    // See above.
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    Array(U&& singleElement) : // cppcheck-suppress noExplicitConstructor
        m_size{1}, m_data{new T[1]}
    {
        *m_data.get() = std::forward<U>(singleElement);
    }

    void reset(std::uint32_t size)
    {
        m_size = size;
        m_data = std::make_unique<T[]>(size);
    }

    const T* data() const { return m_data.get(); }
    T* data() { return m_data.get(); }
    std::uint32_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }

    const T* begin() const { return m_data.get(); }
    const T* end() const { return std::next(begin(), m_size); }
    T* begin() { return m_data.get(); }
    T* end() { return std::next(begin(), m_size); }
    const T* cbegin() const { return m_data.get(); }
    const T* cend() const { return std::next(cbegin(), m_size); }

private:
    std::uint32_t m_size = 0;
    std::unique_ptr<T[]> m_data = nullptr;
};

} // namespace vkex
