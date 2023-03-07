
#include "GeoLib/Vector.h"

#include "TestUtils.h"

#include <gtest/gtest.h>

using namespace Geo;
using namespace testing;

namespace
{
using Vector1 = Vector<float, 1, VectorTag>;
using Point1 = Vector<float, 1, PointTag>;
using Scale1 = Vector<float, 1, ScaleTag>;

static_assert(std::is_same_v<ScalarType<Vector1>, float>);
static_assert(std::is_same_v<ScalarType<Vector2>, float>);
static_assert(std::is_same_v<ScalarType<Vector3>, float>);
static_assert(std::is_same_v<ScalarType<Vector4>, float>);
static_assert(std::is_same_v<ScalarType<Point1>, float>);
static_assert(std::is_same_v<ScalarType<Point2>, float>);
static_assert(std::is_same_v<ScalarType<Point3>, float>);
static_assert(std::is_same_v<ScalarType<Point4>, float>);
static_assert(std::is_same_v<ScalarType<Scale1>, float>);
static_assert(std::is_same_v<ScalarType<Scale2>, float>);
static_assert(std::is_same_v<ScalarType<Scale3>, float>);
static_assert(std::is_same_v<ScalarType<Scale4>, float>);

TEST(VectorTest, DefaultConstruct)
{
    EXPECT_THAT(Vector1{}, Eq(Vector1{0.0f}));
    EXPECT_THAT(Vector2{}, Eq(Vector2{0.0f, 0.0f}));
    EXPECT_THAT(Vector3{}, Eq(Vector3{0.0f, 0.0f, 0.0f}));
    EXPECT_THAT(Vector4{}, Eq(Vector4{0.0f, 0.0f, 0.0f, 0.0f}));
}

TEST(VectorTest, Zero)
{
    EXPECT_THAT(Vector1::Zero(), Eq(Vector1{0.0f}));
    EXPECT_THAT(Vector2::Zero(), Eq(Vector2{0.0f, 0.0f}));
    EXPECT_THAT(Vector3::Zero(), Eq(Vector3{0.0f, 0.0f, 0.0f}));
    EXPECT_THAT(Vector4::Zero(), Eq(Vector4{0.0f, 0.0f, 0.0f, 0.0f}));
}

TEST(VectorTest, UnitX)
{
    EXPECT_THAT(Vector1::UnitX(), Eq(Vector1{1.0f}));
    EXPECT_THAT(Vector2::UnitX(), Eq(Vector2{1.0f, 0.0f}));
    EXPECT_THAT(Vector3::UnitX(), Eq(Vector3{1.0f, 0.0f, 0.0f}));
    EXPECT_THAT(Vector4::UnitX(), Eq(Vector4{1.0f, 0.0f, 0.0f, 0.0f}));
}

TEST(VectorTest, UnitY)
{
    EXPECT_THAT(Vector2::UnitY(), Eq(Vector2{0.0f, 1.0f}));
    EXPECT_THAT(Vector3::UnitY(), Eq(Vector3{0.0f, 1.0f, 0.0f}));
    EXPECT_THAT(Vector4::UnitY(), Eq(Vector4{0.0f, 1.0f, 0.0f, 0.0f}));
}

TEST(VectorTest, UnitZ)
{
    EXPECT_THAT(Vector3::UnitZ(), Eq(Vector3{0.0f, 0.0f, 1.0f}));
    EXPECT_THAT(Vector4::UnitZ(), Eq(Vector4{0.0f, 0.0f, 1.0f, 0.0f}));
}

TEST(VectorTest, UnitW)
{
    EXPECT_THAT(Vector4::UnitW(), Eq(Vector4{0.0f, 0.0f, 0.0f, 1.0f}));
}

TEST(VectorTest, ConstructVector1)
{
    const Vector2 v1{2.0f};
    EXPECT_EQ(2.0f, v1.x);
}

TEST(VectorTest, ConstructVector2)
{
    const Vector2 v1{1.0f, 2.0f};
    EXPECT_EQ(1.0f, v1.x);
    EXPECT_EQ(2.0f, v1.y);

    const Vector2 v2{3.0f};
    EXPECT_EQ(3.0f, v2.x);
    EXPECT_EQ(3.0f, v2.y);
}

TEST(VectorTest, ConstructVector3)
{
    const Vector3 v1{1.0f, 2.0f, 3.0f};
    EXPECT_EQ(1.0f, v1.x);
    EXPECT_EQ(2.0f, v1.y);
    EXPECT_EQ(3.0f, v1.z);

    const Vector3 v2{10.0f};
    EXPECT_EQ(10.0f, v2.x);
    EXPECT_EQ(10.0f, v2.y);
    EXPECT_EQ(10.0f, v2.z);
}

TEST(VectorTest, ConstructVector4)
{
    const Vector4 v1{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_EQ(1.0f, v1.x);
    EXPECT_EQ(2.0f, v1.y);
    EXPECT_EQ(3.0f, v1.z);
    EXPECT_EQ(4.0f, v1.w);

    const Vector4 v2{5.0f};
    EXPECT_EQ(5.0f, v2.x);
    EXPECT_EQ(5.0f, v2.y);
    EXPECT_EQ(5.0f, v2.z);
    EXPECT_EQ(5.0f, v2.w);
}

TEST(VectorTest, ConstructVector1FromPoint1)
{
    const Vector1 v{Point1{1.0f}};
    EXPECT_EQ(1.0f, v.x);
}

TEST(VectorTest, ConstructVector2FromPoint2)
{
    const Vector2 v{Point2{1.0f, 2.0f}};
    EXPECT_EQ(1.0f, v.x);
    EXPECT_EQ(2.0f, v.y);
}

TEST(VectorTest, ConstructVector3FromPoint3)
{
    const Vector3 v{Point3{1.0f, 2.0f, 3.0f}};
    EXPECT_EQ(1.0f, v.x);
    EXPECT_EQ(2.0f, v.y);
    EXPECT_EQ(3.0f, v.z);
}

TEST(VectorTest, ConstructVector4FromPoint4)
{
    const Vector4 v{Point4{1.0f, 2.0f, 3.0f, 4.0f}};
    EXPECT_EQ(1.0f, v.x);
    EXPECT_EQ(2.0f, v.y);
    EXPECT_EQ(3.0f, v.z);
    EXPECT_EQ(4.0f, v.w);
}

TEST(VectorTest, AssignVector1)
{
    Vector1 v;
    v = {1};
    EXPECT_EQ(v.x, 1);
}

TEST(VectorTest, AssignVector2)
{
    Vector2 v;
    v = {2, 3};
    EXPECT_EQ(v.x, 2);
    EXPECT_EQ(v.y, 3);
}

TEST(VectorTest, AssignVector3)
{
    Vector3 v;
    v = {9, 10, 11};
    EXPECT_EQ(v.x, 9);
    EXPECT_EQ(v.y, 10);
    EXPECT_EQ(v.z, 11);
}

TEST(VectorTest, AssignVector4)
{
    Vector4 v;
    v = {9, 10, 11, 5};
    EXPECT_EQ(v.x, 9);
    EXPECT_EQ(v.y, 10);
    EXPECT_EQ(v.z, 11);
    EXPECT_EQ(v.w, 5);
}

TEST(VectorTest, CompareVector1)
{
    EXPECT_TRUE(Compare(Vector1{1.0f}, Vector1{1.0f}, 0.00001f));
    EXPECT_FALSE(Compare(Vector1{1.0f}, Vector1{2.0f}, 0.00001f));
}

TEST(VectorTest, CompareVector2)
{
    EXPECT_TRUE(Compare(Vector2{1.0f, 0.0f}, Vector2{1.0f, 0.0000001f}, 0.00001f));
    EXPECT_FALSE(Compare(Vector2{1.0f, 0.0f}, Vector2{1.0f, 0.0001f}, 0.00001f));
}

TEST(VectorTest, CompareVector3)
{
    EXPECT_TRUE(Compare(Vector3{1.0f, 0.0f, 2.0f}, Vector3{1.0f, 0.0f, 2.0f}, 0.00001f));
    EXPECT_FALSE(Compare(Vector3{1.0f, 0.0f, 2.0f}, Vector3{1.0f, 0.0f, -2.0f}, 0.00001f));
}

TEST(VectorTest, CompareVector4)
{
    EXPECT_TRUE(Compare(Vector4{1.0f, 0.0f, 2.0f, -5.0f}, Vector4{1.0f, 0.0f, 2.0f, -5.0f}, 0.00001f));
    EXPECT_FALSE(Compare(Vector4{1.0f, 0.0f, 2.0f, -5.0f}, Vector4{1.0f, 0.1f, 2.0f, -5.0f}, 0.00001f));
}

TEST(PointTest, ConstructPoint1FromVector1)
{
    const Point1 v{Vector1{1.0f}};
    EXPECT_EQ(1.0f, v.x);
}

TEST(PointTest, ConstructPoint2FromVector2)
{
    const Point2 v{Vector2{1.0f, 2.0f}};
    EXPECT_EQ(1.0f, v.x);
    EXPECT_EQ(2.0f, v.y);
}

TEST(PointTest, ConstructPoint3FromVector3)
{
    const Point3 v{Vector3{1.0f, 2.0f, 3.0f}};
    EXPECT_EQ(1.0f, v.x);
    EXPECT_EQ(2.0f, v.y);
    EXPECT_EQ(3.0f, v.z);
}

TEST(PointTest, ConstructPoint4FromVector4)
{
    const Point4 v{Vector4{1.0f, 2.0f, 3.0f, 4.0f}};
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
    EXPECT_THAT(2.0f * v1, ApproxEq(Vector1{4.0f}));
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
    EXPECT_THAT(2.0f * v1, ApproxEq(Vector2{6.0f, 4.0f}));
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
    EXPECT_THAT(2.0f * v1, ApproxEq(Vector3{6.0f, 4.0f, 2.0f}));
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
    EXPECT_THAT(2.0f * a, ApproxEq(Vector4{10.0f, 12.0f, 14.0f, 16.0f}));
    EXPECT_THAT(a / 2.0f, ApproxEq(Vector4{2.5f, 3.0f, 3.5f, 4.0f}));
    EXPECT_THAT(-b, ApproxEq(Vector4{-1.0f, -2.0f, -3.0f, -4.0f}));
}

TEST(VectorTest, OperationsScale3)
{
    const Vector3 v{3.0f, 2.0f, 1.0f};
    const Point3 p{4.0f, -2.0f, 3.0f};
    const Scale3 s{2.0f, 1.0f, 10.0f};

    EXPECT_THAT(v * s, ApproxEq(Vector3{6.0f, 2.0f, 10.0f}));
    EXPECT_THAT(p * s, ApproxEq(Point3{8.0f, -2.0f, 30.0f}));
    EXPECT_THAT(s * v, ApproxEq(Vector3{6.0f, 2.0f, 10.0f}));
    EXPECT_THAT(s * p, ApproxEq(Point3{8.0f, -2.0f, 30.0f}));
    EXPECT_THAT(s * s, ApproxEq(Scale3{4.0f, 1.0f, 100.0f}));
    EXPECT_THAT(v / s, ApproxEq(Vector3{1.5f, 2.0f, 0.1f}));
    EXPECT_THAT(p / s, ApproxEq(Point3{2.0f, -2.0f, 0.3f}));
    EXPECT_THAT(s / s, ApproxEq(Scale3{1.0f, 1.0f, 1.0f}));
    EXPECT_THAT(s * 2.0f, ApproxEq(Scale3{4.0f, 2.0f, 20.0f}));
    EXPECT_THAT(s / 2.0f, ApproxEq(Scale3{1.0f, 0.5f, 5.0f}));
    EXPECT_THAT(2.0f * s, ApproxEq(Scale3{4.0f, 2.0f, 20.0f}));
    EXPECT_THAT(1.0f / s, ApproxEq(Scale3{0.5f, 1.0f, 0.1f}));
}

TEST(VectorTest, InPlaceOperationsScale3)
{
    const Vector3 v1{3.0f, 2.0f, 1.0f};
    const Point3 p1{4.0f, -2.0f, 3.0f};
    const Scale3 s1{2.0f, 1.0f, 10.0f};

    Vector3 v = v1;
    v *= s1;
    EXPECT_THAT(v, ApproxEq(Vector3{6.0f, 2.0f, 10.0f}));

    Point3 p = p1;
    p *= s1;
    EXPECT_THAT(p, ApproxEq(Point3{8.0f, -2.0f, 30.0f}));

    Scale3 s = s1;
    s *= s;
    EXPECT_THAT(s, ApproxEq(Scale3{4.0f, 1.0f, 100.0f}));

    v = v1;
    v /= s1;
    EXPECT_THAT(v, ApproxEq(Vector3{1.5f, 2.0f, 0.1f}));

    p = p1;
    p /= s1;
    EXPECT_THAT(p, ApproxEq(Point3{2.0f, -2.0f, 0.3f}));

    s = s1;
    s /= s1;
    EXPECT_THAT(s, ApproxEq(Scale3{1.0f, 1.0f, 1.0f}));

    s = s1;
    s *= 2.0f;
    EXPECT_THAT(s, ApproxEq(Scale3{4.0f, 2.0f, 20.0f}));

    s = s1;
    s /= 2.0f;
    EXPECT_THAT(s, ApproxEq(Scale3{1.0f, 0.5f, 5.0f}));
}

TEST(VectorTest, NormalizeVector1)
{
    EXPECT_THAT(Normalise(Vector1{-3.0f}), ApproxEq(Vector1{-1.0f}));
}

TEST(VectorTest, NormalizeVector2)
{
    EXPECT_THAT(Normalise(Vector2{3.0f, 4.0f}), ApproxEq(Vector2{0.6f, 0.8f}));
}

TEST(VectorTest, NormalizeVector3)
{
    EXPECT_THAT(Normalise(Vector3{2.0f, 1.0f, 2.0f}), ApproxEq(Vector3{2.0f / 3.0f, 1.0f / 3.0f, 2.0f / 3.0f}));
}

TEST(VectorTest, NormalizeVector4)
{
    EXPECT_THAT(Normalise(Vector4{2.0f, -2.0f, 2.0f, -2.0f}), ApproxEq(Vector4{0.5f, -0.5f, 0.5f, -0.5f}));
}

TEST(VectorTest, MagnitudeVector1)
{
    EXPECT_THAT(Magnitude(Vector1{3.0f}), ApproxEq(3.0f));
}

TEST(VectorTest, MagnitudeVector2)
{
    EXPECT_THAT(Magnitude(Vector2{3.0f, 4.0f}), ApproxEq(5.0f));
}

TEST(VectorTest, MagnitudeVector3)
{
    EXPECT_THAT(Magnitude(Vector3{2.0f, 1.0f, 2.0f}), ApproxEq(3.0f));
}

TEST(VectorTest, MagnitudeVector4)
{
    EXPECT_THAT(Magnitude(Vector4{2.0f, -2.0f, 2.0f, -2.0f}), ApproxEq(4.0f));
}

TEST(VectorTest, MagnitudeSqVector1)
{
    EXPECT_THAT(MagnitudeSq(Vector1{3.0f}), ApproxEq(9.0f));
}

TEST(VectorTest, MagnitudeSqVector2)
{
    EXPECT_THAT(MagnitudeSq(Vector2{3.0f, 4.0f}), ApproxEq(25.0f));
}

TEST(VectorTest, MagnitudeSqVector3)
{
    EXPECT_THAT(MagnitudeSq(Vector3{2.0f, 1.0f, 2.0f}), ApproxEq(9.0f));
}

TEST(VectorTest, MagnitudeSqVector4)
{
    EXPECT_THAT(MagnitudeSq(Vector4{2.0f, -2.0f, 2.0f, -2.0f}), ApproxEq(16.0f));
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

TEST(VectorTest, Lerp)
{
    const auto a = 2.0f;
    const auto b = 5.0f;

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(2.0f));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(3.5f));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(5.0f));
}

