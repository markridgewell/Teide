
#include "GeoLib/Matrix.h"

#include "TestUtils.h"

#include <gtest/gtest.h>

using namespace Geo;
using namespace testing;

using Matrix1x1 = Matrix<float, 1, 1>;
using Matrix1x2 = Matrix<float, 1, 2>;
using Matrix1x3 = Matrix<float, 1, 3>;
using Matrix1x4 = Matrix<float, 1, 4>;
using Matrix2x1 = Matrix<float, 2, 1>;
using Matrix2x2 = Matrix<float, 2, 2>;
using Matrix2x3 = Matrix<float, 2, 3>;
using Matrix2x4 = Matrix<float, 2, 4>;
using Matrix3x1 = Matrix<float, 3, 1>;
using Matrix3x2 = Matrix<float, 3, 2>;
using Matrix3x3 = Matrix<float, 3, 3>;
using Matrix3x4 = Matrix<float, 3, 4>;
using Matrix4x1 = Matrix<float, 4, 1>;
using Matrix4x2 = Matrix<float, 4, 2>;
using Matrix4x3 = Matrix<float, 4, 3>;
using Matrix4x4 = Matrix<float, 4, 4>;

namespace
{
TEST(MatrixTest, DefaultConstruct)
{
	EXPECT_THAT(Matrix1x1{}, Eq(Matrix1x1{{1}}));
	EXPECT_THAT(Matrix1x2{}, Eq(Matrix1x2{{1, 0}}));
	EXPECT_THAT(Matrix1x3{}, Eq(Matrix1x3{{1, 0, 0}}));
	EXPECT_THAT(Matrix1x4{}, Eq(Matrix1x4{{1, 0, 0, 0}}));
	EXPECT_THAT(Matrix2x1{}, Eq(Matrix2x1{{1}, {0}}));
	EXPECT_THAT(Matrix2x2{}, Eq(Matrix2x2{{1, 0}, {0, 1}}));
	EXPECT_THAT(Matrix2x3{}, Eq(Matrix2x3{{1, 0, 0}, {0, 1, 0}}));
	EXPECT_THAT(Matrix2x4{}, Eq(Matrix2x4{{1, 0, 0, 0}, {0, 1, 0, 0}}));
	EXPECT_THAT(Matrix3x1{}, Eq(Matrix3x1{{1}, {0}, {0}}));
	EXPECT_THAT(Matrix3x2{}, Eq(Matrix3x2{{1, 0}, {0, 1}, {0, 0}}));
	EXPECT_THAT(Matrix3x3{}, Eq(Matrix3x3{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}));
	EXPECT_THAT(Matrix3x4{}, Eq(Matrix3x4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}));
	EXPECT_THAT(Matrix4x1{}, Eq(Matrix4x1{{1}, {0}, {0}, {0}}));
	EXPECT_THAT(Matrix4x2{}, Eq(Matrix4x2{{1, 0}, {0, 1}, {0, 0}, {0, 0}}));
	EXPECT_THAT(Matrix4x3{}, Eq(Matrix4x3{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}}));
	EXPECT_THAT(Matrix4x4{}, Eq(Matrix4x4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}));
}

