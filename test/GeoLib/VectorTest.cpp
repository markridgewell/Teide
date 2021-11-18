
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
