
#include "VulkanSurface.h"

#include "GeoLib/Vector.h"
#include "Teide/Pipeline.h"

#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <span>

namespace Teide
{

namespace
{
    const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

    vk::SurfaceFormatKHR ChooseSurfaceFormat(std::span<const vk::SurfaceFormatKHR> availableFormats)
    {
        const auto preferredFormat = vk::SurfaceFormatKHR{
            .format = vk::Format::eB8G8R8A8Srgb,
            .colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        };

        if (const auto it = std::ranges::find(availableFormats, preferredFormat); it != availableFormats.end())
        {
            return *it;
        }

        return availableFormats.front();
    }

    vk::PresentModeKHR ChoosePresentMode(std::span<const vk::PresentModeKHR> availableModes)
    {
        const auto preferredMode = vk::PresentModeKHR::eMailbox;

        if (const auto it = std::ranges::find(availableModes, preferredMode); it != availableModes.end())
        {
            return *it;
        }

        // FIFO mode is guaranteed to be supported
        return vk::PresentModeKHR::eFifo;
    }

    Geo::Size2i ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return {capabilities.currentExtent.width, capabilities.currentExtent.height};
        }


        int width = 0;
        int height = 0;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);

        const Geo::Size2i windowExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        const Geo::Size2i actualExtent
            = {std::clamp(windowExtent.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
               std::clamp(windowExtent.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};

        return actualExtent;
    }

    vk::UniqueSwapchainKHR CreateSwapchain(
        vk::PhysicalDevice physicalDevice, std::span<const uint32_t> queueFamilyIndices, vk::SurfaceKHR surface,
        vk::SurfaceFormatKHR surfaceFormat, Geo::Size2i surfaceExtent, vk::Device device, vk::SwapchainKHR oldSwapchain)
    {
        const auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

        const auto mode = ChoosePresentMode(physicalDevice.getSurfacePresentModesKHR(surface));

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0)
        {
            imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount);
        }

        const auto sharingMode = queueFamilyIndices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;

        const vk::SwapchainCreateInfoKHR createInfo = {
            .surface = surface,
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = {.width = surfaceExtent.x, .height = surfaceExtent.y},
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = sharingMode,
            .queueFamilyIndexCount = size32(queueFamilyIndices),
            .pQueueFamilyIndices = data(queueFamilyIndices),
            .preTransform = surfaceCapabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = mode,
            .clipped = true,
            .oldSwapchain = oldSwapchain,
        };

        return device.createSwapchainKHRUnique(createInfo, s_allocator);
    }

    std::vector<vk::UniqueImageView>
    CreateSwapchainImageViews(vk::Format swapchainFormat, std::span<const vk::Image> images, vk::Device device)
    {
        auto imageViews = std::vector<vk::UniqueImageView>(images.size());

        for (size_t i = 0; i < images.size(); i++)
        {
            const vk::ImageViewCreateInfo createInfo = {
                .image = images[i],
                .viewType = vk::ImageViewType::e2D,
                .format = swapchainFormat,
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            imageViews[i] = device.createImageViewUnique(createInfo, s_allocator);
            SetDebugName(imageViews[i], "SwapchainImageView[{}]", i);
        }

        return imageViews;
    }

    Format FindSupportedFormat(
        vk::PhysicalDevice physicalDevice, std::span<const Format> candidates, vk::ImageTiling tiling,
        vk::FormatFeatureFlags features)
    {
        for (const auto format : candidates)
        {
            const vk::FormatProperties props = physicalDevice.getFormatProperties(ToVulkan(format));
            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw VulkanError("Failed to find a suitable format");
    }

    std::vector<vk::UniqueFramebuffer> CreateFramebuffers(
        std::span<const vk::UniqueImageView> imageViews, vk::ImageView colorImageView, vk::ImageView depthImageView,
        vk::RenderPass renderPass, Geo::Size2i surfaceExtent, vk::Device device)
    {
        auto framebuffers = std::vector<vk::UniqueFramebuffer>(imageViews.size());

        for (size_t i = 0; i < imageViews.size(); i++)
        {
            const auto msaaAttachments = std::array{colorImageView, depthImageView, imageViews[i].get()};
            const auto nonMsaattachments = std::array{imageViews[i].get(), depthImageView};

            const auto attachments = colorImageView ? std::span<const vk::ImageView>(msaaAttachments)
                                                    : std::span<const vk::ImageView>(nonMsaattachments);

            const vk::FramebufferCreateInfo createInfo = {
                .renderPass = renderPass,
                .attachmentCount = size32(attachments),
                .pAttachments = data(attachments),
                .width = surfaceExtent.x,
                .height = surfaceExtent.y,
                .layers = 1,
            };

            framebuffers[i] = device.createFramebufferUnique(createInfo, s_allocator);
            SetDebugName(framebuffers[i], "SwapchainFramebuffer[{}]", i);
        }

        return framebuffers;
    }

} // namespace

VulkanSurface::VulkanSurface(
    SDL_Window* window, vk::UniqueSurfaceKHR surface, vk::Device device, const PhysicalDevice& physicalDevice,
    vma::Allocator allocator, vk::Queue queue, bool multisampled) :
    m_device{device},
    m_physicalDevice{physicalDevice},
    m_allocator{allocator},
    m_queue{queue},
    m_window{window},
    m_surface{std::move(surface)}
{
    std::ranges::generate(m_imageAvailable, [=] { return device.createSemaphoreUnique({}, s_allocator); });

    if (multisampled)
    {
        const auto deviceLimits = physicalDevice.properties.limits;
        const auto supportedSampleCounts
            = deviceLimits.framebufferColorSampleCounts & deviceLimits.framebufferDepthSampleCounts;
        m_msaaSampleCount = std::bit_floor(static_cast<uint32>(supportedSampleCounts));
    }

    CreateSwapchainAndImages();
}

void VulkanSurface::OnResize()
{
    RecreateSwapchain();
}

std::optional<SurfaceImage> VulkanSurface::AcquireNextImage(vk::Fence fence)
{
    constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

    [[maybe_unused]] const auto waitResult = m_device.waitForFences(fence, true, timeout);
    TEIDE_ASSERT(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

    // Acquire an image from the swap chain
    const auto semaphore = GetNextSemaphore();
    const auto [result, imageIndex] = m_device.acquireNextImageKHR(m_swapchain.get(), timeout, semaphore);
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        RecreateSwapchain();
        return std::nullopt;
    }
    if (result == vk::Result::eSuboptimalKHR)
    {
        spdlog::warn("Suboptimal swapchain image");
    }
    else if (result != vk::Result::eSuccess)
    {
        spdlog::error("Couldn't acquire swapchain image");
        return std::nullopt;
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (m_imagesInFlight[imageIndex])
    {
        [[maybe_unused]] const auto waitResult2 = m_device.waitForFences(m_imagesInFlight[imageIndex], true, timeout);
        TEIDE_ASSERT(waitResult2 == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout
    }
    // Mark the image as in flight
    m_imagesInFlight[imageIndex] = fence;

    const SurfaceImage ret = {
        .surface = m_surface.get(),
        .swapchain = m_swapchain.get(),
        .imageIndex = imageIndex,
        .imageAvailable = semaphore,
        .image = m_swapchainImages[imageIndex],
        .framebuffer = {
            .framebuffer = m_swapchainFramebuffers[imageIndex].get(),
            .layout = m_framebufferLayout,
            .size = m_surfaceExtent,
        },
    };

    return ret;
}

vk::Semaphore VulkanSurface::GetNextSemaphore()
{
    const auto index = m_nextSemaphoreIndex;
    m_nextSemaphoreIndex = (m_nextSemaphoreIndex + 1) % MaxFramesInFlight;
    return m_imageAvailable[index].get();
}

void VulkanSurface::CreateColorBuffer(vk::Format format)
{
    // Create image
    auto [image, allocation] = m_allocator.createImageUnique(
        vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = {.width = m_surfaceExtent.x, .height = m_surfaceExtent.y, .depth = 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits{m_msaaSampleCount},
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined,
        },
        vma::AllocationCreateInfo{
            .usage = vma::MemoryUsage::eAutoPreferDevice,
        });
    m_colorImage = vk::UniqueImage(image.release(), m_device);
    m_colorMemory = std::move(allocation);
    SetDebugName(m_colorImage, "ColorImage");

    // Create image view
    m_colorImageView = m_device.createImageViewUnique(
        vk::ImageViewCreateInfo {
            .image = m_colorImage.get(),
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        }, s_allocator);
    SetDebugName(m_colorImageView, "ColorImageView");
}

void VulkanSurface::CreateDepthBuffer()
{
    const auto format = ToVulkan(m_framebufferLayout.depthStencilFormat.value());

    // Create image
    auto [image, allocation] = m_allocator.createImageUnique(
        vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = {.width = m_surfaceExtent.x, .height = m_surfaceExtent.y, .depth = 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits{m_msaaSampleCount},
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined,
        },
        vma::AllocationCreateInfo{
            .usage = vma::MemoryUsage::eAutoPreferDevice,
        });
    m_depthImage = vk::UniqueImage(image.release(), m_device);
    m_depthMemory = std::move(allocation);
    SetDebugName(m_depthImage, "DepthImage");

    // Create image view
    m_depthImageView = m_device.createImageViewUnique(
        vk::ImageViewCreateInfo {
            .image = m_depthImage.get(),
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eDepth,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        }, s_allocator);
    SetDebugName(m_depthImageView, "DepthImageView");
}

void VulkanSurface::CreateSwapchainAndImages()
{
    const auto surfaceCapabilities = m_physicalDevice.physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
    const auto surfaceFormat = ChooseSurfaceFormat(m_physicalDevice.physicalDevice.getSurfaceFormatsKHR(m_surface.get()));
    const auto depthFormatCandidates = std::array{Format::Depth32, Format::Depth32Stencil8, Format::Depth24Stencil8};

    m_framebufferLayout = {
        .colorFormat = FromVulkan(surfaceFormat.format),
        .depthStencilFormat = FindSupportedFormat(
            m_physicalDevice.physicalDevice, depthFormatCandidates, vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment),
        .sampleCount = m_msaaSampleCount,
        .captureColor = true,
        .resolveColor = m_msaaSampleCount > 1,
    };

    m_surfaceExtent = ChooseSwapExtent(surfaceCapabilities, m_window);
    m_swapchain = CreateSwapchain(
        m_physicalDevice.physicalDevice, m_physicalDevice.queueFamilyIndices, m_surface.get(), surfaceFormat,
        m_surfaceExtent, m_device, m_swapchain.get());
    m_swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain.get());
    m_swapchainImageViews = CreateSwapchainImageViews(surfaceFormat.format, m_swapchainImages, m_device);
    m_imagesInFlight.resize(m_swapchainImages.size());

    if (m_msaaSampleCount != 1)
    {
        CreateColorBuffer(surfaceFormat.format);
    }
    CreateDepthBuffer();

    m_renderPass = CreateRenderPass(m_device, m_framebufferLayout, FramebufferUsage::PresentSrc);
    SetDebugName(m_renderPass, "SwapchainRenderPass");
    m_swapchainFramebuffers = CreateFramebuffers(
        m_swapchainImageViews, m_colorImageView.get(), m_depthImageView.get(), m_renderPass.get(), m_surfaceExtent, m_device);
}

void VulkanSurface::RecreateSwapchain()
{
    m_device.waitIdle();
    CreateSwapchainAndImages();
}

} // namespace Teide
