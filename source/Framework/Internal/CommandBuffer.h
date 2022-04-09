
#include "Framework/ForwardDeclare.h"
#include "Framework/Internal/Vulkan.h"

class CommandBuffer
{
public:
	explicit CommandBuffer(vk::UniqueCommandBuffer commandBuffer);

	void AddTexture(TexturePtr texture);
	void AddParameterBlock(ParameterBlockPtr parameterBlock);

	void TakeOwnership(vk::UniqueBuffer buffer);
	void Reset();

	vk::UniqueCommandBuffer& Get() { return m_cmdBuffer; }

	operator vk::CommandBuffer() const;

protected:
	CommandBuffer() = default;

	vk::UniqueCommandBuffer m_cmdBuffer;

	std::unordered_set<TexturePtr> m_referencedTextures;
	std::unordered_set<ParameterBlockPtr> m_referencedParameterBlocks;
	std::vector<vk::UniqueBuffer> m_ownedBuffers;
};

class OneShotCommandBuffer : public CommandBuffer
{
public:
	explicit OneShotCommandBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue queue);
	~OneShotCommandBuffer();

	OneShotCommandBuffer(const OneShotCommandBuffer&) = delete;
	OneShotCommandBuffer(OneShotCommandBuffer&&) = delete;
	OneShotCommandBuffer& operator=(const OneShotCommandBuffer&) = delete;
	OneShotCommandBuffer& operator=(OneShotCommandBuffer&&) = delete;

private:
	vk::Queue m_queue;
};
