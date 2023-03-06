
#pragma once

#include <GeoLib/ForwardDeclare.h>
#include <GeoLib/Scalar.h>

#include <cassert>
#include <cmath>
#include <ostream>

namespace Geo
{
template <class T, Extent N, class Tag>
struct Vector;

template <class T, class Tag>
struct Vector<T, 1, Tag>
{
    T x{};

    constexpr Vector() = default;
    constexpr Vector(T x) noexcept : x{x} {}

    template <class OtherTag>
    constexpr explicit Vector(const Vector<T, 1, OtherTag>& v) : x{v.x}
    {}

    constexpr T& operator[](Extent i) noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }
    constexpr T operator[](Extent i) const noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }

    friend auto operator<=>(const Vector& a, const Vector& b) noexcept = default;

    static constexpr Vector Zero() noexcept { return {}; }
    static constexpr Vector UnitX() noexcept { return {1.0f}; }

private:
    static constexpr decltype(&Vector::x) members[] = {&Vector::x};
};

template <class T, class Tag>
struct Vector<T, 2, Tag>
{
    T x{}, y{};

    constexpr Vector() = default;
    explicit constexpr Vector(T v) noexcept : x{v}, y{v} {}
    constexpr Vector(T x, T y) noexcept : x{x}, y{y} {}

    template <class OtherTag>
    constexpr explicit Vector(const Vector<T, 2, OtherTag>& v) : x{v.x}, y{v.y}
    {}

    constexpr T& operator[](Extent i) noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }
    constexpr T operator[](Extent i) const noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }

    friend auto operator<=>(const Vector& a, const Vector& b) noexcept = default;

    static constexpr Vector Zero() noexcept { return {}; }
    static constexpr Vector UnitX() noexcept { return {1.0f, 0.0f}; }
    static constexpr Vector UnitY() noexcept { return {0.0f, 1.0f}; }

private:
    static constexpr decltype(&Vector::x) members[] = {&Vector::x, &Vector::y};
};

template <class T, class Tag>
struct Vector<T, 3, Tag>
{
    T x{}, y{}, z{};

    constexpr Vector() = default;
    explicit constexpr Vector(T v) noexcept : x{v}, y{v}, z{v} {}
    constexpr Vector(T x, T y, T z) noexcept : x{x}, y{y}, z{z} {}

    template <class OtherTag>
    constexpr explicit Vector(const Vector<T, 3, OtherTag>& v) : x{v.x}, y{v.y}, z{v.z}
    {}

    constexpr T& operator[](Extent i) noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }
    constexpr T operator[](Extent i) const noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }

    friend auto operator<=>(const Vector& a, const Vector& b) = default;

    static constexpr Vector Zero() noexcept { return {}; }
    static constexpr Vector UnitX() noexcept { return {1.0f, 0.0f, 0.0f}; }
    static constexpr Vector UnitY() noexcept { return {0.0f, 1.0f, 0.0f}; }
    static constexpr Vector UnitZ() noexcept { return {0.0f, 0.0f, 1.0f}; }

private:
    static constexpr decltype(&Vector::x) members[] = {&Vector::x, &Vector::y, &Vector::z};
};

template <class T, class Tag>
struct Vector<T, 4, Tag>
{
    T x{}, y{}, z{}, w{};

    constexpr Vector() = default;
    explicit constexpr Vector(T v) noexcept : x{v}, y{v}, z{v}, w{v} {}
    constexpr Vector(T x, T y, T z, T w) noexcept : x{x}, y{y}, z{z}, w{w} {}

    template <class OtherTag>
    constexpr explicit Vector(const Vector<T, 4, OtherTag>& v) : x{v.x}, y{v.y}, z{v.z}, w{v.w}
    {}

    constexpr T& operator[](Extent i) noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }
    constexpr T operator[](Extent i) const noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }

    friend auto operator<=>(const Vector& a, const Vector& b) = default;

    static constexpr Vector Zero() noexcept { return {}; }
    static constexpr Vector UnitX() noexcept { return {1.0f, 0.0f, 0.0f, 0.0f}; }
    static constexpr Vector UnitY() noexcept { return {0.0f, 1.0f, 0.0f, 0.0f}; }
    static constexpr Vector UnitZ() noexcept { return {0.0f, 0.0f, 1.0f, 0.0f}; }
    static constexpr Vector UnitW() noexcept { return {0.0f, 0.0f, 0.0f, 1.0f}; }

private:
    static constexpr decltype(&Vector::x) members[] = {&Vector::x, &Vector::y, &Vector::z, &Vector::w};
};

template <class T, Extent N, class Tag>
struct Traits<Vector<T, N, Tag>>
{
    using ScalarType = T;
    using TagType = Tag;
    static constexpr Extent VectorSize = N;
};

