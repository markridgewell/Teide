
#include "CommandBuffer.h"

CommandBuffer::CommandBuffer(vk::UniqueCommandBuffer commandBuffer) : m_cmdBuffer(std::move(commandBuffer))
{}

void CommandBuffer::AddTexture(TexturePtr texture)
{
	m_referencedTextures.insert(texture);
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

OneShotCommandBuffer::OneShotCommandBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue queue) :
    m_queue{queue}
{
	const auto allocInfo = vk::CommandBufferAllocateInfo{
	    .commandPool = commandPool,
	    .level = vk::CommandBufferLevel::ePrimary,
	    .commandBufferCount = 1,
	};
	auto cmdBuffers = device.allocateCommandBuffersUnique(allocInfo);
	m_cmdBuffer = std::move(cmdBuffers.front());

	m_cmdBuffer->begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

OneShotCommandBuffer::~OneShotCommandBuffer()
{
	m_cmdBuffer->end();

	const auto submitInfo = vk::SubmitInfo{}.setCommandBuffers(m_cmdBuffer.get());
	m_queue.submit(submitInfo);
	m_queue.waitIdle();

	m_cmdBuffer.reset();
}
