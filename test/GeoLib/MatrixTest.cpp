
#include "GeoLib/Matrix.h"

#include <gtest/gtest.h>

TEST(MatrixTest, MultiplyMatrix4Matrix4)
{
	const Geo::Matrix4 v1{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
	const Geo::Matrix4 v2{17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
	const Geo::Matrix4 expected{250, 260, 270, 280, 618, 644, 670, 696, 986, 1028, 1070, 1112, 1354, 1412, 1470, 1528};

	EXPECT_EQ(expected, v1 * v2);
}
