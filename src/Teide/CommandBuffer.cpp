
#include "CommandBuffer.h"

namespace Teide
{

CommandBuffer::CommandBuffer(vk::UniqueCommandBuffer commandBuffer) : m_cmdBuffer(std::move(commandBuffer))
{}

void CommandBuffer::AddReference(const TexturePtr& p)
{
    m_referencedTextures.insert(p);
}

void CommandBuffer::AddReference(const BufferPtr& p)
{
    m_referencedBuffers.insert(p);
}

void CommandBuffer::AddReference(const MeshPtr& p)
{
    m_referencedMeshes.insert(p);
}

void CommandBuffer::AddReference(const ParameterBlockPtr& p)
{
    m_referencedParameterBlocks.insert(p);
}

void CommandBuffer::AddReference(const PipelinePtr& p)
{
    m_referencedPipelines.insert(p);
}

void CommandBuffer::TakeOwnership(vk::UniqueBuffer buffer)
{
    m_ownedBuffers.push_back(std::move(buffer));
}

void CommandBuffer::Reset()
{
    m_ownedBuffers.clear();
}

CommandBuffer::operator vk::CommandBuffer()
{
    assert(m_cmdBuffer);
    return m_cmdBuffer.get();
}

vk::CommandBuffer& CommandBuffer::operator*()
{
    assert(m_cmdBuffer);
    return m_cmdBuffer.get();
}

vk::CommandBuffer* CommandBuffer::operator->()
{
    assert(m_cmdBuffer);
    return &m_cmdBuffer.get();
}

} // namespace Teide
