
#pragma once

#include "MemoryAllocator.h"
#include "Teide/Surface.h"
#include "Vulkan.h"

#include <array>
#include <optional>
#include <vector>

constexpr uint32_t MaxFramesInFlight = 2;

typedef struct SDL_Window SDL_Window;

struct SurfaceImage
{
	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapchain;
	uint32_t imageIndex = 0;
	vk::Semaphore imageAvailable;
	vk::Image image;
	vk::Framebuffer framebuffer;
	vk::Extent2D extent;
	vk::CommandBuffer prePresentCommandBuffer;
};

class VulkanSurface : public Surface
{
public:
	VulkanSurface(
	    SDL_Window* window, vk::UniqueSurfaceKHR surface, vk::Device device, vk::PhysicalDevice physicalDevice,
	    std::vector<uint32_t> queueFamilyIndices, vk::CommandPool commandPool, vk::Queue queue, bool multisampled);

	vk::Extent2D GetExtent() const override { return m_surfaceExtent; }
	TextureFormat GetColorFormat() const override { return m_colorFormat; }
	TextureFormat GetDepthFormat() const override { return m_depthFormat; }
	vk::SampleCountFlagBits GetSampleCount() const override { return m_msaaSampleCount; }

	void OnResize() override;

	// Internal
	std::optional<SurfaceImage> AcquireNextImage(vk::Fence fence);

	vk::SurfaceKHR GetVulkanSurface() const { return m_surface.get(); }
	vk::RenderPass GetVulkanRenderPass() const { return m_renderPass.get(); }

private:
	vk::Semaphore GetNextSemaphore();
	void CreateColorBuffer(vk::Format format);
	void CreateDepthBuffer();
	void CreateSwapchainAndImages();
	void RecreateSwapchain();

	vk::Device m_device;
	vk::PhysicalDevice m_physicalDevice;
	std::vector<uint32_t> m_queueFamilyIndices;
	vk::CommandPool m_commandPool;
	vk::Queue m_queue;

	SDL_Window* m_window;
	vk::UniqueSurfaceKHR m_surface;
	vk::Extent2D m_surfaceExtent;
	MemoryAllocator m_swapchainAllocator;
	vk::UniqueSwapchainKHR m_swapchain;
	std::vector<vk::Image> m_swapchainImages;
	std::vector<vk::UniqueImageView> m_swapchainImageViews;
	std::vector<vk::UniqueCommandBuffer> m_transitionToPresentSrc;
	vk::SampleCountFlagBits m_msaaSampleCount = vk::SampleCountFlagBits::e1;
	TextureFormat m_colorFormat;
	vk::UniqueImage m_colorImage;
	MemoryAllocation m_colorMemory;
	vk::UniqueImageView m_colorImageView;
	TextureFormat m_depthFormat;
	vk::UniqueImage m_depthImage;
	MemoryAllocation m_depthMemory;
	vk::UniqueImageView m_depthImageView;
	std::vector<vk::UniqueFramebuffer> m_swapchainFramebuffers;
	vk::UniqueRenderPass m_renderPass;

	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_imageAvailable;
	uint32_t m_nextSemaphoreIndex = 0;
	std::vector<vk::Fence> m_imagesInFlight;
};

template <>
struct VulkanImpl<Surface>
{
	using type = VulkanSurface;
};
