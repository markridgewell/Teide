
#pragma once

#include "Teide/BasicTypes.h"

#include <span>
#include <type_traits>
#include <vector>

namespace Teide
{

template <class T>
concept Span = requires(T t) {
    { std::span{t} };
};

template <class T>
concept TrivialSpan
    = Span<T> && std::is_trivially_copyable_v<typename T::value_type> && std::is_standard_layout_v<typename T::value_type>;

template <class T>
concept TrivialObject
    = !Span<T> && std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> && !std::is_pointer_v<T>;

class BytesView
{
    using span_type = std::span<const byte>;

public:
    using element_type = span_type::element_type;
    using value_type = span_type::value_type;
    using size_type = span_type::size_type;
    using difference_type = span_type::difference_type;
    using pointer = span_type::pointer;
    using const_pointer = span_type::const_pointer;
    using reference = span_type::reference;
    using const_reference = span_type::const_reference;
    using iterator = span_type::iterator;
    using const_iterator = span_type::iterator;
    using reverse_iterator = span_type::reverse_iterator;

    BytesView(const byte* p, usize size) : m_span(p, size) {}

    BytesView(std::span<const byte> bytes = {}) : m_span{bytes} {} // cppcheck-suppress noExplicitConstructor

    template <TrivialSpan T>
    BytesView(const T& data) : m_span{std::as_bytes(std::span(data))} // cppcheck-suppress noExplicitConstructor
    {}

    template <TrivialObject T>
    BytesView(const T& data) : m_span{std::as_bytes(std::span(&data, 1))} // cppcheck-suppress noExplicitConstructor
    {}

    [[nodiscard]] auto data() const { return m_span.data(); }
    [[nodiscard]] auto size() const { return m_span.size(); }
    [[nodiscard]] auto empty() const { return m_span.empty(); }
    [[nodiscard]] auto begin() const { return m_span.begin(); }
    [[nodiscard]] auto end() const { return m_span.end(); }
    [[nodiscard]] auto rbegin() const { return m_span.rbegin(); }
    [[nodiscard]] auto rend() const { return m_span.rend(); }

private:
    span_type m_span;
};

inline auto ToBytes(BytesView view)
{
    return std::vector<byte>(view.begin(), view.end());
}

} // namespace Teide
