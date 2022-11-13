
#pragma once

#include "Teide/Mesh.h"
#include "Teide/Vulkan.h"
#include "Teide/VulkanBuffer.h"

namespace Teide
{

class MemoryAllocator;

struct VulkanMesh : public Mesh
{
	std::shared_ptr<VulkanBuffer> vertexBuffer;
	std::shared_ptr<VulkanBuffer> indexBuffer;
	uint32 vertexCount = 0;
	uint32 indexCount = 0;
	vk::IndexType indexType = vk::IndexType::eUint16;

	BufferPtr GetVertexBuffer() const override { return vertexBuffer; }
	BufferPtr GetIndexBuffer() const override { return indexBuffer; }
	uint32 GetVertexCount() const override { return vertexCount; }
	uint32 GetIndexCount() const override { return indexCount; }
};

template <>
struct VulkanImpl<Mesh>
{
	using type = VulkanMesh;
};

} // namespace Teide
