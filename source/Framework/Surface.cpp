
#include "Framework/Surface.h"

#include "Framework/Internal/CommandBuffer.h"

#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <span>

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

vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width = 0, height = 0;
		SDL_Vulkan_GetDrawableSize(window, &width, &height);

		const vk::Extent2D windowExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

		const vk::Extent2D actualExtent
		    = {std::clamp(windowExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		       std::clamp(windowExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};

		return actualExtent;
	}
}

vk::UniqueSwapchainKHR CreateSwapchain(
    vk::PhysicalDevice physicalDevice, std::span<const uint32_t> queueFamilyIndices, vk::SurfaceKHR surface,
    vk::SurfaceFormatKHR surfaceFormat, vk::Extent2D surfaceExtent, vk::Device device, vk::SwapchainKHR oldSwapchain)
{
	const auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

	const auto mode = ChoosePresentMode(physicalDevice.getSurfacePresentModesKHR(surface));

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0)
	{
		imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount);
	}

	std::vector<uint32_t> queueFamiliesCopy;
	std::ranges::copy(queueFamilyIndices, std::back_inserter(queueFamiliesCopy));
	std::ranges::sort(queueFamiliesCopy);
	const auto uniqueQueueFamilies = std::ranges::unique(queueFamiliesCopy);
	assert(!uniqueQueueFamilies.empty());
	const auto sharingMode = uniqueQueueFamilies.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;

	const auto createInfo = vk::SwapchainCreateInfoKHR{
	    .surface = surface,
	    .minImageCount = imageCount,
	    .imageFormat = surfaceFormat.format,
	    .imageColorSpace = surfaceFormat.colorSpace,
	    .imageExtent = surfaceExtent,
	    .imageArrayLayers = 1,
	    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
	    .imageSharingMode = sharingMode,
	    .queueFamilyIndexCount = size32(uniqueQueueFamilies),
	    .pQueueFamilyIndices = data(uniqueQueueFamilies),
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
		const auto createInfo = vk::ImageViewCreateInfo{
		    .image = images[i],
		    .viewType = vk::ImageViewType::e2D,
		    .format = swapchainFormat,
		    .subresourceRange
		    = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1,},};

		imageViews[i] = device.createImageViewUnique(createInfo, s_allocator);
		SetDebugName(imageViews[i], "SwapchainImageView[{}]", i);
	}

	return imageViews;
}

