
#pragma once

#include "Framework/Buffer.h"
#include "Framework/ForwardDeclare.h"
#include "Framework/MemoryAllocator.h"
#include "Framework/ParameterBlock.h"
#include "Framework/Renderer.h"
#include "Framework/Surface.h"
#include "Framework/Vulkan.h"

#include <optional>
#include <unordered_map>

class GraphicsDevice
{
public:
	explicit GraphicsDevice(SDL_Window* window = nullptr);

	Surface CreateSurface(SDL_Window* window, bool multisampled);
	Renderer CreateRenderer();

	BufferPtr CreateBuffer(const BufferData& data, const char* name);
	DynamicBufferPtr CreateDynamicBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags usage, const char* name);

	ShaderPtr CreateShader(const ShaderData& data, const char* name);

	TexturePtr CreateTexture(const TextureData& data, const char* name);
	RenderableTexturePtr CreateRenderableTexture(const TextureData& data, const char* name);

	PipelinePtr CreatePipeline(
	    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates, const Surface& surface);
	PipelinePtr CreatePipeline(
	    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates,
	    const RenderableTexture& texture);

	ParameterBlockPtr CreateParameterBlock(const ParameterBlockData& data, const char* name);
	DynamicParameterBlockPtr CreateDynamicParameterBlock(const ParameterBlockData& data, const char* name);

	const vk::PhysicalDeviceProperties GetProperties() const { return m_physicalDevice.getProperties(); }

	void WaitIdle() const { m_device->waitIdle(); }

private:
	auto OneShotCommands() { return OneShotCommandBuffer(m_device.get(), m_setupCommandPool.get(), m_graphicsQueue); }

	Texture CreateTextureImpl(
	    const TextureData& data, vk::ImageUsageFlags usage, OneShotCommandBuffer& cmdBuffer, const char* debugName);

	std::vector<vk::UniqueDescriptorSet> CreateDescriptorSets(
	    vk::DescriptorSetLayout layout, size_t numSets, const DynamicBuffer& uniformBuffer,
	    std::span<const Texture* const> textures, const char* name);

	vk::UniqueInstance m_instance;
	vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_device;

	uint32_t m_graphicsQueueFamily;
	std::optional<uint32_t> m_presentQueueFamily;
	vk::Queue m_graphicsQueue;
	vk::UniqueDescriptorPool m_descriptorPool;
	vk::UniqueCommandPool m_setupCommandPool;

	std::optional<MemoryAllocator> m_allocator;

	std::unordered_map<SDL_Window*, vk::UniqueSurfaceKHR> m_pendingWindowSurfaces;
};
