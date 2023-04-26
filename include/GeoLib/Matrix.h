
#pragma once

#include <GeoLib/Angle.h>
#include <GeoLib/ForwardDeclare.h>
#include <GeoLib/Vector.h>

namespace Geo
{
namespace Impl
{
    template <class T, Extent N>
    consteval Vector<T, N, VectorTag> InitRow(T x, T y, T z, T w)
    {
        if constexpr (N == 1)
        {
            return {x};
        }
        if constexpr (N == 2)
        {
            return {x, y};
        }
        if constexpr (N == 3)
        {
            return {x, y, z};
        }
        if constexpr (N == 4)
        {
            return {x, y, z, w};
        }
        return {};
    }

    template <class T, Extent N, Extent Row>
    consteval Vector<T, N, VectorTag> MatrixIdentityRow()
    {
        return InitRow<T, N>(Row == 0 ? T{1} : 0, Row == 1 ? T{1} : 0, Row == 2 ? T{1} : 0, Row == 3 ? T{1} : 0);
    }
} // namespace Impl

template <class T, Extent M, Extent N>
struct Matrix;

template <class T, Extent N>
struct Matrix<T, 1, N>
{
    Vector<T, N, VectorTag> x = Impl::MatrixIdentityRow<T, N, 0>();

    constexpr auto& operator[](Extent i) noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }
    constexpr const auto& operator[](Extent i) const noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }

    friend constexpr bool operator==(const Matrix& a, const Matrix& b) noexcept = default;

    static constexpr Matrix Zero() noexcept { return {{}}; }
    static constexpr Matrix Identity() noexcept { return Matrix{}; }

private:
    static constexpr decltype(&Matrix::x) members[] = {&Matrix::x};
};

template <class T, Extent N>
struct Matrix<T, 2, N>
{
    Vector<T, N, VectorTag> x = Impl::MatrixIdentityRow<T, N, 0>();
    Vector<T, N, VectorTag> y = Impl::MatrixIdentityRow<T, N, 1>();

    constexpr auto& operator[](Extent i) noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }
    constexpr const auto& operator[](Extent i) const noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }

    friend constexpr bool operator==(const Matrix& a, const Matrix& b) noexcept = default;

    static constexpr Matrix Zero() noexcept { return {{}, {}}; }
    static constexpr Matrix Identity() noexcept { return Matrix{}; }

private:
    static constexpr decltype(&Matrix::x) members[] = {&Matrix::x, &Matrix::y};
};

template <class T, Extent N>
struct Matrix<T, 3, N>
{
    Vector<T, N, VectorTag> x = Impl::MatrixIdentityRow<T, N, 0>();
    Vector<T, N, VectorTag> y = Impl::MatrixIdentityRow<T, N, 1>();
    Vector<T, N, VectorTag> z = Impl::MatrixIdentityRow<T, N, 2>();

    constexpr auto& operator[](Extent i) noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }
    constexpr const auto& operator[](Extent i) const noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }

    friend constexpr bool operator==(const Matrix& a, const Matrix& b) noexcept = default;

    static constexpr Matrix Zero() noexcept { return {{}, {}, {}}; }
    static constexpr Matrix Identity() noexcept { return Matrix{}; }

private:
    static constexpr decltype(&Matrix::x) members[] = {&Matrix::x, &Matrix::y, &Matrix::z};
};

template <class T, Extent N>
struct Matrix<T, 4, N>
{
    Vector<T, N, VectorTag> x = Impl::MatrixIdentityRow<T, N, 0>();
    Vector<T, N, VectorTag> y = Impl::MatrixIdentityRow<T, N, 1>();
    Vector<T, N, VectorTag> z = Impl::MatrixIdentityRow<T, N, 2>();
    Vector<T, N, VectorTag> w = Impl::MatrixIdentityRow<T, N, 3>();

    constexpr auto& operator[](Extent i) noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }
    constexpr const auto& operator[](Extent i) const noexcept
    {
        assert(i >= 0 && i < sizeof(members) / sizeof(members[0]));
        return this->*members[i];
    }

    friend constexpr bool operator==(const Matrix& a, const Matrix& b) noexcept = default;

    static constexpr Matrix Zero() noexcept { return {{}, {}, {}, {}}; }
    static constexpr Matrix Identity() noexcept { return Matrix{}; }

    static Matrix<T, 4, 4> RotationX(AngleT<T> angle) noexcept
        requires(N == 4)
    {
        return {
            {1, 0, 0, 0},
            {0, Cos(angle), -Sin(angle), 0},
            {0, Sin(angle), Cos(angle), 0},
            {0, 0, 0, 1},
        };
    }

    static Matrix<T, 4, 4> RotationY(AngleT<T> angle) noexcept
        requires(N == 4)
    {
        return {
            {Cos(angle), 0, Sin(angle), 0},
            {0, 1, 0, 0},
            {-Sin(angle), 0, Cos(angle), 0},
            {0, 0, 0, 1},
        };
    }

    static Matrix<T, 4, 4> RotationZ(AngleT<T> angle) noexcept
        requires(N == 4)
    {
        return {
            {Cos(angle), -Sin(angle), 0, 0},
            {Sin(angle), Cos(angle), 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        };
    }

private:
    static constexpr decltype(&Matrix::x) members[] = {&Matrix::x, &Matrix::y, &Matrix::z, &Matrix::w};
};

