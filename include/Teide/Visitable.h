
#pragma once

#include <concepts>

namespace Teide
{

struct Visitor // for exposition only, not actually used
{
    template <typename... Args>
    void operator()(Args&&... args)
    {}
};

// clang-format off
template <typename T>
concept Visitable = requires(const T& obj, Visitor v)
{
    { obj.Visit(v) } -> std::same_as<void>;
};
// clang-format on

} // namespace Teide
