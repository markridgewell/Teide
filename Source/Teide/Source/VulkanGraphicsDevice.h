
#pragma once

#include "CommandBuffer.h"
#include "MemoryAllocator.h"
#include "Teide/Buffer.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/ParameterBlock.h"
#include "Teide/Renderer.h"
#include "Teide/Surface.h"
#include "Vulkan.h"

#include <compare>
#include <map>
#include <optional>
#include <thread>
#include <type_traits>
#include <unordered_map>

class VulkanGraphicsDevice : public GraphicsDevice
{
public:
	explicit VulkanGraphicsDevice(SDL_Window* window = nullptr);

	~VulkanGraphicsDevice();

	SurfacePtr CreateSurface(SDL_Window* window, bool multisampled) override;
	RendererPtr CreateRenderer() override;

	BufferPtr CreateBuffer(const BufferData& data, const char* name) override;
	DynamicBufferPtr CreateDynamicBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags usage, const char* name) override;

	ShaderPtr CreateShader(const ShaderData& data, const char* name) override;

	TexturePtr CreateTexture(const TextureData& data, const char* name) override;
	DynamicTexturePtr CreateRenderableTexture(const TextureData& data, const char* name) override;

	PipelinePtr CreatePipeline(const PipelineData& data) override;

	ParameterBlockPtr CreateParameterBlock(const ParameterBlockData& data, const char* name) override;
	DynamicParameterBlockPtr CreateDynamicParameterBlock(const ParameterBlockData& data, const char* name) override;

	const vk::PhysicalDeviceProperties GetProperties() const { return m_physicalDevice.getProperties(); }

	// Internal
	vk::Device GetVulkanDevice() { return m_device.get(); }
	MemoryAllocator& GetMemoryAllocator() { return m_allocator.value(); }

	template <class T>
	auto& GetImpl(T& obj)
	{
		return dynamic_cast<const VulkanImpl<std::remove_const_t<T>>::type&>(obj);
	}

	auto OneShotCommands() { return OneShotCommandBuffer(m_device.get(), m_setupCommandPool.get(), m_graphicsQueue); }

	vk::RenderPass CreateRenderPass(const FramebufferLayout& framebufferLayout, const RenderPassInfo& renderPassInfo = {});
	vk::Framebuffer CreateFramebuffer(vk::RenderPass renderPass, vk::Extent2D size, std::vector<vk::ImageView> attachments);

private:
	struct RenderPassDesc
	{
		FramebufferLayout framebufferLayout;
		RenderPassInfo renderPassInfo;

		auto operator<=>(const RenderPassDesc&) const = default;
	};

	struct FramebufferDesc
	{
		vk::RenderPass renderPass;
		vk::Extent2D size;
		std::vector<vk::ImageView> attachments;

		auto operator<=>(const FramebufferDesc&) const = default;
	};

	std::vector<vk::UniqueDescriptorSet> CreateDescriptorSets(
	    vk::DescriptorSetLayout layout, size_t numSets, const DynamicBuffer& uniformBuffer,
	    std::span<const Texture* const> textures, const char* name);

	vk::DynamicLoader m_loader;
	vk::UniqueInstance m_instance;
	vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_device;

	std::mutex m_renderPassCacheMutex;
	std::map<RenderPassDesc, vk::UniqueRenderPass> m_renderPassCache;

	std::mutex m_framebufferCacheMutex;
	std::map<FramebufferDesc, vk::UniqueFramebuffer> m_framebufferCache;

	uint32_t m_graphicsQueueFamily;
	std::optional<uint32_t> m_presentQueueFamily;
	vk::Queue m_graphicsQueue;
	vk::UniqueDescriptorPool m_descriptorPool;
	vk::UniqueCommandPool m_setupCommandPool;

	std::optional<MemoryAllocator> m_allocator;

	std::unordered_map<SDL_Window*, vk::UniqueSurfaceKHR> m_pendingWindowSurfaces;
};
