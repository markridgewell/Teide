
#pragma once

#include "Vulkan.h"

#include "Teide/ForwardDeclare.h"

namespace Teide
{

class CommandBuffer
{
public:
    explicit CommandBuffer(vk::UniqueCommandBuffer commandBuffer);

    void AddTexture(const TexturePtr& texture);
    void AddBuffer(const BufferPtr& buffer);
    void AddParameterBlock(const ParameterBlockPtr& parameterBlock);

    void TakeOwnership(vk::UniqueBuffer buffer);
    void Reset();
    
    std::string_view GetDebugName() const;
    void SetDebugName(std::string_view name);

    template <class... Args>
    std::string SetDebugName(fmt::format_string<Args...> fmt [[maybe_unused]], Args&&... args [[maybe_unused]])
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

    std::unordered_set<TexturePtr> m_referencedTextures;
    std::unordered_set<BufferPtr> m_referencedBuffers;
    std::unordered_set<ParameterBlockPtr> m_referencedParameterBlocks;
    std::vector<vk::UniqueBuffer> m_ownedBuffers;

    std::string m_debugName = "Unnamed";
};

} // namespace Teide
