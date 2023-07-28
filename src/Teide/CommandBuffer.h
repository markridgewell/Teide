
#pragma once

#include "Vulkan.h"

#include "Teide/ForwardDeclare.h"

namespace Teide
{

class CommandBuffer
{
public:
    explicit CommandBuffer(vk::UniqueCommandBuffer commandBuffer);

    void AddReference(const TexturePtr& p);
    void AddReference(const BufferPtr& p);
    void AddReference(const MeshPtr& p);
    void AddReference(const ParameterBlockPtr& p);
    void AddReference(const PipelinePtr& p);

    void TakeOwnership(vk::UniqueBuffer buffer);
    void Reset();

    vk::UniqueCommandBuffer& Get() { return m_cmdBuffer; }

    operator vk::CommandBuffer();
    vk::CommandBuffer& operator*();
    vk::CommandBuffer* operator->();

private:
    CommandBuffer() = default;

    vk::UniqueCommandBuffer m_cmdBuffer;

    std::unordered_set<TexturePtr> m_referencedTextures;
    std::unordered_set<BufferPtr> m_referencedBuffers;
    std::unordered_set<MeshPtr> m_referencedMeshes;
    std::unordered_set<ParameterBlockPtr> m_referencedParameterBlocks;
    std::unordered_set<PipelinePtr> m_referencedPipelines;
    std::vector<vk::UniqueBuffer> m_ownedBuffers;
};

} // namespace Teide
