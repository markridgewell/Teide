
#include "GeoLib/Angle.h"

#include "TestUtils.h"

#include <gtest/gtest.h>

using namespace Geo;
using namespace testing;

namespace
{
TEST(AngleTest, DefaultZero)
{
	EXPECT_THAT(Angle(), 0.0_rad);
}

TEST(AngleTest, Degrees)
{
	EXPECT_THAT(Angle::Degrees(90.0f).AsDegrees(), ApproxEq(90.0f));
}

TEST(AngleTest, Radians)
{
	EXPECT_THAT(Angle::Radians(Pi).AsRadians(), ApproxEq(Pi));
}

TEST(AngleTest, DegreesAsRadians)
{
	EXPECT_THAT(Angle::Degrees(180.0f).AsRadians(), ApproxEq(Pi));
}

TEST(AngleTest, RadiansAsDegrees)
{
	EXPECT_THAT(Angle::Radians(Pi).AsDegrees(), ApproxEq(180.0f));
}

TEST(AngleTest, Operations)
{
	EXPECT_THAT(1.0_rad + 2.0_rad, ApproxEq(3.0_rad));
	EXPECT_THAT(1.0_rad - 2.0_rad, ApproxEq(-1.0_rad));
	EXPECT_THAT(1.0_rad * 2.0f, ApproxEq(2.0_rad));
	EXPECT_THAT(1.0_rad / 2.0f, ApproxEq(0.5_rad));
	EXPECT_THAT(1.0 * 2.0_rad, ApproxEq(2.0_rad));
	EXPECT_THAT(-(2.0_rad), ApproxEq(Angle::Radians(-2.0)));
}

TEST(AngleTest, InPlaceOperations)
{
	Angle a = 1.0_rad;
	a += 2.0_rad;
	EXPECT_THAT(a, ApproxEq(3.0_rad));

	a = 1.0_rad;
	a -= 2.0_rad;
	EXPECT_THAT(a, ApproxEq(-1.0_rad));

	a = 1.0_rad;
	a *= 2.0f;
	EXPECT_THAT(a, ApproxEq(2.0_rad));

	a = 1.0_rad;
	a /= 2.0f;
	EXPECT_THAT(a, ApproxEq(0.5_rad));
}

TEST(AngleTest, Sin)
{
	EXPECT_THAT(Sin(0.0_deg), ApproxEq(0.0f));
	EXPECT_THAT(Sin(90.0_deg), ApproxEq(1.0f));
	EXPECT_THAT(Sin(180.0_deg), ApproxEq(0.0f));
	EXPECT_THAT(Sin(270.0_deg), ApproxEq(-1.0f));
}

TEST(AngleTest, Cos)
{
	EXPECT_THAT(Cos(0.0_deg), ApproxEq(1.0f));
	EXPECT_THAT(Cos(90.0_deg), ApproxEq(0.0f));
	EXPECT_THAT(Cos(180.0_deg), ApproxEq(-1.0f));
	EXPECT_THAT(Cos(270.0_deg), ApproxEq(0.0f));
}

TEST(AngleTest, Tan)
{
	EXPECT_THAT(Tan(-45.0_deg), ApproxEq(-1.0f));
	EXPECT_THAT(Tan(0.0_deg), ApproxEq(0.0f));
	EXPECT_THAT(Tan(45.0_deg), ApproxEq(1.0f));
}

} // namespace

#ifdef _MSC_VER
namespace Geo
{
// Explicit instantiations to ensure correct code coverage
template AngleT<float>;
template AngleT<double>;

template float Sin<float>(AngleT<float>) noexcept;
template float Cos<float>(AngleT<float>) noexcept;
template float Tan<float>(AngleT<float>) noexcept;
} // namespace Geo
#endif
