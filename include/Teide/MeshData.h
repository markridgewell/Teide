
#pragma once

#include "GeoLib/Box.h"
#include "Teide/BasicTypes.h"
#include "Teide/Format.h"
#include "Teide/ForwardDeclare.h"

#include <string>
#include <vector>

namespace Teide
{

enum class VertexClass
{
    PerVertex,
    PerInstance,
};

enum class PrimitiveTopology
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    LineListAdj,
    LineStripAdj,
    TriangleListAdj,
    TriangleStripAdj,
};

struct VertexBufferBinding
{
    uint32 binding = 0;
    uint32 stride = 0;
    VertexClass vertexClass = VertexClass::PerVertex;
};

struct VertexAttribute
{
    std::string name;
    Format format = Format::Float4;
    uint32 bufferIndex = 0;
    uint32 offset = 0;
    uint32 instanceDataStepRate = 0;
};

struct VertexLayout
{
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    std::vector<VertexBufferBinding> bufferBindings;
    std::vector<VertexAttribute> attributes;
};

struct MeshData
{
    ResourceLifetime lifetime = ResourceLifetime::Permanent;
    VertexLayout vertexLayout;
    std::vector<byte> vertexData;
    std::vector<byte> indexData;
    uint32 vertexCount = 0;
    Geo::Box3 aabb;
};

} // namespace Teide
