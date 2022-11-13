
#include "CommandBuffer.h"

namespace Teide
{

CommandBuffer::CommandBuffer(vk::UniqueCommandBuffer commandBuffer) : m_cmdBuffer(std::move(commandBuffer))
{}

void CommandBuffer::AddTexture(TexturePtr texture)
{
    m_referencedTextures.insert(texture);
}

void CommandBuffer::AddBuffer(BufferPtr buffer)
{
    m_referencedBuffers.insert(buffer);
}

void CommandBuffer::AddParameterBlock(ParameterBlockPtr parameterBlock)
{
    m_referencedParameterBlocks.insert(parameterBlock);
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
