
#include "GeoLib/Matrix.h"

#include "TestUtils.h"

#include <gtest/gtest.h>

using namespace Geo;
using namespace testing;

namespace
{
TEST(MatrixTest, MultiplyMatrix4Matrix4)
{
	const Matrix4 v1{{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}};
	const Matrix4 v2{{17, 18, 19, 20}, {21, 22, 23, 24}, {25, 26, 27, 28}, {29, 30, 31, 32}};
	const Matrix4 expected1{{250, 260, 270, 280}, {618, 644, 670, 696}, {986, 1028, 1070, 1112}, {1354, 1412, 1470, 1528}};
	const Matrix4 expected2{{538, 612, 686, 760}, {650, 740, 830, 920}, {762, 868, 974, 1080}, {874, 996, 1118, 1240}};

	EXPECT_EQ(expected1, v1 * v2);
	EXPECT_EQ(expected2, v2 * v1);
}

TEST(MatrixTest, MultiplyMatrix4Vector4)
{
	EXPECT_THAT(Matrix4::Identity() * Vector4(1, 2, 3, 1), ApproxEq(Vector4(1, 2, 3, 1)));

	const auto rotation = Matrix4{
	    {1, 0, 0, 0},
	    {0, 0, -1, 0},
	    {0, 1, 0, 0},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(rotation * Vector4(1, 2, 3, 1), ApproxEq(Vector4(1, -3, 2, 1)));

	const auto translation = Matrix4{
	    {1, 0, 0, 10},
	    {0, 1, 0, 50},
	    {0, 0, 1, 20},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(translation * Vector4(1, 2, 3, 1), ApproxEq(Vector4(11, 52, 23, 1)));

	const auto scaling = Matrix4{
	    {10, 0, 0, 0},
	    {0, 2, 0, 0},
	    {0, 0, 100, 0},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(scaling * Vector4(1, 2, 3, 1), ApproxEq(Vector4(10, 4, 300, 1)));

	EXPECT_THAT((scaling * translation) * Vector4(1, 2, 3, 1), ApproxEq(Vector4(110, 104, 2300, 1)));
	EXPECT_THAT(scaling * (translation * Vector4(1, 2, 3, 1)), ApproxEq(Vector4(110, 104, 2300, 1)));
	EXPECT_THAT((translation * scaling) * Vector4(1, 2, 3, 1), ApproxEq(Vector4(20, 54, 320, 1)));
	EXPECT_THAT(translation * (scaling * Vector4(1, 2, 3, 1)), ApproxEq(Vector4(20, 54, 320, 1)));
}

TEST(MatrixTest, MultiplyMatrix3x2Matrix4x3)
{
	const auto a = Matrix<float, 3, 2>{
	    {1, 2, 3},
	    {4, 5, 6},
	};

	const auto b = Matrix<float, 4, 3>{
	    {1, 2, 3, 4},
	    {5, 6, 7, 8},
	    {9, 10, 11, 12},
	};

	const auto expected = Matrix<float, 4, 2>{
	    {38, 44, 50, 56},
	    {83, 98, 113, 128},
	};

	EXPECT_THAT(a * b, Eq(expected));
}

TEST(MatrixTest, MultiplyMatrix4x1Matrix1x4)
{
	const auto a = Matrix<float, 4, 1>{
	    {1, 2, 3, 4},
	};

	const auto b = Matrix<float, 1, 4>{
	    {5},
	    {6},
	    {7},
	    {8},
	};

	const auto expected = Matrix<float, 1, 1>{
	    {70},
	};

	// This is equivalent to the dot product
	EXPECT_THAT(a * b, Eq(expected));
}

TEST(MatrixTest, Matrix4Rotation)
{
	EXPECT_THAT(Matrix4::RotationX(90.0_deg) * Vector4(0, 2, 0, 1), ApproxEq(Vector4(0, 0, 2, 1)));
	EXPECT_THAT(Matrix4::RotationX(90.0_deg) * Vector4(1, 2, 3, 1), ApproxEq(Vector4(1, -3, 2, 1)));
	EXPECT_THAT(Matrix4::RotationY(90.0_deg) * Vector4(2, 0, 0, 1), ApproxEq(Vector4(0, 0, -2, 1)));
}

} // namespace

namespace Geo
{
// Explicit instantiations to ensure correct code coverage
template Matrix<float, 1, 1>;
template Matrix<float, 2, 2>;
template Matrix<float, 3, 3>;
template Matrix<float, 4, 4>;

template Matrix<float, 4, 4> operator*(const Matrix<float, 4, 4>& a, const Matrix<float, 4, 4>& b) noexcept;
template Vector<float, 4, VectorTag> operator*(const Matrix<float, 4, 4>& a, const Vector<float, 4, VectorTag>& b) noexcept;
template Vector<float, 3, VectorTag> operator*(const Matrix<float, 4, 4>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, PointTag> operator*(const Matrix<float, 4, 4>& a, const Vector<float, 3, PointTag>& b) noexcept;
template Matrix<float, 4, 4> Transpose(const Matrix<float, 4, 4>& m) noexcept;
template Matrix<float, 4, 4> LookAt(
    const Vector<float, 3, PointTag>& eye, const Vector<float, 3, PointTag>& target,
    const Vector<float, 3, VectorTag>& up) noexcept;
template Matrix<float, 4, 4> Perspective(AngleT<float> fovy, float aspect, float near, float far) noexcept;
template Matrix<float, 4, 4> Orthographic(float width, float height) noexcept;
template Matrix<float, 4, 4> Orthographic(float left, float right, float bottom, float top, float nclip, float fclip) noexcept;

} // namespace Geo
