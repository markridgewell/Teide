
#include "GeoLib/Vector.h"

#include "TestUtils.h"

#include <gtest/gtest.h>

using namespace Geo;
using namespace testing;

namespace
{
using Vector1 = Vector<float, 1, VectorTag>;
using Point1 = Vector<float, 1, PointTag>;

TEST(VectorTest, ConstructVector1)
{
	const Vector1 v{1.0f};
	EXPECT_EQ(1.0f, v.x);
}

TEST(VectorTest, ConstructVector2)
{
	const Vector2 v{1.0f, 2.0f};
	EXPECT_EQ(1.0f, v.x);
	EXPECT_EQ(2.0f, v.y);
}

TEST(VectorTest, ConstructVector3)
{
	const Vector3 v{1.0f, 2.0f, 3.0f};
	EXPECT_EQ(1.0f, v.x);
	EXPECT_EQ(2.0f, v.y);
	EXPECT_EQ(3.0f, v.z);
}

TEST(VectorTest, ConstructVector4)
{
	const Vector4 v{1.0f, 2.0f, 3.0f, 4.0f};
	EXPECT_EQ(1.0f, v.x);
	EXPECT_EQ(2.0f, v.y);
	EXPECT_EQ(3.0f, v.z);
	EXPECT_EQ(4.0f, v.w);
}

TEST(VectorTest, OperationsVector1)
{
	const Vector1 v1{2.0f};
	const Vector1 v2{4.0f};
	const Point1 p1{-2.0f};
	const Point1 p2{2.5f};

	EXPECT_THAT(v1 + v2, ApproxEq(Vector1{6.0f}));
	EXPECT_THAT(v1 - v2, ApproxEq(Vector1{-2.0f}));
	EXPECT_THAT(v1 * 2.0f, ApproxEq(Vector1{4.0f}));
	EXPECT_THAT(v1 / 2.0f, ApproxEq(Vector1{1.0f}));
	EXPECT_THAT(-v1, ApproxEq(Vector1{-2.0f}));

	EXPECT_THAT(p1 + v1, ApproxEq(Point1{0.0f}));
	EXPECT_THAT(p1 - v1, ApproxEq(Point1{-4.0f}));
	EXPECT_THAT(p1 - p2, ApproxEq(Vector1{-4.5f}));
	EXPECT_THAT(p1 * -1.0f, ApproxEq(Point1{2.0f}));
	EXPECT_THAT(p2 / 10.0f, ApproxEq(Point1{0.25f}));
	EXPECT_THAT(v1 + p1, ApproxEq(Point1{0.0f}));
}

TEST(VectorTest, OperationsVector2)
{
	const Vector2 v1{3.0f, 2.0f};
	const Vector2 v2{2.0f, 4.0f};
	const Point2 p1{4.0f, -2.0f};
	const Point2 p2{1.5f, 2.5f};

	EXPECT_THAT(v1 + v2, ApproxEq(Vector2{5.0f, 6.0f}));
	EXPECT_THAT(v1 - v2, ApproxEq(Vector2{1.0f, -2.0f}));
	EXPECT_THAT(v1 * 2.0f, ApproxEq(Vector2{6.0f, 4.0f}));
	EXPECT_THAT(v1 / 2.0f, ApproxEq(Vector2{1.5f, 1.0f}));
	EXPECT_THAT(-v1, ApproxEq(Vector2{-3.0f, -2.0f}));

	EXPECT_THAT(p1 + v1, ApproxEq(Point2{7.0f, 0.0f}));
	EXPECT_THAT(p1 - v1, ApproxEq(Point2{1.0f, -4.0f}));
	EXPECT_THAT(p1 - p2, ApproxEq(Vector2{2.5f, -4.5f}));
	EXPECT_THAT(p1 * -1.0f, ApproxEq(Point2{-4.0f, 2.0f}));
	EXPECT_THAT(p2 / 10.0f, ApproxEq(Point2{0.15f, 0.25f}));
	EXPECT_THAT(v1 + p1, ApproxEq(Point2{7.0f, 0.0f}));
}

TEST(VectorTest, OperationsVector3)
{
	const Vector3 v1{3.0f, 2.0f, 1.0f};
	const Vector3 v2{2.0f, 4.0f, 6.0f};
	const Point3 p1{4.0f, -2.0f, 3.0f};
	const Point3 p2{1.5f, 2.5f, 0.0f};

	EXPECT_THAT(v1 + v2, ApproxEq(Vector3{5.0f, 6.0f, 7.0f}));
	EXPECT_THAT(v1 - v2, ApproxEq(Vector3{1.0f, -2.0f, -5.0f}));
	EXPECT_THAT(v1 * 2.0f, ApproxEq(Vector3{6.0f, 4.0f, 2.0f}));
	EXPECT_THAT(v1 / 2.0f, ApproxEq(Vector3{1.5f, 1.0f, 0.5f}));
	EXPECT_THAT(-v1, ApproxEq(Vector3{-3.0f, -2.0f, -1.0f}));

	EXPECT_THAT(p1 + v1, ApproxEq(Point3{7.0f, 0.0f, 4.0f}));
	EXPECT_THAT(p1 - v1, ApproxEq(Point3{1.0f, -4.0f, 2.0f}));
	EXPECT_THAT(p1 - p2, ApproxEq(Vector3{2.5f, -4.5f, 3.0f}));
	EXPECT_THAT(p1 * -1.0f, ApproxEq(Point3{-4.0f, 2.0f, -3.0f}));
	EXPECT_THAT(p2 / 10.0f, ApproxEq(Point3{0.15f, 0.25f, 0.0f}));
	EXPECT_THAT(v1 + p1, ApproxEq(Point3{7.0f, 0.0f, 4.0f}));
}

TEST(VectorTest, InPlaceOperationsVector3)
{
	const Vector3 v1{3.0f, 2.0f, 1.0f};
	const Vector3 v2{2.0f, 4.0f, 6.0f};
	const Point3 p1{4.0f, -2.0f, 3.0f};
	const Point3 p2{1.5f, 2.5f, 0.0f};

	Vector3 v = v1;
	v += v2;
	EXPECT_THAT(v, ApproxEq(Vector3{5.0f, 6.0f, 7.0f}));

	v = v1;
	v -= v2;
	EXPECT_THAT(v, ApproxEq(Vector3{1.0f, -2.0f, -5.0f}));

	v = v1;
	v *= 2.0f;
	EXPECT_THAT(v, ApproxEq(Vector3{6.0f, 4.0f, 2.0f}));

	v = v1;
	v /= 2.0f;
	EXPECT_THAT(v, ApproxEq(Vector3{1.5f, 1.0f, 0.5f}));

	Point3 p = p1;
	p += v1;
	EXPECT_THAT(p, ApproxEq(Point3{7.0f, 0.0f, 4.0f}));

	p = p1;
	p -= v1;
	EXPECT_THAT(p, ApproxEq(Point3{1.0f, -4.0f, 2.0f}));

	p = p1;
	p -= p2;
	EXPECT_THAT(p1 - p2, ApproxEq(Vector3{2.5f, -4.5f, 3.0f}));

	p = p1;
	p *= -1.0f;
	EXPECT_THAT(p, ApproxEq(Point3{-4.0f, 2.0f, -3.0f}));

	p = p2;
	p /= 1.0f;
	EXPECT_THAT(p2 / 10.0f, ApproxEq(Point3{0.15f, 0.25f, 0.0f}));
}

TEST(VectorTest, OperationsVector4)
{
	const Vector4 a{5.0f, 6.0f, 7.0f, 8.0f};
	const Vector4 b{1.0f, 2.0f, 3.0f, 4.0f};

	EXPECT_THAT(a + b, ApproxEq(Vector4{6.0f, 8.0f, 10.0f, 12.0f}));
	EXPECT_THAT(a - b, ApproxEq(Vector4{4.0f, 4.0f, 4.0f, 4.0f}));
	EXPECT_THAT(a * 2.0f, ApproxEq(Vector4{10.0f, 12.0f, 14.0f, 16.0f}));
	EXPECT_THAT(a / 2.0f, ApproxEq(Vector4{2.5f, 3.0f, 3.5f, 4.0f}));
	EXPECT_THAT(-b, ApproxEq(Vector4{-1.0f, -2.0f, -3.0f, -4.0f}));
}

TEST(VectorTest, MagnitudeVector1)
{
	EXPECT_THAT(Magnitude(Vector1{3.0f}), ApproxEq(3.0f));
}

TEST(VectorTest, NormalizeVector1)
{
	EXPECT_THAT(Normalise(Vector1{-3.0f}), ApproxEq(Vector1{-1.0f}));
}

TEST(VectorTest, MagnitudeVector2)
{
	EXPECT_THAT(Magnitude(Vector2{3.0f, 4.0f}), ApproxEq(5.0f));
}

TEST(VectorTest, NormalizeVector2)
{
	EXPECT_THAT(Normalise(Vector2{3.0f, 4.0f}), ApproxEq(Vector2{0.6f, 0.8f}));
}

TEST(VectorTest, MagnitudeVector3)
{
	EXPECT_THAT(Magnitude(Vector3{2.0f, 1.0f, 2.0f}), ApproxEq(3.0f));
}

TEST(VectorTest, NormalizeVector3)
{
	EXPECT_THAT(Normalise(Vector3{2.0f, 1.0f, 2.0f}), ApproxEq(Vector3{2.0f / 3.0f, 1.0f / 3.0f, 2.0f / 3.0f}));
}

TEST(VectorTest, MagnitudeVector4)
{
	EXPECT_THAT(Magnitude(Vector4{2.0f, -2.0f, 2.0f, -2.0f}), ApproxEq(4.0f));
}

TEST(VectorTest, NormalizeVector4)
{
	EXPECT_THAT(Normalise(Vector4{2.0f, -2.0f, 2.0f, -2.0f}), ApproxEq(Vector4{0.5f, -0.5f, 0.5f, -0.5f}));
}

TEST(VectorTest, DotVector4)
{
	const auto a = Vector4{1, 2, 3, 4};
	const auto b = Vector4{5, 6, 7, 8};

	EXPECT_THAT(Dot(a, b), Eq(70));
}

TEST(VectorTest, CrossVector3)
{
	const auto a = Vector3{1, 2, 3};
	const auto b = Vector3{4, 5, 6};

	EXPECT_THAT(Cross(a, b), Eq(Vector3{-3, 6, -3}));
}

} // namespace

