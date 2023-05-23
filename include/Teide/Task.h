
#pragma once

#include <future>
#include <optional>

namespace Teide
{

template <class T = void>
using Task = std::shared_future<T>;

} // namespace Teide
