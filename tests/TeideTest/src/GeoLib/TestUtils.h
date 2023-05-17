
#pragma once

#include "GeoLib/Angle.h"
#include "GeoLib/Matrix.h"
#include "GeoLib/Vector.h"

#include <fmt/core.h>
#include <gmock/gmock.h>

#include <concepts>
#include <ostream>

namespace Geo
{
template <class T, Extent M, Extent N>
std::ostream& operator<<(std::ostream& os, const Matrix<T, M, N>& m)
{
    os << '(';
    for (Extent i = 0; i < M - 1; i++)
    {
        os << m[i] << ", ";
    }
    return os << m[M - 1] << ')';
}
} // namespace Geo

template <class T>
constexpr T Epsilon = std::numeric_limits<T>::epsilon();

MATCHER_P(ApproxEq, v, fmt::format("{} [approximately]", testing::PrintToString(v)))
{
    (void)result_listener;
    return Geo::Compare(arg, v, Epsilon<float> * 10.0f);
}
