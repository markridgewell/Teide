
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
	std::uint32_t vertexCount = 0;
	std::uint32_t indexCount = 0;
	vk::IndexType indexType = vk::IndexType::eUint16;

	BufferPtr GetVertexBuffer() const override { return vertexBuffer; }
	BufferPtr GetIndexBuffer() const override { return indexBuffer; }
	std::uint32_t GetVertexCount() const override { return vertexCount; }
	std::uint32_t GetIndexCount() const override { return indexCount; }
};

template <>
struct VulkanImpl<Mesh>
{
	using type = VulkanMesh;
};

} // namespace Teide
