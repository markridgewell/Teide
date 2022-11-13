
#include "Teide/BasicTypes.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

namespace Teide
{

template <typename K, typename V, usize N>
class StaticMap
{
public:
    constexpr StaticMap(std::initializer_list<std::pair<K, V>> args)
    {
        if (args.size() != N)
        {
            throw std::invalid_argument("args must have exactly N elements");
        }
        usize i = 0;
        for (const auto& elem : args)
        {
            m_data[i] = elem;
            ++i;
        }
    }

    [[nodiscard]] constexpr V at(const K& key) const
    {
        const auto it = std::find_if(m_data.begin(), m_data.end(), [&key](const auto& v) { return v.first == key; });
        if (it == m_data.end())
        {
            throw std::range_error("Key not found");
        }
        return it->second;
    }

    [[nodiscard]] constexpr K inverse_at(const V& value) const
    {
        const auto it = std::find_if(m_data.begin(), m_data.end(), [&value](const auto& v) { return v.second == value; });
        if (it == m_data.end())
        {
            throw std::range_error("Value not found");
        }
        return it->first;
    }

private:
    std::array<std::pair<K, V>, N> m_data;
};

} // namespace Teide
