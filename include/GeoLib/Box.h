#pragma once

#include "ForwardDeclare.h"
#include "Vector.h"

#include <concepts>
#include <type_traits>

namespace Geo
{
template <class T, Extent N>
struct Box
{
	using Point = Vector<T, N, PointTag>;

	Point min;
	Point max;

	constexpr bool Contains(Point point) const noexcept
	{
		return point.x >= min.x && point.y >= min.y && point.x <= max.x && point.y <= max.y;
	}

	constexpr bool Contains(const Box& box) const noexcept
	{
		return box.min.x >= min.x && box.min.y >= min.y && box.max.x <= max.x && box.max.y <= max.y;
	}
};

template <class T>
using SizeType = std::conditional_t<std::integral<T>, std::make_unsigned_t<T>, T>;

template <class T, Extent N>
constexpr Vector<SizeType<T>, N, SizeTag> GetSize(const Box<T, N>& box)
{
	Vector<SizeType<T>, N, SizeTag> ret;
	for (Extent i = 0; i < N; i++)
	{
		ret[i] = static_cast<SizeType<T>>(box.max[i] - box.min[i]);
	}
	return ret;
}

template <class T, Extent N>
constexpr Vector<T, N, PointTag> GetCentre(const Box<T, N>& box)
{
	return Midpoint(box.min, box.max);
}

} // namespace Geo
