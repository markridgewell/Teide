
#pragma once

#include "Framework/MemoryAllocator.h"
#include "Framework/Vulkan.h"

#include <array>
#include <optional>
#include <vector>

constexpr uint32_t MaxFramesInFlight = 2;

typedef struct SDL_Window SDL_Window;

struct SurfaceImage
{
	vk::SwapchainKHR swapchain;
	uint32_t imageIndex = 0;
	vk::Semaphore imageAvailable;
	vk::Framebuffer framebuffer;
	vk::Extent2D extent;
};

class Surface
{
public:
	Surface(
	    SDL_Window* window, vk::UniqueSurfaceKHR surface, vk::Device device, vk::PhysicalDevice physicalDevice,
	    std::vector<uint32_t> queueFamilyIndices, vk::CommandPool commandPool, vk::Queue queue, bool multisampled);

	vk::Extent2D GetExtent() const { return m_surfaceExtent; }
	vk::Format GetColorFormat() const { return m_colorFormat; }
	vk::Format GetDepthFormat() const { return m_depthFormat; }
	vk::SampleCountFlagBits GetSampleCount() const { return m_msaaSampleCount; }

	void OnResize();

	std::optional<SurfaceImage> AcquireNextImage(vk::Fence fence);

	// Internal
	vk::SurfaceKHR GetVulkanSurface() const { return m_surface.get(); }
	vk::RenderPass GetVulkanRenderPass() const { return m_renderPass.get(); }

private:
	vk::Semaphore GetNextSemaphore();
	void CreateColorBuffer(vk::Format format, vk::CommandBuffer cmdBuffer);
	void CreateDepthBuffer(vk::CommandBuffer cmdBuffer);
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
	vk::SampleCountFlagBits m_msaaSampleCount = vk::SampleCountFlagBits::e1;
	vk::Format m_colorFormat;
	vk::UniqueImage m_colorImage;
	MemoryAllocation m_colorMemory;
	vk::UniqueImageView m_colorImageView;
	vk::Format m_depthFormat;
	vk::UniqueImage m_depthImage;
	MemoryAllocation m_depthMemory;
	vk::UniqueImageView m_depthImageView;
	std::vector<vk::UniqueFramebuffer> m_swapchainFramebuffers;
	vk::UniqueRenderPass m_renderPass;

	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_imageAvailable;
	uint32_t m_nextSemaphoreIndex = 0;
	std::vector<vk::Fence> m_imagesInFlight;
};
