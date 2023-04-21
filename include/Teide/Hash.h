
#pragma once

#include <cstddef>
#include <functional>
#include <ranges>

template <typename T>
concept hashable = requires(const T& obj)
{
    { obj.hash() } -> std::same_as<std::size_t>;
};

template <typename T>
struct hash : std::hash<T> {};

template <typename T>
    requires hashable<T>
struct hash<T>
{
    std::size_t operator()(const T& val) const
    {
        return val.hash();
    }
};

// hash_combine function from P0814R0
namespace detail
{
template <typename T>
void hash_combine(std::size_t& seed, const T& val)
{
    seed ^= hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace detail

template <typename... Types>
std::size_t hash_combine(const Types&... args)
{
    std::size_t seed = 0;
    (detail::hash_combine(seed, args), ...); // create hash value with seed over all args
    return seed;
}

template <typename T>
    requires std::ranges::range<T>
struct hash<T>
{
    std::size_t operator()(const T& val) const
    {
        std::size_t seed = 0;
        for (auto&& elem : val)
        {
            detail::hash_combine(seed, elem);
        }
        return seed;
    }
};

