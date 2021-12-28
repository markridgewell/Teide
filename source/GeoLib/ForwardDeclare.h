
#pragma once

#include <cstdint>

namespace Geo
{
using Extent = size_t;

template <class T, Extent N, class Tag>
struct Vector;

struct VectorTag;
using Vector2 = Vector<float, 2, VectorTag>;
using Vector3 = Vector<float, 3, VectorTag>;
using Vector4 = Vector<float, 4, VectorTag>;

struct PointTag;
using Point2 = Vector<float, 2, PointTag>;
using Point3 = Vector<float, 3, PointTag>;
using Point4 = Vector<float, 4, PointTag>;

struct ScaleTag;
using Scale2 = Vector<float, 2, ScaleTag>;
using Scale3 = Vector<float, 3, ScaleTag>;
using Scale4 = Vector<float, 4, ScaleTag>;
} // namespace Geo
