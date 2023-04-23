
#pragma once

#include "Teide/BasicTypes.h"

#include <cstddef>
#include <functional>
#include <ranges>

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

template <typename T>
struct Hash : std::hash<T>
{
    using Base = std::hash<T>;
    usize operator()(const T& val, usize seed = 0)
    {
        return seed ^ Base::operator()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};

// hash_combine function from P0814R0
template <typename T>
void HashCombineSingle(usize& seed, const T& val)
{
    seed ^= Hash<T>()(val, seed) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct HashCombiner
{
    usize value;

    template <typename... Types>
    void operator()(const Types&... args)
    {
        (HashCombineSingle(value, args), ...);
    }
};

template <typename T>
    requires Visitable<T>
struct Hash<T>
{
    usize operator()(const T& val, usize seed = 0) const
    {
        HashCombiner h{seed};
        val.Visit(h);
        return h.value;
    }
};

template <typename T>
    requires std::ranges::range<T>
struct Hash<T>
{
    usize operator()(const T& val, usize seed = 0) const
    {
        Hash<std::ranges::range_value_t<T>> hash;
        for (auto&& elem : val)
        {
            seed = hash(elem, seed);
        }
        return seed;
    }
};
} // namespace Teide