using Matrix2 = Matrix<float, 2, 2>;
using Matrix3 = Matrix<float, 3, 3>;
using Matrix4 = Matrix<float, 4, 4>;

template <class T, Extent N, Extent M, Extent O>
constexpr Matrix<T, M, O> operator*(const Matrix<T, M, N>& a, const Matrix<T, N, O>& b) noexcept
{
    auto ret = Matrix<T, M, O>::Zero();
    for (Extent row = 0; row < M; row++)
    {
        for (Extent col = 0; col < O; col++)
        {
            for (Extent i = 0; i < N; i++)
            {
                ret[row][col] += a[row][i] * b[i][col];
            }
        }
    }
    return ret;
}

template <class T, Extent M, class VectorTag>
constexpr Vector<T, M, VectorTag> operator*(const Matrix<T, M, M>& a, const Vector<T, M, VectorTag>& b) noexcept
{
    Vector<T, M, VectorTag> ret{};
    for (Extent row = 0; row < M; row++)
    {
        for (Extent col = 0; col < M; col++)
        {
            ret[row] += a[row][col] * b[col];
        }
    }
    return ret;
}

template <class T>
constexpr Vector<T, 3, VectorTag> operator*(const Matrix<T, 4, 4>& a, const Vector<T, 3, VectorTag>& b) noexcept
{
    const auto ret = a * Vector<T, 4, VectorTag>(b.x, b.y, b.z, T{0});
    return {ret.x, ret.y, ret.z};
}

template <class T>
constexpr Vector<T, 3, PointTag> operator*(const Matrix<T, 4, 4>& a, const Vector<T, 3, PointTag>& b) noexcept
{
    const auto ret = a * Vector<T, 4, VectorTag>(b.x, b.y, b.z, T{1});
    return {ret.x / ret.w, ret.y / ret.w, ret.z / ret.w};
}

template <class T, Extent M, Extent N>
constexpr Matrix<T, N, M> Transpose(const Matrix<T, M, N>& m) noexcept
{
    auto ret = Matrix<T, N, M>::Zero();
    for (Extent row = 0; row < N; row++)
    {
        for (Extent col = 0; col < M; col++)
        {
            ret[row][col] = m[col][row];
        }
    }
    return ret;
}

template <class T>
Matrix<T, 4, 4>
LookAt(const Vector<T, 3, PointTag>& eye, const Vector<T, 3, PointTag>& target, const Vector<T, 3, VectorTag>& up) noexcept
{
    const Vector3 forward = Normalise(target - eye);
    const Vector3 right = Normalise(Cross(up, forward));
    const Vector3 realUp = Cross(forward, right);

    const T eyeX = -Dot(right, eye);
    const T eyeY = -Dot(realUp, eye);
    const T eyeZ = -Dot(forward, eye);

    return {
        {right.x, right.y, right.z, eyeX},
        {realUp.x, realUp.y, realUp.z, eyeY},
        {forward.x, forward.y, forward.z, eyeZ},
        {0, 0, 0, 1},
    };
}

template <class T>
Matrix<T, 4, 4> Perspective(AngleT<T> fovy, T aspect, T near, T far) noexcept
{
    T const tanHalfFovy = Tan(fovy / static_cast<T>(2));

    return {
        {T{1} / (aspect * tanHalfFovy), 0, 0, 0},
        {0, T{-1} / (tanHalfFovy), 0, 0},
        {0, 0, (far + near) / (far - near), -(T{2} * far * near) / (far - near)},
        {0, 0, T{1}, 0},
    };
}

template <class T>
Matrix<T, 4, 4> Orthographic(T width, T height) noexcept
{
    return {
        {T{2} / width, 0, 0, T{-1}},
        {0, T{2} / height, 0, T{-1}},
        {0, 0, T{1}, 0},
        {0, 0, 0, T{1}},
    };
}

template <class T>
Matrix<T, 4, 4> Orthographic(T left, T right, T bottom, T top, T nclip, T fclip) noexcept
{
    return {
        {T{2} / (right - left), 0, 0, -(right + left) / (right - left)},
        {0, T{2} / (top - bottom), 0, -(top + bottom) / (top - bottom)},
        {0, 0, T{1} / (fclip - nclip), -(nclip) / (fclip - nclip)},
        {0, 0, 0, T{1}},
    };
}

} // namespace Geo
