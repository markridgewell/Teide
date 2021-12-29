
#pragma once

#include "Framework/Buffer.h"
#include "Framework/MemoryAllocator.h"
#include "Framework/Pipeline.h"
#include "Framework/Renderer.h"
#include "Framework/Surface.h"
#include "Framework/Vulkan.h"

#include <optional>
#include <unordered_map>

struct Shader;
struct ShaderData;
struct Texture;
struct TextureData;

struct DescriptorSet
{
	std::vector<vk::DescriptorSet> sets;
};

struct UniformBuffer
{
	std::array<Buffer, MaxFramesInFlight> buffers;
	vk::DeviceSize size = 0;

	void SetData(int currentFrame, BytesView data) { buffers[currentFrame % MaxFramesInFlight].SetData(data); }
};

class GraphicsDevice
{
public:
	explicit GraphicsDevice(SDL_Window* window);

	Surface CreateSurface(SDL_Window* window, bool multisampled);
	Renderer CreateRenderer();

	Buffer CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, const char* name);
	Buffer CreateBufferWithData(BytesView data, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, const char* name);

	UniformBuffer CreateUniformBuffer(vk::DeviceSize bufferSize, const char* name);

	Shader CreateShader(const ShaderData& data, const char* name);

	Texture CreateTexture(const TextureData& data, const char* name);
	RenderableTexture CreateRenderableTexture(const TextureData& data, const char* name);

	PipelinePtr CreatePipeline(
	    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates, const Surface& surface);
	PipelinePtr CreatePipeline(
	    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates,
	    const RenderableTexture& texture);

	DescriptorSet CreateDescriptorSet(
	    vk::DescriptorSetLayout layout, const UniformBuffer& uniformBuffer, std::span<const Texture* const> textures = {});

	vk::UniqueFramebuffer CreateFramebuffer(const vk::FramebufferCreateInfo& createInfo) const;
	vk::UniqueRenderPass CreateRenderPass(const vk::RenderPassCreateInfo& createInfo) const;

	const vk::PhysicalDeviceProperties GetProperties() const { return m_physicalDevice.getProperties(); }

	void WaitIdle() const { m_device->waitIdle(); }

	// TODO no direct access to vk device
	vk::Device get() { return m_device.get(); }

private:
	auto OneShotCommands() { return OneShotCommandBuffer(m_device.get(), m_setupCommandPool.get(), m_graphicsQueue); }

	Texture CreateTextureImpl(
	    const TextureData& data, vk::ImageUsageFlags usage, OneShotCommandBuffer& cmdBuffer, const char* debugName);

	vk::UniqueInstance m_instance;
	vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_device;

	uint32_t m_graphicsQueueFamily;
	uint32_t m_presentQueueFamily;
	vk::Queue m_graphicsQueue;
	vk::UniqueDescriptorPool m_descriptorPool;
	vk::UniqueCommandPool m_setupCommandPool;

	std::optional<MemoryAllocator> m_allocator;

	std::unordered_map<SDL_Window*, vk::UniqueSurfaceKHR> m_pendingWindowSurfaces;
};
