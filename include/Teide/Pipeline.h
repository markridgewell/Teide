
#pragma once

#include "Teide/Format.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/PipelineData.h"

#include <cstdint>
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
	std::uint32_t binding = 0;
	std::uint32_t stride = 0;
	VertexClass vertexClass = VertexClass::PerVertex;
};

struct VertexAttribute
{
	std::string name;
	Format format = Format::Float4;
	std::uint32_t bufferIndex = 0;
	std::uint32_t offset = 0;
	std::uint32_t instanceDataStepRate = 0;
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
