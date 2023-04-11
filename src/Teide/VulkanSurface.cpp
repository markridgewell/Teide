
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
    static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

    vk::SurfaceFormatKHR ChooseSurfaceFormat(std::span<const vk::SurfaceFormatKHR> availableFormats)
    {
        const auto preferredFormat = vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};

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
        else
        {
            int width = 0, height = 0;
            SDL_Vulkan_GetDrawableSize(window, &width, &height);

            const Geo::Size2i windowExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            const Geo::Size2i actualExtent
                = {std::clamp(windowExtent.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                   std::clamp(windowExtent.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};

            return actualExtent;
        }
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
            .imageExtent = {surfaceExtent.x, surfaceExtent.y},
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
    SDL_Window* window, vk::UniqueSurfaceKHR surface, vk::Device device, vk::PhysicalDevice physicalDevice,
    std::vector<uint32> queueFamilyIndices, vk::CommandPool commandPool, vk::Queue queue, bool multisampled) :
    m_device{device},
    m_physicalDevice{physicalDevice},
    m_queueFamilyIndices{std::move(queueFamilyIndices)},
    m_commandPool{commandPool},
    m_queue{queue},
    m_window{window},
    m_surface{std::move(surface)},
    m_swapchainAllocator(device, physicalDevice)
{
    std::ranges::generate(m_imageAvailable, [=] { return device.createSemaphoreUnique({}, s_allocator); });

    if (multisampled)
    {
        const auto deviceLimits = physicalDevice.getProperties().limits;
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
    assert(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

    // Acquire an image from the swap chain
    const auto semaphore = GetNextSemaphore();
    const auto [result, imageIndex] = m_device.acquireNextImageKHR(m_swapchain.get(), timeout, semaphore);
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        RecreateSwapchain();
        return std::nullopt;
    }
    else if (result == vk::Result::eSuboptimalKHR)
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
        assert(waitResult2 == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout
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
            .layout = {
                .colorFormat = m_colorFormat,
                .depthStencilFormat = m_depthFormat,
                .sampleCount = m_msaaSampleCount,
            },
            .size = m_surfaceExtent,
        },
        .prePresentCommandBuffer = m_transitionToPresentSrc[imageIndex].get(),
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
    const vk::ImageCreateInfo imageInfo = {
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {m_surfaceExtent.x, m_surfaceExtent.y, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits{m_msaaSampleCount},
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    m_colorImage = m_device.createImageUnique(imageInfo, s_allocator);
    SetDebugName(m_colorImage, "ColorImage");
    const auto allocation = m_swapchainAllocator.Allocate(
        m_device.getImageMemoryRequirements(m_colorImage.get()), vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_device.bindImageMemory(m_colorImage.get(), allocation.memory, allocation.offset);

    // Create image view
    const vk::ImageViewCreateInfo viewInfo = {
        .image = m_colorImage.get(),
        .viewType = vk::ImageViewType::e2D,
        .format = imageInfo.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    m_colorImageView = m_device.createImageViewUnique(viewInfo, s_allocator);
    SetDebugName(m_colorImageView, "ColorImageView");
}

void VulkanSurface::CreateDepthBuffer()
{
    const auto formatCandidates = std::array{Format::Depth32, Format::Depth32Stencil8, Format::Depth24Stencil8};
    m_depthFormat = FindSupportedFormat(
        m_physicalDevice, formatCandidates, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    // Create image
    const vk::ImageCreateInfo imageInfo = {
        .imageType = vk::ImageType::e2D,
        .format = ToVulkan(m_depthFormat),
        .extent = {m_surfaceExtent.x, m_surfaceExtent.y, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits{m_msaaSampleCount},
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    m_depthImage = m_device.createImageUnique(imageInfo, s_allocator);
    SetDebugName(m_depthImage, "DepthImage");
    const auto allocation = m_swapchainAllocator.Allocate(
        m_device.getImageMemoryRequirements(m_depthImage.get()), vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_device.bindImageMemory(m_depthImage.get(), allocation.memory, allocation.offset);

    // Create image view
    const vk::ImageViewCreateInfo viewInfo = {
        .image = m_depthImage.get(),
        .viewType = vk::ImageViewType::e2D,
        .format = imageInfo.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eDepth,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    m_depthImageView = m_device.createImageViewUnique(viewInfo, s_allocator);
    SetDebugName(m_depthImageView, "DepthImageView");
}

void VulkanSurface::CreateSwapchainAndImages()
{
    const auto surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
    const auto surfaceFormat = ChooseSurfaceFormat(m_physicalDevice.getSurfaceFormatsKHR(m_surface.get()));
    m_colorFormat = FromVulkan(surfaceFormat.format);
    m_surfaceExtent = ChooseSwapExtent(surfaceCapabilities, m_window);
    m_swapchain = CreateSwapchain(
        m_physicalDevice, m_queueFamilyIndices, m_surface.get(), surfaceFormat, m_surfaceExtent, m_device,
        m_swapchain.get());
    m_swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain.get());
    m_swapchainImageViews = CreateSwapchainImageViews(surfaceFormat.format, m_swapchainImages, m_device);
    m_imagesInFlight.resize(m_swapchainImages.size());

    if (m_msaaSampleCount != 1)
    {
        CreateColorBuffer(surfaceFormat.format);
    }
    CreateDepthBuffer();

    const FramebufferLayout framebufferLayout = {
        .colorFormat = FromVulkan(surfaceFormat.format),
        .depthStencilFormat = m_depthFormat,
        .sampleCount = m_msaaSampleCount,
    };
    m_renderPass = CreateRenderPass(m_device, framebufferLayout);
    SetDebugName(m_renderPass, "SwapchainRenderPass");
    m_swapchainFramebuffers = CreateFramebuffers(
        m_swapchainImageViews, m_colorImageView.get(), m_depthImageView.get(), m_renderPass.get(), m_surfaceExtent, m_device);

    // Create command buffers for transitioning images to present source
    const vk::CommandBufferAllocateInfo cmdBufferAllocInfo = {
        .commandPool = m_commandPool,
        .commandBufferCount = size32(m_swapchainImages),
    };
    m_transitionToPresentSrc.clear();
    m_transitionToPresentSrc = m_device.allocateCommandBuffersUnique(cmdBufferAllocInfo);
    for (uint32 i = 0; i < size32(m_swapchainImages); i++)
    {
        const auto cmdBuffer = *m_transitionToPresentSrc[i];
        cmdBuffer.begin(vk::CommandBufferBeginInfo{});
        TransitionImageLayout(
            cmdBuffer, m_swapchainImages[i], Format::Unknown, 1, vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR, vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eTopOfPipe);
        cmdBuffer.end();
    }
}

void VulkanSurface::RecreateSwapchain()
{
    m_swapchainAllocator.DeallocateAll();

    m_device.waitIdle();
    CreateSwapchainAndImages();
}

} // namespace Teide