#ifdef _MSC_VER
namespace Geo
{
// Explicit instantiations to ensure correct code coverage
template Vector<float, 1, VectorTag>;
template Vector<float, 2, VectorTag>;
template Vector<float, 3, VectorTag>;
template Vector<float, 4, VectorTag>;

template Vector<float, 3, VectorTag> operator+(const Vector<float, 3, VectorTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, VectorTag>& operator+=(Vector<float, 3, VectorTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, PointTag> operator+(const Vector<float, 3, PointTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, PointTag>& operator+=(Vector<float, 3, PointTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, PointTag> operator+(const Vector<float, 3, VectorTag>& a, const Vector<float, 3, PointTag>& b) noexcept;
template Vector<float, 3, VectorTag> operator-(const Vector<float, 3, VectorTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, VectorTag>& operator-=(Vector<float, 3, VectorTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, PointTag> operator-(const Vector<float, 3, PointTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, PointTag>& operator-=(Vector<float, 3, PointTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, VectorTag> operator-(const Vector<float, 3, PointTag>& a, const Vector<float, 3, PointTag>& b) noexcept;
template Vector<float, 3, VectorTag> operator-(const Vector<float, 3, VectorTag>& a) noexcept;
template Vector<float, 3, VectorTag> operator*(const Vector<float, 3, VectorTag>& a, float b) noexcept;
template Vector<float, 3, VectorTag> operator/(const Vector<float, 3, VectorTag>& a, float b) noexcept;

template float Magnitude(const Vector<float, 3, VectorTag>& a) noexcept;
template Vector<float, 3, VectorTag> Normalise(const Vector<float, 3, VectorTag>& a) noexcept;
template float Dot(const Vector<float, 3, VectorTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
template Vector<float, 3, VectorTag> Cross(const Vector<float, 3, VectorTag>& a, const Vector<float, 3, VectorTag>& b) noexcept;
} // namespace Geo
#endif