TEST(VectorTest, LerpVector1)
{
    const auto a = Vector1{2};
    const auto b = Vector1{5};

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(Vector1{2.0f}));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(Vector1{3.5f}));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(Vector1{5.0f}));
}

TEST(VectorTest, LerpVector2)
{
    const auto a = Vector2{3, 0};
    const auto b = Vector2{5, -2};

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(Vector2{3.0f, 0.0f}));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(Vector2{4.0f, -1.0f}));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(Vector2{5.0f, -2.0f}));
}

TEST(VectorTest, LerpVector3)
{
    const auto a = Vector3{0, 3, 0};
    const auto b = Vector3{-5, 1, 2};

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(Vector3{0.0f, 3.0f, 0.0f}));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(Vector3{-2.5f, 2.0f, 1.0f}));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(Vector3{-5.0f, 1.0f, 2.0f}));
}

TEST(VectorTest, LerpVector4)
{
    const auto a = Vector4{1, 2, 3, 4};
    const auto b = Vector4{0, -1, 0, 1};

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(Vector4{0.5f, 0.5f, 1.5f, 2.5f}));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(Vector4{0.0f, -1.0f, 0.0f, 1.0f}));
}

TEST(VectorTest, LerpPoint1)
{
    const auto a = Point1{2};
    const auto b = Point1{5};

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(Point1{2.0f}));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(Point1{3.5f}));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(Point1{5.0f}));
}

