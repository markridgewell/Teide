
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
};

template <class T, int M>
struct Matrix<T, M, 3>
{
	Vector<T, M, VectorTag> x, y, z;
};

template <class T, int M>
struct Matrix<T, M, 4>
{
	Vector<T, M, VectorTag> x, y, z, w;
};

template <class T>
struct Matrix<T, 4, 4>
{
	Vector<T, 4, VectorTag> x, y, z, w;

	static Matrix<T, 4, 4> RotationX(Angle angle) noexcept
	{
		return {
		    {1, 0, 0, 0},
		    {0, Cos(angle), Sin(angle), 0},
		    {0, -Sin(angle), Cos(angle), 0},
		    {0, 0, 0, 1},
		};
	}

	static Matrix<T, 4, 4> RotationY(Angle angle) noexcept
	{
		return {
		    {Cos(angle), 0, -Sin(angle), 0},
		    {0, 1, 0, 0},
		    {Sin(angle), 0, Cos(angle), 0},
		    {0, 0, 0, 1},
		};
	}

	static Matrix<T, 4, 4> RotationZ(Angle angle) noexcept
	{
		return {
		    {Cos(angle), Sin(angle), 0, 0},
		    {-Sin(angle), Cos(angle), 0, 0},
		    {0, 0, 1, 0},
		    {0, 0, 0, 1},
		};
	}

	static Matrix<T, 4, 4> LookAt(const Point3& eye, const Point3& target, const Vector3& up) noexcept
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

	static Matrix<T, 4, 4> Perspective(Angle fovy, T aspect, T near, T far) noexcept
	{
		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		return {
		    {T{1} / (aspect * tanHalfFovy), 0, 0, 0},
		    {0, T{-1} / (tanHalfFovy), 0, 0},
		    {0, 0, (far + near) / (far - near), T{1}},
		    {0, 0, -(T{2} * far * near) / (far - near), 0},
		};
	}
};

using Matrix2 = Matrix<float, 2, 2>;
using Matrix3 = Matrix<float, 3, 3>;
using Matrix4 = Matrix<float, 4, 4>;

} // namespace Geo
