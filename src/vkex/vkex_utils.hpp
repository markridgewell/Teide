
#include <vulkan/vulkan.hpp>

#include <concepts>
#include <cstdint>
#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>

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

template <typename Range, typename T>
concept SizedRangeOf = std::ranges::sized_range<Range> && std::same_as<std::ranges::range_value_t<Range>, T>;

template <typename Range, typename T>
concept SizedViewOf = SizedRangeOf<Range, T> && std::ranges::view<Range>;

template <typename T, typename View1, typename View2>
    requires(SizedViewOf<View1, T> && SizedViewOf<View2, T>)
class JoinView : public std::ranges::view_interface<JoinView<T, View1, View2>>
{
public:
    using value_type = T;

    class Sentinel
    {};

    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        explicit Iterator(const std::pair<View1, View2>& views) :
            m_iterators{views.first.begin(), views.second.begin()}, //
            m_sentinels{views.first.end(), views.second.end()},
            m_size{std::ranges::size(views.first) + std::ranges::size(views.second)}
        {}

        const T& operator*() const { return m_first ? *m_iterators.first : *m_iterators.second; }
        const T* operator->() const { return std::addressof(operator*()); }

        Iterator& operator++()
        {
            if (m_first)
            {
                ++m_iterators.first;
                m_first = !(m_iterators.first == m_sentinels.first);
            }
            else
            {
                ++m_iterators.second;
            }
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            operator++();
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Sentinel)
        {
            return !a.m_first && a.m_iterators.second == a.m_sentinels.second;
        };

    private:
        std::pair<std::ranges::iterator_t<View1>, std::ranges::iterator_t<View2>> m_iterators;
        std::pair<std::ranges::sentinel_t<View1>, std::ranges::sentinel_t<View2>> m_sentinels;
        bool m_first = true;
        std::size_t m_size = 0;
    };

    explicit JoinView(View1 view1, View2 view2) : m_views{view1, view2} {}

    auto begin() const { return Iterator(m_views); }
    auto end() const { return Sentinel(); }

private:
    std::pair<View1, View2> m_views;
};

template <typename... Ranges>
    requires((std::ranges::sized_range<Ranges>) && ...)
auto Join(const Ranges&... ranges)
{
    using T = std::common_type_t<std::ranges::range_value_t<Ranges>...>;
    return JoinView<T, decltype(std::ranges::ref_view(ranges))...>(std::ranges::ref_view(ranges)...);
}

} // namespace vkex
