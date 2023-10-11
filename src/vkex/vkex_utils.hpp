
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
        std::ranges::move(std::forward<U>(range), m_data.get());
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

template <class T>
concept SizedView = std::ranges::sized_range<T> && std::ranges::view<T>;

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
        using reference = std::common_type_t<std::ranges::range_reference_t<View1>, std::ranges::range_reference_t<View2>>;

        explicit Iterator(const std::pair<View1, View2>& views) :
            m_iterators{std::ranges::begin(views.first), std::ranges::begin(views.second)}, //
            m_sentinels{std::ranges::end(views.first), std::ranges::end(views.second)},
            m_first{!(m_iterators.first == m_sentinels.first)}
        {}

        reference operator*() const
        {
            if (m_first)
            {
                return m_iterators.first.operator*();
            }
            return m_iterators.second.operator*();
        }

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

        friend bool operator==(const Iterator& a, const Sentinel s [[maybe_unused]])
        {
            return !a.m_first && a.m_iterators.second == a.m_sentinels.second;
        };

    private:
        std::pair<std::ranges::iterator_t<View1>, std::ranges::iterator_t<View2>> m_iterators;
        std::pair<std::ranges::sentinel_t<View1>, std::ranges::sentinel_t<View2>> m_sentinels;
        bool m_first = true;
    };

    explicit JoinView(View1 view1, View2 view2) :
        m_views{view1, view2}, m_size{std::ranges::size(view1) + std::ranges::size(view2)}
    {}

    auto begin() const { return Iterator(m_views); }
    auto end() const { return Sentinel(); }
    auto size() const { return m_size; }

private:
    std::pair<View1, View2> m_views;
    std::size_t m_size = 0;
};

template <typename T>
class OptionalView : public std::ranges::view_interface<OptionalView<T>>
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

        explicit Iterator(const std::optional<T>* opt) : m_value{opt} {}

        const T& operator*() const { return m_value->value(); }
        const T* operator->() const { return std::addressof(operator*()); }

        Iterator& operator++()
        {
            m_atStart = false;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            operator++();
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Sentinel s [[maybe_unused]])
        {
            return !(a.m_atStart && a.m_value->has_value());
        };

    private:
        const std::optional<T>* m_value;
        bool m_atStart = true;
    };

    explicit OptionalView(const std::optional<T>& opt) : m_value{&opt} {}

    auto begin() const { return Iterator(m_value); }
    auto end() const { return Sentinel(); }
    auto size() const { return m_value->has_value() ? 1u : 0u; }

private:
    const std::optional<T>* m_value;
};

auto MakeRefView(const std::ranges::range auto& range)
{
    return std::ranges::ref_view(range);
}

template <typename T>
auto MakeRefView(const std::optional<T>& opt)
{
    return OptionalView(opt);
}

// clang-format off
template <class T>
concept SizedViewable = requires(const T& obj)
{
    { MakeRefView(obj) } -> SizedView;
};
// clang-format on

template <typename... Views>
    requires((SizedView<Views>) && ...)
auto JoinViews(const Views&... ranges)
{
    using T = std::common_type_t<std::ranges::range_value_t<Views>...>;
    return JoinView<T, Views...>(ranges...);
}

template <typename... Ranges>
    requires((SizedViewable<Ranges>) && ...)
auto Join(const Ranges&... ranges)
{
    return JoinViews(MakeRefView(ranges)...);
}

} // namespace vkex