// Arithmetic operators
namespace Impl
{
    template <class RetTag, class T, Extent N, class Tag1, class Tag2>
    constexpr Vector<T, N, RetTag> Add(const Vector<T, N, Tag1>& a, const Vector<T, N, Tag2>& b) noexcept
    {
        Vector<T, N, RetTag> ret{};
        for (Extent i = 0; i < N; i++)
        {
            ret[i] = a[i] + b[i];
        }
        return ret;
    }

    template <class RetTag, class T, Extent N, class Tag1, class Tag2>
    constexpr Vector<T, N, RetTag> Subtract(const Vector<T, N, Tag1>& a, const Vector<T, N, Tag2>& b) noexcept
    {
        Vector<T, N, RetTag> ret{};
        for (Extent i = 0; i < N; i++)
        {
            ret[i] = a[i] - b[i];
        }
        return ret;
    }

    template <class RetTag, class T, Extent N, class Tag1, class Tag2>
    constexpr Vector<T, N, RetTag> Multiply(const Vector<T, N, Tag1>& a, const Vector<T, N, Tag2>& b) noexcept
    {
        Vector<T, N, RetTag> ret{};
        for (Extent i = 0; i < N; i++)
        {
            ret[i] = a[i] * b[i];
        }
        return ret;
    }

    template <class RetTag, class T, Extent N, class Tag1, class Tag2>
    constexpr Vector<T, N, RetTag> Divide(const Vector<T, N, Tag1>& a, const Vector<T, N, Tag2>& b) noexcept
    {
        Vector<T, N, RetTag> ret{};
        for (Extent i = 0; i < N; i++)
        {
            ret[i] = a[i] / b[i];
        }
        return ret;
    }
} // namespace Impl

