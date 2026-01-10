
#pragma once

#include "Vulkan.h"

#include "GeoLib/Vector.h"
#include "Teide/Surface.h"

#include <array>
#include <optional>
#include <vector>

namespace Teide
{

constexpr uint32_t MaxFramesInFlight = 2;

struct SurfaceImage
{
    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapchain;
    uint32_t imageIndex = 0;
    vk::Semaphore imageAvailable;
    vk::Image image;
    Framebuffer framebuffer;
};

class VulkanSurface : public Surface
{
public:
    VulkanSurface(
        Geo::Size2i extent, vk::UniqueSurfaceKHR surface, vk::Device device, const PhysicalDevice& physicalDevice,
        vma::Allocator allocator, vk::Queue queue, bool multisampled);

    Geo::Size2i GetExtent() const override { return m_surfaceExtent; }
    Format GetColorFormat() const override { return m_framebufferLayout.colorFormat.value(); }
    Format GetDepthFormat() const override { return m_framebufferLayout.depthStencilFormat.value(); }
    uint32 GetSampleCount() const override { return m_msaaSampleCount; }
    FramebufferLayout GetFramebufferLayout() const override { return m_framebufferLayout; }

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
    const PhysicalDevice& m_physicalDevice;
    vma::Allocator m_allocator;
    vk::Queue m_queue;

    vk::UniqueSurfaceKHR m_surface;
    Geo::Size2i m_surfaceExtent;
    vk::UniqueSwapchainKHR m_swapchain;
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::UniqueImageView> m_swapchainImageViews;
    FramebufferLayout m_framebufferLayout;
    uint32 m_msaaSampleCount = 1;
    vk::UniqueImage m_colorImage;
    vma::UniqueAllocation m_colorMemory;
    vk::UniqueImageView m_colorImageView;
    vk::UniqueImage m_depthImage;
    vma::UniqueAllocation m_depthMemory;
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

} // namespace Teide
