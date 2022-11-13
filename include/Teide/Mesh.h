
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/ForwardDeclare.h"

#include <vector>

namespace Teide
{

struct MeshData
{
    ResourceLifetime lifetime = ResourceLifetime::Permanent;
    std::vector<byte> vertexData;
    std::vector<byte> indexData;
    uint32 vertexCount = 0;
};

class Mesh
{
public:
    virtual ~Mesh() = default;

    virtual BufferPtr GetVertexBuffer() const = 0;
    virtual BufferPtr GetIndexBuffer() const = 0;
    virtual uint32 GetVertexCount() const = 0;
    virtual uint32 GetIndexCount() const = 0;
};

} // namespace Teide
