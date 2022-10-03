
#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename K, typename V, std::size_t N>
class StaticMap
{
public:
	constexpr StaticMap(std::array<std::pair<K, V>, N> args) : m_data{args} {}

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
