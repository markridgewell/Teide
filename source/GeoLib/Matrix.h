
#pragma once

#include "Angle.h"
#include "Vector.h"

namespace Geo
{
template <class T, int M, int N>
struct Matrix;

template <class T, int M>
struct Matrix<T, M, 2>
{
	Vector<T, M, VectorTag> x, y;

	constexpr auto& operator[](int i) { return this->*Members()[i]; }
	constexpr auto operator[](int i) const { return this->*Members()[i]; }

	static Matrix<T, M, 2> Identity()
	{
		Matrix<T, M, 2> ret;
		ret.x[0] = T{1};
		ret.y[1] = T{1};
		return ret;
	}

private:
	static constexpr auto& Members()
	{
		static constexpr auto members = std::array{&Matrix::x, &Matrix::y};
		return members;
	}
};

template <class T, int M>
struct Matrix<T, M, 3>
{
	Vector<T, M, VectorTag> x, y, z;

	constexpr auto& operator[](int i) { return this->*Members()[i]; }
	constexpr auto operator[](int i) const { return this->*Members()[i]; }

	static Matrix<T, M, 3> Identity()
	{
		Matrix<T, M, 3> ret;
		ret.x[0] = T{1};
		ret.y[1] = T{1};
		if constexpr (M >= 3)
		{
			ret.z[2] = T{1};
		}
		return ret;
	}

private:
	static constexpr auto& Members()
	{
		static constexpr auto members = std::array{&Matrix::x, &Matrix::y, &Matrix::z};
		return members;
	}
};

template <class T, int M>
struct Matrix<T, M, 4>
{
	Vector<T, M, VectorTag> x, y, z, w;

	constexpr auto& operator[](int i) { return this->*Members()[i]; }
	constexpr auto operator[](int i) const { return this->*Members()[i]; }

	static Matrix<T, M, 4> Identity()
	{
		Matrix<T, M, 4> ret;
		ret.x[0] = T{1};
		ret.y[1] = T{1};
		if constexpr (M >= 3)
		{
			ret.z[2] = T{1};
		}
		if constexpr (M >= 4)
		{
			ret.w[3] = T{1};
		}
		return ret;
	}

	static Matrix<T, 4, 4> RotationX(AngleT<T> angle) noexcept
	{
		static_assert(M == 4, "RotationX only works with 4x4 matrices");
		return {
		    {1, 0, 0, 0},
		    {0, Cos(angle), Sin(angle), 0},
		    {0, -Sin(angle), Cos(angle), 0},
		    {0, 0, 0, 1},
		};
	}

	static Matrix<T, 4, 4> RotationY(AngleT<T> angle) noexcept
	{
		static_assert(M == 4, "RotationY only works with 4x4 matrices");
		return {
		    {Cos(angle), 0, -Sin(angle), 0},
		    {0, 1, 0, 0},
		    {Sin(angle), 0, Cos(angle), 0},
		    {0, 0, 0, 1},
		};
	}

	static Matrix<T, 4, 4> RotationZ(AngleT<T> angle) noexcept
	{
		static_assert(M == 4, "RotationZ only works with 4x4 matrices");
		return {
		    {Cos(angle), Sin(angle), 0, 0},
		    {-Sin(angle), Cos(angle), 0, 0},
		    {0, 0, 1, 0},
		    {0, 0, 0, 1},
		};
	}

private:
	static constexpr auto& Members()
	{
		static constexpr auto members = std::array{&Matrix::x, &Matrix::y, &Matrix::z, &Matrix::w};
		return members;
	}
};

using Matrix2 = Matrix<float, 2, 2>;
using Matrix3 = Matrix<float, 3, 3>;
using Matrix4 = Matrix<float, 4, 4>;

template <class T, int M>
Matrix<T, M, M> operator*(const Matrix<T, M, M>& a, const Matrix<T, M, M>& b)
{
	Matrix<T, M, M> ret{};
	for (int row = 0; row < M; row++)
	{
		for (int col = 0; col < M; col++)
		{
			for (int i = 0; i < M; i++)
			{
				ret[row][col] += a[row][i] * b[i][col];
			}
		}
	}
	return ret;
}

template <class T, int M, class VectorTag>
Vector<T, M, VectorTag> operator*(const Matrix<T, M, M>& a, const Vector<T, M, VectorTag>& b)
{
	Vector<T, M, VectorTag> ret{};
	for (int row = 0; row < M; row++)
	{
		for (int col = 0; col < M; col++)
		{
			ret[row] += a[row][col] * b[col];
		}
	}
	return ret;
}

template <class T, int M, int N>
Matrix<T, N, M> Transpose(const Matrix<T, M, N>& m)
{
	Matrix<T, N, M> ret;
	for (int row = 0; row < M; row++)
	{
		for (int col = 0; col < M; col++)
		{
			ret[col][row] = m[row][col];
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
	    {right.x, realUp.x, forward.x, 0},
	    {right.y, realUp.y, forward.y, 0},
	    {right.z, realUp.z, forward.z, 0},
	    {eyeX, eyeY, eyeZ, 1},
	};
}

template <class T>
Matrix<T, 4, 4> Perspective(AngleT<T> fovy, T aspect, T near, T far) noexcept
{
	T const tanHalfFovy = Tan(fovy / static_cast<T>(2));

	return {
	    {T{1} / (aspect * tanHalfFovy), 0, 0, 0},
	    {0, T{-1} / (tanHalfFovy), 0, 0},
	    {0, 0, (far + near) / (far - near), T{1}},
	    {0, 0, -(T{2} * far * near) / (far - near), 0},
	};
}

} // namespace Geo
