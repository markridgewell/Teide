
#include "GeoLib/Vector.h"
#include <gtest/gtest.h>

TEST(VectorTest, Test)
{
	Geo::Vector2 v {1.0f, 2.0f};
	EXPECT_EQ(1.0f, v.x);
	EXPECT_EQ(2.0f, v.y);
}