TEST(VectorTest, LerpPoint2)
{
    const auto a = Point2{3, 0};
    const auto b = Point2{5, -2};

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(Point2{3.0f, 0.0f}));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(Point2{4.0f, -1.0f}));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(Point2{5.0f, -2.0f}));
}

TEST(VectorTest, LerpPoint3)
{
    const auto a = Point3{0, 3, 0};
    const auto b = Point3{-5, 1, 2};

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(Point3{0.0f, 3.0f, 0.0f}));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(Point3{-2.5f, 2.0f, 1.0f}));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(Point3{-5.0f, 1.0f, 2.0f}));
}

TEST(VectorTest, LerpPoint4)
{
    const auto a = Point4{1, 2, 3, 4};
    const auto b = Point4{0, -1, 0, 1};

    EXPECT_THAT(Lerp(a, b, 0.0f), ApproxEq(Point4{1.0f, 2.0f, 3.0f, 4.0f}));
    EXPECT_THAT(Lerp(a, b, 0.5f), ApproxEq(Point4{0.5f, 0.5f, 1.5f, 2.5f}));
    EXPECT_THAT(Lerp(a, b, 1.0f), ApproxEq(Point4{0.0f, -1.0f, 0.0f, 1.0f}));
}

TEST(VectorTest, Midpoint)
{
    const auto a = 2.0f;
    const auto b = 5.0f;

    EXPECT_THAT(Midpoint(a, b), ApproxEq(3.5f));
}