// Vector + Vector = Vector
template <class T, Extent N>
constexpr Vector<T, N, VectorTag> operator+(const Vector<T, N, VectorTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return Impl::Add<VectorTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, VectorTag>& operator+=(Vector<T, N, VectorTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return a = Impl::Add<VectorTag>(a, b);
}

// Point + Vector = Point
template <class T, Extent N>
constexpr Vector<T, N, PointTag> operator+(const Vector<T, N, PointTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return Impl::Add<PointTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, PointTag>& operator+=(Vector<T, N, PointTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return a = Impl::Add<PointTag>(a, b);
}

// Vector + Point = Point
template <class T, Extent N>
constexpr Vector<T, N, PointTag> operator+(const Vector<T, N, VectorTag>& a, const Vector<T, N, PointTag>& b) noexcept
{
    return Impl::Add<PointTag>(a, b);
}

// Vector - Vector = Vector
template <class T, Extent N>
constexpr Vector<T, N, VectorTag> operator-(const Vector<T, N, VectorTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return Impl::Subtract<VectorTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, VectorTag>& operator-=(Vector<T, N, VectorTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return a = Impl::Subtract<VectorTag>(a, b);
}

// Point - Vector = Point
template <class T, Extent N>
constexpr Vector<T, N, PointTag> operator-(const Vector<T, N, PointTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return Impl::Subtract<PointTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, PointTag>& operator-=(Vector<T, N, PointTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return a = Impl::Subtract<PointTag>(a, b);
}

// Point - Point = Vector
template <class T, Extent N>
constexpr Vector<T, N, VectorTag> operator-(const Vector<T, N, PointTag>& a, const Vector<T, N, PointTag>& b) noexcept
{
    return Impl::Subtract<VectorTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, PointTag>& operator-=(Vector<T, N, PointTag>& a, const Vector<T, N, PointTag>& b) noexcept
{
    return a = Impl::Subtract<PointTag>(a, b);
}

// Vector * Scale = Vector
template <class T, Extent N>
constexpr Vector<T, N, VectorTag> operator*(const Vector<T, N, VectorTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return Impl::Multiply<VectorTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, VectorTag>& operator*=(Vector<T, N, VectorTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return a = Impl::Multiply<VectorTag>(a, b);
}

// Scale * Vector = Vector
template <class T, Extent N>
constexpr Vector<T, N, VectorTag> operator*(const Vector<T, N, ScaleTag>& a, const Vector<T, N, VectorTag>& b) noexcept
{
    return Impl::Multiply<VectorTag>(a, b);
}

// Point * Scale = Point
template <class T, Extent N>
constexpr Vector<T, N, PointTag> operator*(const Vector<T, N, PointTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return Impl::Multiply<PointTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, PointTag>& operator*=(Vector<T, N, PointTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return a = Impl::Multiply<PointTag>(a, b);
}

// Scale * Point = Point
template <class T, Extent N>
constexpr Vector<T, N, PointTag> operator*(const Vector<T, N, ScaleTag>& a, const Vector<T, N, PointTag>& b) noexcept
{
    return Impl::Multiply<PointTag>(a, b);
}

// Scale * Scale = Scale
template <class T, Extent N>
constexpr Vector<T, N, ScaleTag> operator*(const Vector<T, N, ScaleTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return Impl::Multiply<ScaleTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, ScaleTag>& operator*=(Vector<T, N, ScaleTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return a = Impl::Multiply<ScaleTag>(a, b);
}

// Vector / Scale = Vector
template <class T, Extent N>
constexpr Vector<T, N, VectorTag> operator/(const Vector<T, N, VectorTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return Impl::Divide<VectorTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, VectorTag>& operator/=(Vector<T, N, VectorTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return a = Impl::Divide<VectorTag>(a, b);
}

// Point / Scale = Point
template <class T, Extent N>
constexpr Vector<T, N, PointTag> operator/(const Vector<T, N, PointTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return Impl::Divide<PointTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, PointTag>& operator/=(Vector<T, N, PointTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return a = Impl::Divide<PointTag>(a, b);
}

// Scale / Scale = Scale
template <class T, Extent N>
constexpr Vector<T, N, ScaleTag> operator/(const Vector<T, N, ScaleTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return Impl::Divide<ScaleTag>(a, b);
}
template <class T, Extent N>
constexpr Vector<T, N, ScaleTag>& operator/=(Vector<T, N, ScaleTag>& a, const Vector<T, N, ScaleTag>& b) noexcept
{
    return a = Impl::Divide<ScaleTag>(a, b);
}

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag> operator-(const Vector<T, N, Tag>& a) noexcept
{
    Vector<T, N, Tag> ret{};
    for (Extent i = 0; i < N; i++)
    {
        ret[i] = -a[i];
    }
    return ret;
}

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag> operator*(const Vector<T, N, Tag>& a, T b) noexcept
{
    Vector<T, N, Tag> ret{};
    for (Extent i = 0; i < N; i++)
    {
        ret[i] = a[i] * b;
    }
    return ret;
}

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag> operator*(T a, const Vector<T, N, Tag>& b) noexcept
{
    Vector<T, N, Tag> ret{};
    for (Extent i = 0; i < N; i++)
    {
        ret[i] = a * b[i];
    }
    return ret;
}

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag>& operator*=(Vector<T, N, Tag>& a, T b) noexcept
{
    return a = a * b;
}

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag> operator/(const Vector<T, N, Tag>& a, T b) noexcept
{
    Vector<T, N, Tag> ret{};
    for (Extent i = 0; i < N; i++)
    {
        ret[i] = a[i] / b;
    }
    return ret;
}

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag> operator/(T a, const Vector<T, N, Tag>& b) noexcept
{
    Vector<T, N, Tag> ret{};
    for (Extent i = 0; i < N; i++)
    {
        ret[i] = a / b[i];
    }
    return ret;
}

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag>& operator/=(Vector<T, N, Tag>& a, T b) noexcept
{
    return a = a / b;
}

// Vector operations

template <class T, Extent N, class Tag>
inline T MagnitudeSq(const Vector<T, N, Tag>& a) noexcept
{
    T ret{};
    for (Extent i = 0; i < N; i++)
    {
        ret += a[i] * a[i];
    }
    return ret;
}

template <class T, Extent N, class Tag>
inline T Magnitude(const Vector<T, N, Tag>& a) noexcept
{
    return std::sqrt(MagnitudeSq(a));
}

template <class T, Extent N, class Tag>
inline Vector<T, N, Tag> Normalise(const Vector<T, N, Tag>& a) noexcept
{
    return a / Magnitude(a);
}

template <class T, Extent N, class Tag1, class Tag2>
constexpr T Dot(const Vector<T, N, Tag1>& a, const Vector<T, N, Tag2>& b) noexcept
{
    T ret{};
    for (Extent i = 0; i < N; i++)
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

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag> Lerp(const Vector<T, N, Tag>& a, const Vector<T, N, Tag>& b, T t) noexcept
{
    Vector<T, N, Tag> ret{};
    for (Extent i = 0; i < N; i++)
    {
        ret[i] = Lerp(a[i], b[i], t);
    }
    return ret;
}

template <class T, Extent N, class Tag>
constexpr Vector<T, N, Tag> Midpoint(const Vector<T, N, Tag>& a, const Vector<T, N, Tag>& b) noexcept
{
    Vector<T, N, Tag> ret{};
    for (Extent i = 0; i < N; i++)
    {
        ret[i] = Midpoint(a[i], b[i]);
    }
    return ret;
}


template <class T, Extent N, class Tag>
bool Compare(const Geo::Vector<T, N, Tag>& a, const Geo::Vector<T, N, Tag>& b, T maxRelDiff)
{
    for (Extent i = 0; i < N; i++)
    {
        if (!Compare(a[i], b[i], maxRelDiff))
        {
            return false;
        }
    }
    return true;
}

template <class T, Extent N, class Tag>
std::ostream& operator<<(std::ostream& os, const Geo::Vector<T, N, Tag>& v)
{
    os << v[0];
    for (Extent i = 1; i < N; i++)
    {
        os << ", " << v[i];
    }
    return os;
}

} // namespace Geo
