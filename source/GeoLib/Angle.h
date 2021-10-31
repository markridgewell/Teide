
#pragma once

#include <cmath>
#include <numbers>

namespace Geo
{
template <class T>
class Angle
{
public:
	static constexpr Angle Radians(T radians) { return Angle(radians); }
	static constexpr Angle Degrees(T degrees)
	{
		return Angle(static_cast<T>(degrees * std::numbers::pi_v<long double> / 180.0));
	}

	constexpr T AsRadians() const { return m_radians; }
	constexpr T AsDegrees() const { return m_radians / std::numbers::pi_v<T> * 180.0f; }

	friend constexpr Angle operator*(Angle a, T b) { return Angle(a.m_radians * b); }
	friend constexpr Angle operator/(Angle a, T b) { return Angle(a.m_radians / b); }
	friend constexpr Angle operator*(T a, Angle b) { return Angle(a * b.m_radians); }

private:
	constexpr Angle(T r) : m_radians{r} {}

	T m_radians;
};

using Anglef = Angle<float>;
using Angled = Angle<double>;

template <class T>
inline T Sin(Angle<T> a) noexcept
{
	return std::sin(a.AsRadians());
}

template <class T>
inline T Cos(Angle<T> a) noexcept
{
	return std::cos(a.AsRadians());
}

template <class T>
inline T Tan(Angle<T> a) noexcept
{
	return std::tan(a.AsRadians());
}

inline namespace Literals
{
	consteval Angled operator"" _rad(long double x) { return Angled::Radians(static_cast<double>(x)); }
	consteval Angled operator"" _deg(long double x) { return Angled::Degrees(static_cast<double>(x)); }

	consteval Anglef operator"" _radf(long double x) { return Anglef::Radians(static_cast<float>(x)); }
	consteval Anglef operator"" _degf(long double x) { return Anglef::Degrees(static_cast<float>(x)); }

} // namespace Literals

} // namespace Geo