vk::Format FindSupportedFormat(
    vk::PhysicalDevice physicalDevice, std::span<const vk::Format> candidates, vk::ImageTiling tiling,
    vk::FormatFeatureFlags features)
{
	for (const auto format : candidates)
	{
		const vk::FormatProperties props = physicalDevice.getFormatProperties(format);
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

vk::UniqueRenderPass
CreateRenderPass(vk::Device device, vk::Format surfaceFormat, vk::Format depthFormat, vk::SampleCountFlagBits sampleCount)
{
	const bool msaa = sampleCount != vk::SampleCountFlagBits::e1;

	const auto attachments = std::array{
	    vk::AttachmentDescription{
	        // color
	        .format = surfaceFormat,
	        .samples = sampleCount,
	        .loadOp = vk::AttachmentLoadOp::eClear,
	        .storeOp = msaa ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore,
	        .initialLayout = vk::ImageLayout::eUndefined,
	        .finalLayout = msaa ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR,
	    },
	    vk::AttachmentDescription{
	        // depth
	        .format = depthFormat,
	        .samples = sampleCount,
	        .loadOp = vk::AttachmentLoadOp::eClear,
	        .storeOp = vk::AttachmentStoreOp::eDontCare,
	        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
	        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
	        .initialLayout = vk::ImageLayout::eUndefined,
	        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
	    },
	    vk::AttachmentDescription{
	        // color resolved
	        .format = surfaceFormat,
	        .samples = vk::SampleCountFlagBits::e1,
	        .loadOp = vk::AttachmentLoadOp::eDontCare,
	        .storeOp = vk::AttachmentStoreOp::eStore,
	        .initialLayout = vk::ImageLayout::eUndefined,
	        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
	    },
	};
	const uint32_t numAttachments = msaa ? 3 : 2;

	const auto colorAttachmentRef = vk::AttachmentReference{
	    .attachment = 0,
	    .layout = vk::ImageLayout::eColorAttachmentOptimal,
	};

	const auto depthAttachmentRef = vk::AttachmentReference{
	    .attachment = 1,
	    .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
	};

	const auto colorResolveAttachmentRef = vk::AttachmentReference{
	    .attachment = 2,
	    .layout = vk::ImageLayout::eColorAttachmentOptimal,
	};

	const auto subpass = vk::SubpassDescription{
	    .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
	    .colorAttachmentCount = 1,
	    .pColorAttachments = &colorAttachmentRef,
	    .pResolveAttachments = msaa ? &colorResolveAttachmentRef : nullptr,
	    .pDepthStencilAttachment = &depthAttachmentRef,
	};

	const auto dependency = vk::SubpassDependency{
	    .srcSubpass = VK_SUBPASS_EXTERNAL,
	    .dstSubpass = 0,
	    .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
	    .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
	    .srcAccessMask = vk::AccessFlags{},
	    .dstAccessMask = vk ::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
	};

	const auto createInfo = vk::RenderPassCreateInfo{
	    .attachmentCount = numAttachments,
	    .pAttachments = data(attachments),
	    .subpassCount = 1,
	    .pSubpasses = &subpass,
	    .dependencyCount = 1,
	    .pDependencies = &dependency,
	};

	return device.createRenderPassUnique(createInfo, s_allocator);
}

std::vector<vk::UniqueFramebuffer> CreateFramebuffers(
    std::span<const vk::UniqueImageView> imageViews, vk::ImageView colorImageView, vk::ImageView depthImageView,
    vk::RenderPass renderPass, vk::Extent2D surfaceExtent, vk::Device device)
{
	auto framebuffers = std::vector<vk::UniqueFramebuffer>(imageViews.size());

	for (size_t i = 0; i < imageViews.size(); i++)
	{
		const auto msaaAttachments = std::array{colorImageView, depthImageView, imageViews[i].get()};
		const auto nonMsaattachments = std::array{imageViews[i].get(), depthImageView};

		const auto attachments = colorImageView ? std::span<const vk::ImageView>(msaaAttachments)
		                                        : std::span<const vk::ImageView>(nonMsaattachments);

		const auto createInfo = vk::FramebufferCreateInfo{
		    .renderPass = renderPass,
		    .attachmentCount = size32(attachments),
		    .pAttachments = data(attachments),
		    .width = surfaceExtent.width,
		    .height = surfaceExtent.height,
		    .layers = 1,
		};

		framebuffers[i] = device.createFramebufferUnique(createInfo, s_allocator);
		SetDebugName(framebuffers[i], "SwapchainFramebuffer[{}]", i);
	}

	return framebuffers;
}

} // namespace

Surface::Surface(
    SDL_Window* window, vk::UniqueSurfaceKHR surface, vk::Device device, vk::PhysicalDevice physicalDevice,
    std::vector<uint32_t> queueFamilyIndices, vk::CommandPool commandPool, vk::Queue queue, bool multisampled) :
    m_device{device},
    m_physicalDevice{physicalDevice},
    m_queueFamilyIndices{std::move(queueFamilyIndices)},
    m_commandPool{commandPool},
    m_queue{queue},
    m_window{window},
    m_surface{std::move(surface)},
    m_swapchainAllocator(device, physicalDevice)
{
	std::ranges::generate(m_imageAvailable, [=] { return CreateSemaphore(device); });

	if (multisampled)
	{
		const auto deviceLimits = physicalDevice.getProperties().limits;
		const auto supportedSampleCounts
		    = deviceLimits.framebufferColorSampleCounts & deviceLimits.framebufferDepthSampleCounts;
		m_msaaSampleCount = vk::SampleCountFlagBits{std::bit_floor(static_cast<uint32_t>(supportedSampleCounts))};
	}

	CreateSwapchainAndImages();
}

void Surface::OnResize()
{
	RecreateSwapchain();
}

std::optional<SurfaceImage> Surface::AcquireNextImage(vk::Fence fence)
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

	const auto ret = SurfaceImage{
	    .swapchain = m_swapchain.get(),
	    .imageIndex = imageIndex,
	    .imageAvailable = semaphore,
	    .renderPass = m_renderPass.get(),
	    .framebuffer = m_swapchainFramebuffers[imageIndex].get(),
	    .extent = m_surfaceExtent,
	};

	return ret;
}

