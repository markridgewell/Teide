
#pragma once

#include <numbers>

namespace Geo
{
using Angle = float;

inline float Sin(Angle a) noexcept
{
	return static_cast<float>(sin(a));
}
inline float Cos(Angle a) noexcept
{
	return static_cast<float>(cos(a));
}
inline float Tan(Angle a) noexcept
{
	return static_cast<float>(tan(a));
}

inline namespace Literals
{
	constexpr Angle operator"" _deg(long double x)
	{
		return Angle(static_cast<float>(x * std::numbers::pi_v<long double> / 180.0));
	}
	constexpr Angle operator"" _rad(long double x) { return Angle(static_cast<float>(x)); }

} // namespace Literals

} // namespace Geo
