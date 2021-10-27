
#pragma once

#include <cmath>

namespace Geo
{

template <class T, int N, class Tag>
struct Vector;

template <class T, class Tag>
struct Vector<T, 2, Tag>
{
	T x, y;

	constexpr T& operator[](int i)
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		Unreachable();
	}
	constexpr T operator[](int i) const
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		Unreachable();
	}
};

template <class T, class Tag>
struct Vector<T, 3, Tag>
{
	T x, y, z;

	constexpr T& operator[](int i)
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else if (i == 2)
			return z;
		Unreachable();
	}
	constexpr T operator[](int i) const
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else if (i == 2)
			return z;
		Unreachable();
	}
};

template <class T, class Tag>
struct Vector<T, 4, Tag>
{
	T x, y, z, w;

	constexpr T& operator[](int i)
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else if (i == 2)
			return z;
		else if (i == 3)
			return w;
		Unreachable();
	}
	constexpr T operator[](int i) const
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else if (i == 2)
			return z;
		else if (i == 3)
			return w;
		Unreachable();
	}
};

struct VectorTag;
using Vector2 = Vector<float, 2, VectorTag>;
using Vector3 = Vector<float, 3, VectorTag>;
using Vector4 = Vector<float, 4, VectorTag>;

struct PointTag;
using Point2 = Vector<float, 2, PointTag>;
using Point3 = Vector<float, 3, PointTag>;
using Point4 = Vector<float, 4, PointTag>;

// Arithmetic operators
namespace Impl
{
	template <class RetTag, class T, int N, class Tag1, class Tag2>
	constexpr Vector<T, N, RetTag> Add(const Vector<T, N, Tag1>& a, const Vector<T, N, Tag2>& b) noexcept
	{
		Vector<T, N, RetTag> ret{};
		for (int i = 0; i < N; i++)
		{
			ret[i] = a[i] + b[i];
		}
		return ret;
	}

	template <class RetTag, class T, int N, class Tag1, class Tag2>
	constexpr Vector<T, N, RetTag> Subtract(const Vector<T, N, Tag1>& a, const Vector<T, N, Tag2>& b) noexcept
	{
		Vector<T, N, RetTag> ret{};
		for (int i = 0; i < N; i++)
		{
			ret[i] = a[i] - b[i];
		}
		return ret;
	}
} // namespace Impl

// Vector + Vector = Vector
template <class T, int N>
constexpr Vector<T, N, VectorTag> operator+(const Vector<T, N, VectorTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
	return Impl::Add<VectorTag>(a, b);
}

// Point + Vector = Point
template <class T, int N>
constexpr Vector<T, N, PointTag> operator+(const Vector<T, N, PointTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
	return Impl::Add<PointTag>(a, b);
}

// Vector + Point = Point
template <class T, int N>
constexpr Vector<T, N, PointTag> operator+(const Vector<T, N, VectorTag>& a, const Vector<T, N, PointTag>& b) noexcept
{
	return Impl::Add<PointTag>(a, b);
}

// Vector - Vector = Vector
template <class T, int N>
constexpr Vector<T, N, VectorTag> operator-(const Vector<T, N, VectorTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
	return Impl::Subtract<VectorTag>(a, b);
}

// Point - Vector = Point
template <class T, int N>
constexpr Vector<T, N, PointTag> operator-(const Vector<T, N, PointTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
	return Impl::Subtract<PointTag>(a, b);
}

// Point - Point = Vector
template <class T, int N>
constexpr Vector<T, N, VectorTag> operator-(const Vector<T, N, PointTag>& a, const Vector<T, N, PointTag>& b) noexcept
{
	return Impl::Subtract<VectorTag>(a, b);
}

template <class T, int N, class Tag>
constexpr Vector<T, N, Tag> operator-(const Vector<T, N, Tag>& a) noexcept
{
	Vector<T, N, Tag> ret{};
	for (int i = 0; i < N; i++)
	{
		ret[i] = -a[i];
	}
	return ret;
}

template <class T, int N, class Tag>
constexpr Vector<T, N, Tag> operator*(const Vector<T, N, Tag>& a, T b) noexcept
{
	Vector<T, N, Tag> ret{};
	for (int i = 0; i < N; i++)
	{
		ret[i] = a[i] * b;
	}
	return ret;
}

template <class T, int N, class Tag>
constexpr Vector<T, N, Tag> operator/(const Vector<T, N, Tag>& a, T b) noexcept
{
	Vector<T, N, Tag> ret{};
	for (int i = 0; i < N; i++)
	{
		ret[i] = a[i] / b;
	}
	return ret;
}

// Vector operations

template <class T, int N, class Tag>
inline T Magnitude(const Vector<T, N, Tag>& a) noexcept
{
	T ret{};
	for (int i = 0; i < N; i++)
	{
		ret += a[i] * a[i];
	}
	return std::sqrt(ret);
}

template <class T, int N, class Tag>
inline Vector<T, N, Tag> Normalise(const Vector<T, N, Tag>& a) noexcept
{
	return a / Magnitude(a);
}

template <class T, int N, class Tag1, class Tag2>
constexpr T Dot(const Vector<T, N, Tag1>& a, const Vector<T, N, Tag2>& b) noexcept
{
	T ret{};
	for (int i = 0; i < N; i++)
	{
		ret += a[i] * b[i];
	}
	return ret;
}

template <class T, class Tag>
constexpr Vector<T, 3, Tag> Cross(const Vector<T, 3, Tag>& a, const Vector<T, 3, Tag>& b) noexcept
{
	return {
	    a.y * b.z - a.z * b.y,
	    a.z * b.x - a.x * b.z,
	    a.x * b.y - a.y * b.x,
	};
}

} // namespace Geo
