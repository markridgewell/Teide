
#pragma once

#include "GeoLib/Box.h"
#include "Teide/AbstractBase.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/MeshData.h"

#include <vector>

namespace Teide
{

class Mesh : AbstractBase
{
public:
    virtual const VertexLayout& GetVertexLayout() const = 0;
    virtual BufferPtr GetVertexBuffer() const = 0;
    virtual BufferPtr GetIndexBuffer() const = 0;
    virtual uint32 GetVertexCount() const = 0;
    virtual uint32 GetIndexCount() const = 0;
    virtual Geo::Box3 GetBoundingBox() const = 0;
};

} // namespace Teide