vk::Semaphore Surface::GetNextSemaphore()
{
	const auto index = m_nextSemaphoreIndex;
	m_nextSemaphoreIndex = (m_nextSemaphoreIndex + 1) % MaxFramesInFlight;
	return m_imageAvailable[index].get();
}

void Surface::CreateColorBuffer(vk::Format format, vk::CommandBuffer cmdBuffer)
{
	// Create image
	const auto imageInfo = vk::ImageCreateInfo{
	    .imageType = vk::ImageType::e2D,
	    .format = format,
	    .extent = {m_surfaceExtent.width, m_surfaceExtent.height, 1},
	    .mipLevels = 1,
	    .arrayLayers = 1,
	    .samples = m_msaaSampleCount,
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

	TransitionImageLayout(
	    cmdBuffer, m_colorImage.get(), imageInfo.format, 1, vk::ImageLayout::eUndefined,
	    vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
	    vk::PipelineStageFlagBits::eColorAttachmentOutput);

	// Create image view
	const auto viewInfo = vk::ImageViewCreateInfo{
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

void Surface::CreateDepthBuffer(vk::CommandBuffer cmdBuffer)
{
	const auto formatCandidates
	    = std::array{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint};
	m_depthFormat = FindSupportedFormat(
	    m_physicalDevice, formatCandidates, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	// Create image
	const auto imageInfo = vk::ImageCreateInfo{
	    .imageType = vk::ImageType::e2D,
	    .format = m_depthFormat,
	    .extent = {m_surfaceExtent.width, m_surfaceExtent.height, 1},
	    .mipLevels = 1,
	    .arrayLayers = 1,
	    .samples = m_msaaSampleCount,
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

	TransitionImageLayout(
	    cmdBuffer, m_depthImage.get(), imageInfo.format, 1, vk::ImageLayout::eUndefined,
	    vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
	    vk::PipelineStageFlagBits::eEarlyFragmentTests);

	// Create image view
	const auto viewInfo = vk::ImageViewCreateInfo{
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

void Surface::CreateSwapchainAndImages()
{
	auto cmdBuffer = OneShotCommandBuffer(m_device, m_commandPool, m_queue);

	const auto surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
	const auto surfaceFormat = ChooseSurfaceFormat(m_physicalDevice.getSurfaceFormatsKHR(m_surface.get()));
	m_colorFormat = surfaceFormat.format;
	m_surfaceExtent = ChooseSwapExtent(surfaceCapabilities, m_window);
	m_swapchain = CreateSwapchain(
	    m_physicalDevice, m_queueFamilyIndices, m_surface.get(), surfaceFormat, m_surfaceExtent, m_device,
	    m_swapchain.get());
	m_swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain.get());
	m_swapchainImageViews = CreateSwapchainImageViews(surfaceFormat.format, m_swapchainImages, m_device);
	m_imagesInFlight.resize(m_swapchainImages.size());

	if (m_msaaSampleCount != vk::SampleCountFlagBits::e1)
	{
		CreateColorBuffer(surfaceFormat.format, cmdBuffer);
	}
	CreateDepthBuffer(cmdBuffer);

	m_renderPass = CreateRenderPass(m_device, surfaceFormat.format, m_depthFormat, m_msaaSampleCount);
	SetDebugName(m_renderPass, "SwapchainRenderPass");
	m_swapchainFramebuffers = CreateFramebuffers(
	    m_swapchainImageViews, m_colorImageView.get(), m_depthImageView.get(), m_renderPass.get(), m_surfaceExtent, m_device);
}

void Surface::RecreateSwapchain()
{
	m_swapchainAllocator.DeallocateAll();

	m_device.waitIdle();
	CreateSwapchainAndImages();
}