TEST(VectorTest, MidpointVector1)
{
    const auto a = Vector1{2};
    const auto b = Vector1{5};

    EXPECT_THAT(Midpoint(a, b), ApproxEq(Vector1{3.5f}));
}

TEST(VectorTest, MidpointVector2)
{
    const auto a = Vector2{3, 0};
    const auto b = Vector2{5, -2};

    EXPECT_THAT(Midpoint(a, b), ApproxEq(Vector2{4.0f, -1.0f}));
}

TEST(VectorTest, MidpointVector3)
{
    const auto a = Vector3{0, 3, 0};
    const auto b = Vector3{-5, 1, 2};

    EXPECT_THAT(Midpoint(a, b), ApproxEq(Vector3{-2.5f, 2.0f, 1.0f}));
}

TEST(VectorTest, MidpointVector4)
{
    const auto a = Vector4{1, 2, 3, 4};
    const auto b = Vector4{0, -1, 0, 1};

    EXPECT_THAT(Midpoint(a, b), ApproxEq(Vector4{0.5f, 0.5f, 1.5f, 2.5f}));
}

TEST(VectorTest, MidpointPoint1)
{
    const auto a = Point1{2};
    const auto b = Point1{5};

    EXPECT_THAT(Midpoint(a, b), ApproxEq(Point1{3.5f}));
}

TEST(VectorTest, MidpointPoint2)
{
    const auto a = Point2{3, 0};
    const auto b = Point2{5, -2};

    EXPECT_THAT(Midpoint(a, b), ApproxEq(Point2{4.0f, -1.0f}));
}

TEST(VectorTest, MidpointPoint3)
{
    const auto a = Point3{0, 3, 0};
    const auto b = Point3{-5, 1, 2};

    EXPECT_THAT(Midpoint(a, b), ApproxEq(Point3{-2.5f, 2.0f, 1.0f}));
}

TEST(VectorTest, MidpointPoint4)
{
    const auto a = Point4{1, 2, 3, 4};
    const auto b = Point4{0, -1, 0, 1};

    EXPECT_THAT(Midpoint(a, b), ApproxEq(Point4{0.5f, 0.5f, 1.5f, 2.5f}));
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
