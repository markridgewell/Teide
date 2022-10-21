
#pragma once

#include "Teide/ForwardDeclare.h"
#include "Vulkan.h"

namespace Teide
{

class CommandBuffer
{
public:
	explicit CommandBuffer(vk::UniqueCommandBuffer commandBuffer);

	void AddTexture(TexturePtr texture);
	void AddBuffer(BufferPtr buffer);
	void AddParameterBlock(ParameterBlockPtr parameterBlock);

	void TakeOwnership(vk::UniqueBuffer buffer);
	void Reset();

	vk::UniqueCommandBuffer& Get() { return m_cmdBuffer; }

	operator vk::CommandBuffer();
	vk::CommandBuffer& operator*();
	vk::CommandBuffer* operator->();

protected:
	CommandBuffer() = default;

	vk::UniqueCommandBuffer m_cmdBuffer;

	std::unordered_set<TexturePtr> m_referencedTextures;
	std::unordered_set<BufferPtr> m_referencedBuffers;
	std::unordered_set<ParameterBlockPtr> m_referencedParameterBlocks;
	std::vector<vk::UniqueBuffer> m_ownedBuffers;
};

} // namespace Teide
