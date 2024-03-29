
#pragma once

#include "Teide/Mesh.h"
#include "Teide/Vulkan.h"
#include "Teide/VulkanBuffer.h"

namespace Teide
{

class MemoryAllocator;

struct VulkanMesh : public Mesh
{
    VertexLayout vertexLayout;
    std::shared_ptr<VulkanBuffer> vertexBuffer;
    std::shared_ptr<VulkanBuffer> indexBuffer;
    uint32 vertexCount = 0;
    uint32 indexCount = 0;
    vk::IndexType indexType = vk::IndexType::eUint16;
    Geo::Box3 aabb;

    const VertexLayout& GetVertexLayout() const override { return vertexLayout; }
    BufferPtr GetVertexBuffer() const override { return vertexBuffer; }
    BufferPtr GetIndexBuffer() const override { return indexBuffer; }
    uint32 GetVertexCount() const override { return vertexCount; }
    uint32 GetIndexCount() const override { return indexCount; }
    Geo::Box3 GetBoundingBox() const override { return aabb; }
};

template <>
struct VulkanImpl<Mesh>
{
    using type = VulkanMesh;
};

} // namespace Teide
