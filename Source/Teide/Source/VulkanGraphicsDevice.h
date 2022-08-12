
#pragma once

#include "CommandBuffer.h"
#include "MemoryAllocator.h"
#include "Scheduler.h"
#include "Teide/GraphicsDevice.h"
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
	explicit VulkanGraphicsDevice(SDL_Window* window = nullptr, std::uint32_t numThreads = std::thread::hardware_concurrency());

	~VulkanGraphicsDevice();

	SurfacePtr CreateSurface(SDL_Window* window, bool multisampled) override;
	RendererPtr CreateRenderer() override;

	BufferPtr CreateBuffer(const BufferData& data, const char* name) override;

	ShaderPtr CreateShader(const ShaderData& data, const char* name) override;

	TexturePtr CreateTexture(const TextureData& data, const char* name) override;
	DynamicTexturePtr CreateRenderableTexture(const TextureData& data, const char* name) override;

	PipelinePtr CreatePipeline(const PipelineData& data) override;

	ParameterBlockPtr CreateParameterBlock(const ParameterBlockData& data, const char* name) override;

	const vk::PhysicalDeviceProperties GetProperties() const { return m_physicalDevice.getProperties(); }

	// Internal
	vk::Device GetVulkanDevice() { return m_device.get(); }
	MemoryAllocator& GetMemoryAllocator() { return m_allocator.value(); }
	Scheduler& GetScheduler() { return m_scheduler.value(); }

	template <class T>
	auto& GetImpl(T& obj)
	{
		return dynamic_cast<const VulkanImpl<std::remove_const_t<T>>::type&>(obj);
	}

	BufferPtr CreateBuffer(const BufferData& data, const char* name, CommandBuffer& cmdBuffer);
	TexturePtr CreateTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer);
	DynamicTexturePtr CreateRenderableTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer);
	ParameterBlockPtr CreateParameterBlock(
	    const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, std::uint32_t threadIndex);
	ParameterBlockPtr CreateParameterBlock(
	    const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, vk::DescriptorPool descriptorPool);

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

	vk::UniqueDescriptorSet CreateDescriptorSet(
	    vk::DescriptorPool pool, vk::DescriptorSetLayout layout, const Buffer* uniformBuffer,
	    std::span<const Texture* const> textures, const char* name);

	std::vector<vk::UniqueDescriptorSet> CreateDescriptorSets(
	    vk::DescriptorPool pool, vk::DescriptorSetLayout layout, size_t numSets, const Buffer* uniformBuffer,
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
	vk::UniqueDescriptorPool m_mainDescriptorPool;
	std::vector<vk::UniqueDescriptorPool> m_workerDescriptorPools;
	vk::UniqueCommandPool m_setupCommandPool;
	vk::UniqueCommandPool m_surfaceCommandPool;

	std::optional<MemoryAllocator> m_allocator;

	std::optional<Scheduler> m_scheduler;

	std::unordered_map<SDL_Window*, vk::UniqueSurfaceKHR> m_pendingWindowSurfaces;
};
