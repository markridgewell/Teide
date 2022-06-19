
#pragma once

#include <span>
#include <type_traits>

template <class T>
concept Span = requires(T t)
{
	{std::span{t}};
};

template <class T>
concept TrivialSpan
    = Span<T> && std::is_trivially_copyable_v<typename T::value_type> && std::is_standard_layout_v<typename T::value_type>;

template <class T>
concept TrivialObject
    = !Span<T> && std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> && !std::is_pointer_v<T>;

class BytesView
{
	using span_type = std::span<const std::byte>;

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

	BytesView(const BytesView&) = default;
	BytesView(BytesView&&) = default;
	BytesView& operator=(const BytesView&) = default;
	BytesView& operator=(BytesView&&) = default;

	BytesView(std::span<const std::byte> bytes = {}) : m_span{bytes} {}

	template <TrivialSpan T>
	BytesView(const T& data) : m_span{std::as_bytes(std::span(data))}
	{}

	template <TrivialObject T>
	BytesView(const T& data) : m_span{std::as_bytes(std::span(&data, 1))}
	{}

	auto data() const { return m_span.data(); }
	auto size() const { return m_span.size(); }
	auto empty() const { return m_span.empty(); }
	auto begin() const { return m_span.begin(); }
	auto end() const { return m_span.end(); }
	auto rbegin() const { return m_span.rbegin(); }
	auto rend() const { return m_span.rend(); }

private:
	span_type m_span;
};
