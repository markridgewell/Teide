
#pragma once

#include <future>
#include <optional>

namespace Teide
{

namespace detail
{
	template <class T>
	struct TaskHelper
	{
		using type = std::shared_future<std::optional<T>>;
	};

	template <>
	struct TaskHelper<void>
	{
		using type = std::shared_future<void>;
	};
} // namespace detail

template <class T = void>
using Task = detail::TaskHelper<T>::type;

} // namespace Teide
