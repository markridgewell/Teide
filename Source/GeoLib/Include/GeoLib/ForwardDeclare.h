
#pragma once

#include <cstddef>
#include <cstdint>

namespace Geo
{
using Extent = std::size_t;

template <class T, Extent N, class Tag>
struct Vector;

struct VectorTag;
using Vector2 = Vector<float, 2, VectorTag>;
using Vector3 = Vector<float, 3, VectorTag>;
using Vector4 = Vector<float, 4, VectorTag>;
using Vector2i = Vector<std::int32_t, 2, VectorTag>;
using Vector3i = Vector<std::int32_t, 3, VectorTag>;
using Vector4i = Vector<std::int32_t, 4, VectorTag>;

struct PointTag;
using Point2 = Vector<float, 2, PointTag>;
using Point3 = Vector<float, 3, PointTag>;
using Point4 = Vector<float, 4, PointTag>;
using Point2i = Vector<std::int32_t, 2, PointTag>;
using Point3i = Vector<std::int32_t, 3, PointTag>;
using Point4i = Vector<std::int32_t, 4, PointTag>;

struct SizeTag;
using Size2 = Vector<float, 2, SizeTag>;
using Size3 = Vector<float, 3, SizeTag>;
using Size4 = Vector<float, 4, SizeTag>;
using Size2i = Vector<std::uint32_t, 2, SizeTag>;
using Size3i = Vector<std::uint32_t, 3, SizeTag>;
using Size4i = Vector<std::uint32_t, 4, SizeTag>;

struct ScaleTag;
using Scale2 = Vector<float, 2, ScaleTag>;
using Scale3 = Vector<float, 3, ScaleTag>;
using Scale4 = Vector<float, 4, ScaleTag>;

template <class T, Extent N>
struct Box;

using Box2 = Box<float, 2>;
using Box3 = Box<float, 3>;
using Box2i = Box<std::int32_t, 2>;
using Box3i = Box<std::int32_t, 3>;

} // namespace Geo
