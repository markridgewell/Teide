
#pragma once

#include "Vulkan.h"

#include "Teide/ForwardDeclare.h"
#include "Teide/ParameterBlock.h"
#include "Teide/Texture.h"

namespace Teide
{

class CommandBuffer
{
public:
    explicit CommandBuffer(vk::UniqueCommandBuffer commandBuffer);

    void AddReference(const Texture& p);
    void AddReference(const BufferPtr& p);
    void AddReference(const MeshPtr& p);
    void AddReference(const ParameterBlock& p);
    void AddReference(const PipelinePtr& p);

    void TakeOwnership(vk::UniqueBuffer buffer);
    void TakeOwnership(vma::UniqueAllocation allocation);
    void Reset();

    std::string_view GetDebugName() const;
    void SetDebugName(std::string_view name);

    template <class... Args>
    void SetDebugName(fmt::format_string<Args...> fmt [[maybe_unused]], Args&&... args [[maybe_unused]])
    {
        if constexpr (IsDebugBuild)
        {
            SetDebugName(fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...)));
        }
    }

    vk::UniqueCommandBuffer& Get() { return m_cmdBuffer; }

    operator vk::CommandBuffer();
    vk::CommandBuffer& operator*();
    vk::CommandBuffer* operator->();

private:
    CommandBuffer() = default;

    vk::UniqueCommandBuffer m_cmdBuffer;

    std::unordered_set<Texture> m_referencedTextures;
    std::unordered_set<BufferPtr> m_referencedBuffers;
    std::unordered_set<MeshPtr> m_referencedMeshes;
    std::unordered_set<ParameterBlock> m_referencedParameterBlocks;
    std::unordered_set<PipelinePtr> m_referencedPipelines;
    std::vector<vk::UniqueBuffer> m_ownedBuffers;
    std::vector<vma::UniqueAllocation> m_ownedAllocations;

    std::string m_debugName = "Unnamed";
};

} // namespace Teide
