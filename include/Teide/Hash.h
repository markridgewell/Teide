
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Visitable.h"

#include <cstddef>
#include <functional>
#include <ranges>
#include <utility>

namespace Teide
{
template <typename T>
struct HashAppender
{
    constexpr void operator()(usize& seed, const T& value) noexcept
    {
        // hash_combine function from P0814R0
        seed ^= std::hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};

template <typename T>
constexpr void HashAppend(usize& seed, const T& value) noexcept
{
    HashAppender<T>{}(seed, value);
}

constexpr void HashAppend(usize& seed, std::byte b) noexcept
{
    seed = ((seed << 5) + seed) + static_cast<usize>(b);
}

template <typename T>
    requires std::integral<T>
struct HashAppender<T>
{
    constexpr void operator()(usize& seed, T value) noexcept
    {
        for (std::size_t i = 0; i < sizeof(T); i++)
        {
            const auto b = value >> (i * 8);
            HashAppend(seed, static_cast<std::byte>(b & 0xFF));
        }
    }
};

template <typename T>
    requires std::is_enum_v<T>
struct HashAppender<T>
{
    constexpr void operator()(usize& seed, T value) noexcept { HashAppend(seed, std::to_underlying<T>(value)); }
};

template <typename T>
    requires Visitable<T>
struct HashAppender<T>
{
    constexpr void operator()(usize& seed, const T& value) const
    {
        value.Visit([&seed](const auto&... args) { (HashAppend(seed, args), ...); });
    }
};

template <typename T>
    requires std::ranges::range<T>
struct HashAppender<T>
{
    constexpr void operator()(usize& seed, const T& range) const
    {
        for (const auto& elem : range)
        {
            HashAppend(seed, elem);
        }
    }
};

constexpr usize HashInitialSeed = 5381;

template <typename T>
struct Hash
{
    constexpr usize operator()(const T& value) const
    {
        usize seed = HashInitialSeed;
        HashAppend(seed, value);
        return seed;
    }
};

} // namespace Teide
