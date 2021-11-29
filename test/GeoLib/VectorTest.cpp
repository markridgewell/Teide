
#include "GeoLib/Vector.h"

#include "TestUtils.h"

#include <gtest/gtest.h>

using namespace Geo;
using namespace testing;

namespace
{
TEST(VectorTest, ConstructVector2)
{
	Geo::Vector2 v{1.0f, 2.0f};
	EXPECT_EQ(1.0f, v.x);
	EXPECT_EQ(2.0f, v.y);
}

TEST(MatrixTest, DotVector4)
{
	const auto a = Vector4{1, 2, 3, 4};
	const auto b = Vector4{5, 6, 7, 8};

	EXPECT_THAT(Dot(a, b), Eq(70));
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
