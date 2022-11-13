
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Format.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/PipelineData.h"

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
    PrimitiveTopology topology;
    std::vector<VertexBufferBinding> bufferBindings;
    std::vector<VertexAttribute> attributes;
};

struct PipelineData
{
    ShaderPtr shader;
    VertexLayout vertexLayout;
    RenderStates renderStates;
    FramebufferLayout framebufferLayout;
};

class Pipeline
{
public:
    virtual ~Pipeline() = default;
};

} // namespace Teide
