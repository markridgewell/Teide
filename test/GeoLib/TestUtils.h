
#pragma once

#include "GeoLib/Vector.h"

#include <gmock/gmock.h>

#include <concepts>
#include <format>
#include <ostream>

namespace Geo
{
template <class T, int N, class Tag>
std::ostream& operator<<(std::ostream& os, const Vector<T, N, Tag>& v)
{
	os << '(';
	for (int i = 0; i < N - 1; i++)
	{
		os << v[i] << ", ";
	}
	return os << v[N - 1] << ')';
}
} // namespace Geo

template <class T>
constexpr T Epsilon = std::numeric_limits<T>::epsilon();

template <class T>
	requires std::floating_point<T>
bool ApproxEqual(T a, T b, T maxRelDiff = Epsilon<T>)
{
	return std::abs(a - b) <= maxRelDiff;
}

template <class T, int N, class Tag>
bool ApproxEqual(const Geo::Vector<T, N, Tag>& a, const Geo::Vector<T, N, Tag>& b, T maxRelDiff = Epsilon<T>)
{
	for (int i = 0; i < N; i++)
	{
		if (!ApproxEqual(a[i], b[i], maxRelDiff))
		{
			return false;
		}
	}
	return true;
}

MATCHER_P(ApproxEq, v, std::format("{} [approximately]", testing::PrintToString(v)))
{
	(void)result_listener;
	return ApproxEqual(arg, v);
}