TEST(MatrixTest, Zero)
{
	EXPECT_THAT(Matrix1x1::Zero(), Eq(Matrix1x1{{0}}));
	EXPECT_THAT(Matrix1x2::Zero(), Eq(Matrix1x2{{0, 0}}));
	EXPECT_THAT(Matrix1x3::Zero(), Eq(Matrix1x3{{0, 0, 0}}));
	EXPECT_THAT(Matrix1x4::Zero(), Eq(Matrix1x4{{0, 0, 0, 0}}));
	EXPECT_THAT(Matrix2x1::Zero(), Eq(Matrix2x1{{0}, {0}}));
	EXPECT_THAT(Matrix2x2::Zero(), Eq(Matrix2x2{{0, 0}, {0, 0}}));
	EXPECT_THAT(Matrix2x3::Zero(), Eq(Matrix2x3{{0, 0, 0}, {0, 0, 0}}));
	EXPECT_THAT(Matrix2x4::Zero(), Eq(Matrix2x4{{0, 0, 0, 0}, {0, 0, 0, 0}}));
	EXPECT_THAT(Matrix3x1::Zero(), Eq(Matrix3x1{{0}, {0}, {0}}));
	EXPECT_THAT(Matrix3x2::Zero(), Eq(Matrix3x2{{0, 0}, {0, 0}, {0, 0}}));
	EXPECT_THAT(Matrix3x3::Zero(), Eq(Matrix3x3{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}));
	EXPECT_THAT(Matrix3x4::Zero(), Eq(Matrix3x4{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}));
	EXPECT_THAT(Matrix4x1::Zero(), Eq(Matrix4x1{{0}, {0}, {0}, {0}}));
	EXPECT_THAT(Matrix4x2::Zero(), Eq(Matrix4x2{{0, 0}, {0, 0}, {0, 0}, {0, 0}}));
	EXPECT_THAT(Matrix4x3::Zero(), Eq(Matrix4x3{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}));
	EXPECT_THAT(Matrix4x4::Zero(), Eq(Matrix4x4{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}));
}

TEST(MatrixTest, Identity)
{
	EXPECT_THAT(Matrix1x1::Identity(), Eq(Matrix1x1{{1}}));
	EXPECT_THAT(Matrix1x2::Identity(), Eq(Matrix1x2{{1, 0}}));
	EXPECT_THAT(Matrix1x3::Identity(), Eq(Matrix1x3{{1, 0, 0}}));
	EXPECT_THAT(Matrix1x4::Identity(), Eq(Matrix1x4{{1, 0, 0, 0}}));
	EXPECT_THAT(Matrix2x1::Identity(), Eq(Matrix2x1{{1}, {0}}));
	EXPECT_THAT(Matrix2x2::Identity(), Eq(Matrix2x2{{1, 0}, {0, 1}}));
	EXPECT_THAT(Matrix2x3::Identity(), Eq(Matrix2x3{{1, 0, 0}, {0, 1, 0}}));
	EXPECT_THAT(Matrix2x4::Identity(), Eq(Matrix2x4{{1, 0, 0, 0}, {0, 1, 0, 0}}));
	EXPECT_THAT(Matrix3x1::Identity(), Eq(Matrix3x1{{1}, {0}, {0}}));
	EXPECT_THAT(Matrix3x2::Identity(), Eq(Matrix3x2{{1, 0}, {0, 1}, {0, 0}}));
	EXPECT_THAT(Matrix3x3::Identity(), Eq(Matrix3x3{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}));
	EXPECT_THAT(Matrix3x4::Identity(), Eq(Matrix3x4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}));
	EXPECT_THAT(Matrix4x1::Identity(), Eq(Matrix4x1{{1}, {0}, {0}, {0}}));
	EXPECT_THAT(Matrix4x2::Identity(), Eq(Matrix4x2{{1, 0}, {0, 1}, {0, 0}, {0, 0}}));
	EXPECT_THAT(Matrix4x3::Identity(), Eq(Matrix4x3{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}}));
	EXPECT_THAT(Matrix4x4::Identity(), Eq(Matrix4x4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}));
}

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

