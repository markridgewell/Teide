
#pragma once

#include <cmath>
#include <numbers>

namespace Geo
{
template <class T>
constexpr T PiT = std::numbers::pi_v<T>;
constexpr float Pi = PiT<float>;

template <class T>
class AngleT
{
public:
	AngleT() = default;

	static constexpr AngleT Radians(T radians) noexcept { return AngleT(radians); }

	static constexpr AngleT Degrees(T degrees) noexcept
	{
		return AngleT(static_cast<T>(degrees / 180.0 * PiT<long double>));
	}

	constexpr T AsRadians() const noexcept { return m_radians; }
	constexpr T AsDegrees() const noexcept { return m_radians / PiT<T> * 180.0f; }

	friend constexpr AngleT operator*(AngleT a, T b) noexcept { return {a.m_radians * b}; }
	friend constexpr AngleT operator/(AngleT a, T b) noexcept { return {a.m_radians / b}; }
	friend constexpr AngleT operator*(T a, AngleT b) noexcept { return {a * b.m_radians}; }
	friend constexpr AngleT operator+(AngleT a, AngleT b) noexcept { return {a.m_radians + b.m_radians}; }
	friend constexpr AngleT operator-(AngleT a, AngleT b) noexcept { return {a.m_radians - b.m_radians}; }
	friend constexpr AngleT operator-(AngleT a) noexcept { return {-a.m_radians}; }

	friend constexpr AngleT& operator+=(AngleT& a, AngleT b) noexcept
	{
		a.m_radians += b.m_radians;
		return a;
	}
	friend constexpr AngleT& operator-=(AngleT& a, AngleT b) noexcept
	{
		a.m_radians -= b.m_radians;
		return a;
	}
	friend constexpr AngleT& operator*=(AngleT& a, T b) noexcept
	{
		a.m_radians *= b;
		return a;
	}
	friend constexpr AngleT& operator/=(AngleT& a, T b) noexcept
	{
		a.m_radians /= b;
		return a;
	}

	auto friend operator<=>(AngleT a, AngleT b) = default;

private:
	constexpr AngleT(T r) noexcept : m_radians{r} {}

	T m_radians{};
};

using Angle = AngleT<float>;
using Angled = AngleT<double>;

template <class T>
inline T Sin(AngleT<T> a) noexcept
{
	return std::sin(a.AsRadians());
}

template <class T>
inline T Cos(AngleT<T> a) noexcept
{
	return std::cos(a.AsRadians());
}

template <class T>
inline T Tan(AngleT<T> a) noexcept
{
	return std::tan(a.AsRadians());
}

inline namespace Literals
{
	consteval Angled operator"" _radd(long double x) { return Angled::Radians(static_cast<double>(x)); }
	consteval Angled operator"" _degd(long double x) { return Angled::Degrees(static_cast<double>(x)); }

	consteval Angle operator"" _rad(long double x) { return Angle::Radians(static_cast<float>(x)); }
	consteval Angle operator"" _deg(long double x) { return Angle::Degrees(static_cast<float>(x)); }

} // namespace Literals

} // namespace Geo
