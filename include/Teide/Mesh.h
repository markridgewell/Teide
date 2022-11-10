
#pragma once

#include "Teide/ForwardDeclare.h"

#include <cstddef>
#include <vector>

namespace Teide
{

struct MeshData
{
	ResourceLifetime lifetime = ResourceLifetime::Permanent;
	std::vector<std::byte> vertexData;
	std::vector<std::byte> indexData;
	std::uint32_t vertexCount = 0;
};

class Mesh
{
public:
	virtual ~Mesh() = default;

	virtual BufferPtr GetVertexBuffer() const = 0;
	virtual BufferPtr GetIndexBuffer() const = 0;
	virtual std::uint32_t GetVertexCount() const = 0;
	virtual std::uint32_t GetIndexCount() const = 0;
};

} // namespace Teide
