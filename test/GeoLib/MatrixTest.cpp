
#include "GeoLib/Matrix.h"

#include <gtest/gtest.h>

namespace Geo
{
template <class T, int N, class Tag>
std::ostream& operator<<(std::ostream& os, const Vector<T, N, Tag>& v)
{
	os << '(';
	for (int i = 0; i < N - 1; i++)
	{
		os << v[i] << ", ";
	}
	return os << v[N - 1] << ')';
}

TEST(MatrixTest, MultiplyMatrix4Matrix4)
{
	const Matrix4 v1{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
	const Matrix4 v2{17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
	const Matrix4 expected1{250, 260, 270, 280, 618, 644, 670, 696, 986, 1028, 1070, 1112, 1354, 1412, 1470, 1528};
	const Matrix4 expected2{538, 612, 686, 760, 650, 740, 830, 920, 762, 868, 974, 1080, 874, 996, 1118, 1240};

	EXPECT_EQ(expected1, v1 * v2);
	EXPECT_EQ(expected2, v2 * v1);
}

TEST(MatrixTest, MultiplyMatrix4Vector4)
{
	EXPECT_EQ(Vector4(1, 2, 3, 1), Matrix4::Identity() * Vector4(1, 2, 3, 1));
	//EXPECT_EQ(Vector4(1, -3, 2, 1), Matrix4::RotationX(90.0_deg) * Vector4(1, 2, 3, 1));

	const auto translation = Matrix4{
	    {1, 0, 0, 10},
	    {0, 1, 0, 50},
	    {0, 0, 1, 20},
	    {0, 0, 0, 1},
	};
	EXPECT_EQ(Vector4(11, 52, 23, 1), translation * Vector4(1, 2, 3, 1));

	const auto scaling = Matrix4{
	    {10, 0, 0, 0},
	    {0, 2, 0, 0},
	    {0, 0, 100, 0},
	    {0, 0, 0, 1},
	};
	EXPECT_EQ(Vector4(10, 4, 300, 1), scaling * Vector4(1, 2, 3, 1));

	EXPECT_EQ(Vector4(110, 104, 2300, 1), (scaling * translation) * Vector4(1, 2, 3, 1));
	EXPECT_EQ(Vector4(110, 104, 2300, 1), scaling * (translation * Vector4(1, 2, 3, 1)));
	EXPECT_EQ(Vector4(20, 54, 320, 1), (translation * scaling) * Vector4(1, 2, 3, 1));
	EXPECT_EQ(Vector4(20, 54, 320, 1), translation * (scaling * Vector4(1, 2, 3, 1)));
}

} // namespace Geo