TEST(MatrixTest, MultiplyMatrix4Vector3)
{
	EXPECT_THAT(Matrix4::Identity() * Vector3(1, 2, 3), ApproxEq(Vector3(1, 2, 3)));

	const auto rotation = Matrix4{
	    {1, 0, 0, 0},
	    {0, 0, -1, 0},
	    {0, 1, 0, 0},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(rotation * Vector3(1, 2, 3), ApproxEq(Vector3(1, -3, 2)));

	const auto translation = Matrix4{
	    {1, 0, 0, 10},
	    {0, 1, 0, 50},
	    {0, 0, 1, 20},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(translation * Vector3(1, 2, 3), ApproxEq(Vector3(1, 2, 3))); // Vectors can't be translated, only Points!

	const auto scaling = Matrix4{
	    {10, 0, 0, 0},
	    {0, 2, 0, 0},
	    {0, 0, 100, 0},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(scaling * Vector3(1, 2, 3), ApproxEq(Vector3(10, 4, 300)));

	EXPECT_THAT((scaling * translation) * Vector3(1, 2, 3), ApproxEq(Vector3(10, 4, 300)));
	EXPECT_THAT(scaling * (translation * Vector3(1, 2, 3)), ApproxEq(Vector3(10, 4, 300)));
	EXPECT_THAT((translation * scaling) * Vector3(1, 2, 3), ApproxEq(Vector3(10, 4, 300)));
	EXPECT_THAT(translation * (scaling * Vector3(1, 2, 3)), ApproxEq(Vector3(10, 4, 300)));
}

TEST(MatrixTest, MultiplyMatrix4Point3)
{
	EXPECT_THAT(Matrix4::Identity() * Point3(1, 2, 3), ApproxEq(Point3(1, 2, 3)));

	const auto rotation = Matrix4{
	    {1, 0, 0, 0},
	    {0, 0, -1, 0},
	    {0, 1, 0, 0},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(rotation * Point3(1, 2, 3), ApproxEq(Point3(1, -3, 2)));

	const auto translation = Matrix4{
	    {1, 0, 0, 10},
	    {0, 1, 0, 50},
	    {0, 0, 1, 20},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(translation * Point3(1, 2, 3), ApproxEq(Point3(11, 52, 23)));

	const auto scaling = Matrix4{
	    {10, 0, 0, 0},
	    {0, 2, 0, 0},
	    {0, 0, 100, 0},
	    {0, 0, 0, 1},
	};
	EXPECT_THAT(scaling * Point3(1, 2, 3), ApproxEq(Point3(10, 4, 300)));

	EXPECT_THAT((scaling * translation) * Point3(1, 2, 3), ApproxEq(Point3(110, 104, 2300)));
	EXPECT_THAT(scaling * (translation * Point3(1, 2, 3)), ApproxEq(Point3(110, 104, 2300)));
	EXPECT_THAT((translation * scaling) * Point3(1, 2, 3), ApproxEq(Point3(20, 54, 320)));
	EXPECT_THAT(translation * (scaling * Point3(1, 2, 3)), ApproxEq(Point3(20, 54, 320)));
}

TEST(MatrixTest, MultiplyMatrix2x3Matrix3x4)
{
	const auto a = Matrix2x3{
	    {1, 2, 3},
	    {4, 5, 6},
	};

	const auto b = Matrix3x4{
	    {1, 2, 3, 4},
	    {5, 6, 7, 8},
	    {9, 10, 11, 12},
	};

	const auto expected = Matrix2x4{
	    {38, 44, 50, 56},
	    {83, 98, 113, 128},
	};

	EXPECT_THAT(a * b, Eq(expected));
}

TEST(MatrixTest, MultiplyMatrix1x4Matrix4x1)
{
	const auto a = Matrix1x4{
	    {1, 2, 3, 4},
	};

	const auto b = Matrix4x1{
	    {5},
	    {6},
	    {7},
	    {8},
	};

	const auto expected = Matrix1x1{
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
	EXPECT_THAT(Matrix4::RotationZ(90.0_deg) * Vector4(0, 2, 0, 1), ApproxEq(Vector4(-2, 0, 0, 1)));
}

TEST(MatrixTest, TransposeMatrix4)
{
	const auto a = Matrix4{
	    {1, 2, 3, 4},
	    {5, 6, 7, 8},
	    {9, 10, 11, 12},
	    {13, 14, 15, 16},
	};
	const auto b = Matrix4{
	    {1, 5, 9, 13},
	    {2, 6, 10, 14},
	    {3, 7, 11, 15},
	    {4, 8, 12, 16},
	};
	EXPECT_THAT(Transpose(a), Eq(b));
	EXPECT_THAT(Transpose(b), Eq(a));
}

TEST(MatrixTest, TransposeMatrix3x4)
{
	const auto a = Matrix3x4{
	    {1, 2, 3, 4},
	    {5, 6, 7, 8},
	    {9, 10, 11, 12},
	};
	const auto b = Matrix4x3{
	    {1, 5, 9},
	    {2, 6, 10},
	    {3, 7, 11},
	    {4, 8, 12},
	};
	EXPECT_THAT(Transpose(a), Eq(b));
	EXPECT_THAT(Transpose(b), Eq(a));
}

TEST(MatrixTest, LookAt)
{
	const Point3 eyePos = {2, 3, 5};
	const Point3 target = {1, 1, 3};
	const Vector3 up = {0, 1, 0};
	const Matrix4 m = LookAt(eyePos, target, up);

	EXPECT_THAT(m * eyePos, ApproxEq(Point3{0, 0, 0}));
	EXPECT_THAT(m * target, ApproxEq(Point3{0, 0, 3}));
	EXPECT_THAT(m * (Point3{1, 6, 3}), ApproxEq(Point3{0, 3.72678018f, -0.3333333f}));
	EXPECT_THAT(m * (Point3{3, 1, 3}), ApproxEq(Point3{-1.78885436f, -0.596284628f, 2.33333325f}));
}

TEST(MatrixTest, Perspective)
{
	const Matrix4 m = Perspective(90.0_deg, 1.0f, 1.0f, 100.0f);

	const Point3 ntl = {-1.0f, 1.0f, 1.0f};
	const Point3 nbr = {1.0f, -1.0f, 1.0f};
	const Point3 ftl = {-100.0f, 100.0f, 100.0f};
	const Point3 fbr = {100.0f, -100.0f, 100.0f};

	EXPECT_THAT(m * ntl, ApproxEq(Point3{-1.0f, -1.0f, -1.0f}));
	EXPECT_THAT(m * nbr, ApproxEq(Point3{1.0f, 1.0f, -1.0f}));
	EXPECT_THAT(m * ftl, ApproxEq(Point3{-1.0f, -1.0f, 1.0f}));
	EXPECT_THAT(m * fbr, ApproxEq(Point3{1.0f, 1.0f, 1.0f}));
}

TEST(MatrixTest, PerspectiveRect)
{
	const Matrix4 m = Perspective(90.0_deg, 2.0f, 1.0f, 100.0f);

	const Point3 ntl = {-2.0f, 1.0f, 1.0f};
	const Point3 nbr = {2.0f, -1.0f, 1.0f};
	const Point3 ftl = {-200.0f, 100.0f, 100.0f};
	const Point3 fbr = {200.0f, -100.0f, 100.0f};

	EXPECT_THAT(m * ntl, ApproxEq(Point3{-1.0f, -1.0f, -1.0f}));
	EXPECT_THAT(m * nbr, ApproxEq(Point3{1.0f, 1.0f, -1.0f}));
	EXPECT_THAT(m * ftl, ApproxEq(Point3{-1.0f, -1.0f, 1.0f}));
	EXPECT_THAT(m * fbr, ApproxEq(Point3{1.0f, 1.0f, 1.0f}));
}

TEST(MatrixTest, Orthographic)
{
	const Matrix4 m = Orthographic(1000.0f, 400.0f);

	const Point3 tl = {0.0f, 0.0f, 0.0f};
	const Point3 br = {1000.0f, 400.0f, 0.0f};
	const Point3 mid = {500.0f, 200.0f, 0.5f};

	EXPECT_THAT(m * tl, ApproxEq(Point3{-1.0f, -1.0f, 0.0f}));
	EXPECT_THAT(m * br, ApproxEq(Point3{1.0f, 1.0f, 0.0f}));
	EXPECT_THAT(m * mid, ApproxEq(Point3{0.0f, 0.0f, 0.5f}));
}

TEST(MatrixTest, OrthographicPlanes)
{
	const Matrix4 m = Orthographic(-100.0f, 300.0f, 50.0f, 250.0f, -1.0f, 2.0f);

	const Point3 tl = {-100.0f, 50.0f, -1.0f};
	const Point3 br = {300.0f, 250.0f, -1.0f};
	const Point3 mid = {100.0f, 150.0f, 0.5f};

	EXPECT_THAT(m * tl, ApproxEq(Point3{-1.0f, -1.0f, 0.0f}));
	EXPECT_THAT(m * br, ApproxEq(Point3{1.0f, 1.0f, 0.0f}));
	EXPECT_THAT(m * mid, ApproxEq(Point3{0.0f, 0.0f, 0.5f}));
}

} // namespace

#ifdef _MSC_VER
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
#endif
