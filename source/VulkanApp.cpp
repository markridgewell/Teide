
#include "Definitions.h"
#include "ShaderCompiler.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	include <windows.h>
#endif

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace
{
constexpr std::string_view TriangleVertexShader = R"--(
#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
})--";

constexpr std::string_view TrianglePixelShader = R"--(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
})--";

constexpr auto ShaderLang = ShaderLanguage::Glsl;

class CustomError : public vk::Error, public std::exception
{
public:
	explicit CustomError(char const* what) : Error(), std::exception(what) {}

	virtual const char* what() const VULKAN_HPP_NOEXCEPT { return std::exception::what(); }
};

static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

vk::UniqueSurfaceKHR CreateVulkanSurface(SDL_Window* window, vk::Instance instance)
{
	VkSurfaceKHR surfaceTmp = {};
	if (!SDL_Vulkan_CreateSurface(window, instance, &surfaceTmp))
	{
		return {};
	}
	vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderDynamic> deleter(instance, s_allocator, VULKAN_HPP_DEFAULT_DISPATCHER);
	return vk::UniqueSurfaceKHR(surfaceTmp, deleter);
}

void EnableOptionalVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& availableLayers, const char* layerName)
{
	std::string_view layerNameSV = layerName;
	if (std::ranges::find_if(availableLayers, [layerNameSV](const auto& x) { return x.layerName == layerNameSV; })
	    != availableLayers.end())
	{
		spdlog::info("Enabling Vulkan layer {}", layerName);
		enabledLayers.push_back(layerName);
	}
	else
	{
		spdlog::warn("Vulkan layer {} not enabled!", layerName);
	}
}

void EnableRequiredVulkanExtension(
    std::vector<const char*>& enabledExtensions, const std::vector<vk::ExtensionProperties>& availableExtensions,
    const char* extensionName)
{
	std::string_view name = extensionName;
	if (std::ranges::find_if(availableExtensions, [name](const auto& x) { return x.extensionName == name; })
	    != availableExtensions.end())
	{
		spdlog::info("Enabling Vulkan extension {}", extensionName);
		enabledExtensions.push_back(extensionName);
	}
	else
	{
		throw vk::ExtensionNotPresentError(extensionName);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData)
{
	const char* prefix = "";
	switch (static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(messageType))
	{
		using enum vk::DebugUtilsMessageTypeFlagBitsEXT;

		case eGeneral:
			prefix = "";
			break;
		case eValidation:
			prefix = "[validation] ";
			break;
		case ePerformance:
			prefix = "[performance] ";
			break;
	}

	switch (vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity))
	{
		using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;

		case eVerbose:
		case eInfo:
			spdlog::info("{}{}", prefix, pCallbackData->pMessage);
			break;

		case eWarning:
			spdlog::warn("{}{}", prefix, pCallbackData->pMessage);
			break;

		case eError:
			spdlog::error("{}{}", prefix, pCallbackData->pMessage);
			break;
	}
	return VK_FALSE;
}

constexpr auto DebugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT{
    .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
    .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
    .pfnUserCallback = DebugCallback};

vk::UniqueInstance CreateInstance(SDL_Window* window)
{
	uint32_t extensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
	auto extensions = std::vector<const char*>(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());

	vk::ApplicationInfo applicationInfo{};

	const auto availableLayers = vk::enumerateInstanceLayerProperties();
	const auto availableExtensions = vk::enumerateInstanceExtensionProperties();

	std::vector<const char*> layers;
	if constexpr (IsDebugBuild)
	{
		EnableOptionalVulkanLayer(layers, availableLayers, "VK_LAYER_KHRONOS_validation");
		EnableRequiredVulkanExtension(extensions, availableExtensions, "VK_EXT_debug_utils");
	}

	const auto createInfo = vk::InstanceCreateInfo{
	    .pNext = &DebugCreateInfo,
	    .pApplicationInfo = &applicationInfo,
	    .enabledLayerCount = static_cast<uint32_t>(layers.size()),
	    .ppEnabledLayerNames = layers.data(),
	    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
	    .ppEnabledExtensionNames = extensions.data()};

	return vk::createInstanceUnique(createInfo, s_allocator);
}

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
	QueueFamilyIndices indices;

	const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indices.graphicsFamily = i;
		}

		if (physicalDevice.getSurfaceSupportKHR(i, surface))
		{
			indices.presentFamily = i;
		}

		if (indices.IsComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

vk::PhysicalDevice FindPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface, std::span<const char*> requiredExtensions)
{
	// Look for a discrete GPU
	auto physicalDevices = instance.enumeratePhysicalDevices();
	if (physicalDevices.empty())
	{
		throw CustomError("No GPU found!");
	}

	// Prefer discrete GPUs to integrated GPUs
	std::ranges::sort(physicalDevices, [](vk::PhysicalDevice a, vk::PhysicalDevice b) {
		return (a.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		    > (b.getProperties().deviceType == vk::PhysicalDeviceType::eIntegratedGpu);
	});

	// Find the first device that supports all the queue families we need
	const auto it = std::ranges::find_if(physicalDevices, [&](vk::PhysicalDevice device) {
		// Check that all required extensions are supported
		const auto supportedExtensions = device.enumerateDeviceExtensionProperties();
		const bool supportsAllExtensions = std::ranges::all_of(requiredExtensions, [&](std::string_view extensionName) {
			return std::ranges::count(supportedExtensions, extensionName, &vk::ExtensionProperties::extensionName) > 0;
		});

		if (!supportsAllExtensions)
		{
			return false;
		}

		// Check for adequate swap chain support
		if (device.getSurfaceFormatsKHR(surface).empty())
		{
			return false;
		}
		if (device.getSurfacePresentModesKHR(surface).empty())
		{
			return false;
		}

		// Check that all required queue families are supported
		const auto indices = FindQueueFamilies(device, surface);
		return indices.IsComplete();
	});
	if (it == physicalDevices.end())
	{
		throw CustomError("No suitable GPU found!");
	}

	vk::PhysicalDevice physicalDevice = *it;
	spdlog::info("Selected physical device: {}", physicalDevice.getProperties().deviceName);
	return physicalDevice;
}

vk::UniqueDevice
CreateDevice(vk::PhysicalDevice physicalDevice, std::span<const uint32_t> queueFamilyIndices, std::span<const char*> extensions)
{
	// Make a list of create infos for each unique queue we wish to create
	const float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (const uint32_t index : queueFamilyIndices)
	{
		if (std::ranges::count(queueCreateInfos, index, &vk::DeviceQueueCreateInfo::queueFamilyIndex) == 0)
		{
			queueCreateInfos.push_back({.queueFamilyIndex = index, .queueCount = 1, .pQueuePriorities = &queuePriority});
		}
	}

	const auto deviceFeatures = vk::PhysicalDeviceFeatures{};

	const auto availableLayers = physicalDevice.enumerateDeviceLayerProperties();
	const auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

	std::vector<const char*> layers;
	if constexpr (IsDebugBuild)
	{
		EnableOptionalVulkanLayer(layers, availableLayers, "VK_LAYER_KHRONOS_validation");
	}

	const auto deviceCreateInfo = vk::DeviceCreateInfo{
	    .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
	    .pQueueCreateInfos = queueCreateInfos.data(),
	    .enabledLayerCount = static_cast<uint32_t>(layers.size()),
	    .ppEnabledLayerNames = layers.data(),
	    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
	    .ppEnabledExtensionNames = extensions.data(),
	    .pEnabledFeatures = &deviceFeatures};

	return physicalDevice.createDeviceUnique(deviceCreateInfo, s_allocator);
}

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
    vk::SurfaceFormatKHR surfaceFormat, vk::Extent2D surfaceExtent, vk::Device device)
{
	const auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

	const auto mode = ChoosePresentMode(physicalDevice.getSurfacePresentModesKHR(surface));

	const uint32_t imageCount = std::min(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.maxImageCount);

	std::vector<uint32_t> queueFamiliesCopy;
	std::ranges::copy(queueFamilyIndices, std::back_inserter(queueFamiliesCopy));
	std::ranges::sort(queueFamiliesCopy);
	const auto uniqueQueueFamilies = std::ranges::unique(queueFamiliesCopy);
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
	    .queueFamilyIndexCount = static_cast<uint32_t>(uniqueQueueFamilies.size()),
	    .pQueueFamilyIndices = uniqueQueueFamilies.data(),
	    .preTransform = surfaceCapabilities.currentTransform,
	    .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
	    .presentMode = mode,
	    .clipped = true,
	};

	return device.createSwapchainKHRUnique(createInfo, s_allocator);
}

std::vector<vk::UniqueImageView>
CreateSwapchainImageViews(vk::Format swapchainFormat, std::span<const vk::Image> images, vk::Device device)
{
	auto imageViews = std::vector<vk::UniqueImageView>(images.size());

	for (const size_t i : std::views::iota(0u, images.size()))
	{
		const auto createInfo = vk::ImageViewCreateInfo{
		    .image = images[i],
		    .viewType = vk::ImageViewType::e2D,
		    .format = swapchainFormat,
		    .subresourceRange
		    = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1,},};

		imageViews[i] = device.createImageViewUnique(createInfo, s_allocator);
	}

	return imageViews;
}

vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device)
{
	const auto createInfo = vk::CommandPoolCreateInfo{
	    .queueFamilyIndex = queueFamilyIndex,
	};

	return device.createCommandPoolUnique(createInfo, s_allocator);
}

vk::UniqueSemaphore CreateSemaphore(vk::Device device)
{
	return device.createSemaphoreUnique(vk::SemaphoreCreateInfo{}, s_allocator);
}

vk::UniqueFence CreateFence(vk::Device device)
{
	return device.createFenceUnique(vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}, s_allocator);
}

vk::UniqueShaderModule
CreateShaderModule(std::string_view shaderSource, ShaderStage stage, ShaderLanguage language, vk::Device device)
{
	const std::vector<uint32_t> spirv = CompileShader(shaderSource, stage, language);

	const auto createInfo = vk::ShaderModuleCreateInfo{.codeSize = spirv.size() * sizeof(uint32_t), .pCode = spirv.data()};

	return device.createShaderModuleUnique(createInfo, s_allocator);
}

vk::UniquePipeline CreateGraphicsPipeline(
    vk::ShaderModule vertexShader, vk::ShaderModule pixelShader, vk::PipelineLayout layout, vk::RenderPass renderPass,
    vk::Extent2D surfaceExtent, vk::Device device)
{
	const auto shaderStages = std::array{
	    vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = vertexShader, .pName = "main"},
	    vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = pixelShader, .pName = "main"},
	};

	const auto vertexInput = vk::PipelineVertexInputStateCreateInfo{
	    .vertexBindingDescriptionCount = 0,
	    .pVertexBindingDescriptions = nullptr,
	    .vertexAttributeDescriptionCount = 0,
	    .pVertexAttributeDescriptions = nullptr,
	};

	const auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo{
	    .topology = vk::PrimitiveTopology::eTriangleList,
	    .primitiveRestartEnable = false,
	};

	const auto viewport = vk::Viewport{
	    .x = 0.0f,
	    .y = 0.0f,
	    .width = static_cast<float>(surfaceExtent.width),
	    .height = static_cast<float>(surfaceExtent.height),
	    .minDepth = 0.0f,
	    .maxDepth = 1.0f};

	const auto scissor = vk::Rect2D{
	    .offset = {0, 0},
	    .extent = surfaceExtent,
	};

	const auto viewportState = vk::PipelineViewportStateCreateInfo{
	    .viewportCount = 1,
	    .pViewports = &viewport,
	    .scissorCount = 1,
	    .pScissors = &scissor,
	};

	const auto rasterizationState = vk::PipelineRasterizationStateCreateInfo{
	    .depthClampEnable = false,
	    .rasterizerDiscardEnable = false,
	    .polygonMode = vk::PolygonMode::eFill,
	    .cullMode = vk::CullModeFlagBits::eBack,
	    .frontFace = vk::FrontFace::eClockwise,
	    .depthBiasEnable = false,
	    .depthBiasConstantFactor = 0.0f,
	    .depthBiasClamp = 0.0f,
	    .depthBiasSlopeFactor = 0.0f,
	    .lineWidth = 1.0f,
	};

	const auto multisampleState = vk::PipelineMultisampleStateCreateInfo{
	    .rasterizationSamples = vk::SampleCountFlagBits::e1,
	    .sampleShadingEnable = false,
	    .minSampleShading = 1.0f,
	    .pSampleMask = nullptr,
	    .alphaToCoverageEnable = false,
	    .alphaToOneEnable = false,
	};

	const auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{
	    .blendEnable = false,
	    .srcColorBlendFactor = vk::BlendFactor::eOne,
	    .dstColorBlendFactor = vk::BlendFactor::eZero,
	    .colorBlendOp = vk::BlendOp::eAdd,
	    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
	    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
	    .alphaBlendOp = vk::BlendOp::eAdd,
	    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
	        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
	};

	const auto colorBlendState = vk::PipelineColorBlendStateCreateInfo{
	    .logicOpEnable = false,
	    .attachmentCount = 1,
	    .pAttachments = &colorBlendAttachment,
	};

	const auto createInfo = vk::GraphicsPipelineCreateInfo{
	    .stageCount = static_cast<uint32_t>(shaderStages.size()),
	    .pStages = shaderStages.data(),
	    .pVertexInputState = &vertexInput,
	    .pInputAssemblyState = &inputAssembly,
	    .pViewportState = &viewportState,
	    .pRasterizationState = &rasterizationState,
	    .pMultisampleState = &multisampleState,
	    .pDepthStencilState = nullptr,
	    .pColorBlendState = &colorBlendState,
	    .pDynamicState = nullptr,
	    .layout = layout,
	    .renderPass = renderPass,
	    .subpass = 0,
	};

	auto [result, pipeline] = device.createGraphicsPipelineUnique(nullptr, createInfo, s_allocator);
	if (result != vk::Result::eSuccess)
	{
		throw CustomError("Couldn't create graphics pipeline");
	}
	return std::move(pipeline);
}

vk::UniquePipelineLayout CreateGraphicsPipelineLayout(vk::Device device)
{
	const auto createInfo = vk::PipelineLayoutCreateInfo{
	    .setLayoutCount = 0,
	    .pSetLayouts = nullptr,
	    .pushConstantRangeCount = 0,
	    .pPushConstantRanges = nullptr,
	};

	return device.createPipelineLayoutUnique(createInfo, s_allocator);
}

vk::UniqueRenderPass CreateRenderPass(vk::Device device, vk::Format surfaceFormat)
{
	const auto colorAttachment = vk::AttachmentDescription{
	    .format = surfaceFormat,
	    .samples = vk::SampleCountFlagBits::e1,
	    .loadOp = vk::AttachmentLoadOp::eClear,
	    .storeOp = vk::AttachmentStoreOp::eStore,
	    .initialLayout = vk::ImageLayout::eUndefined,
	    .finalLayout = vk::ImageLayout::ePresentSrcKHR,
	};

	const auto colorAttachmentRef = vk::AttachmentReference{
	    .attachment = 0,
	    .layout = vk::ImageLayout::eColorAttachmentOptimal,
	};

	const auto subpass = vk::SubpassDescription{
	    .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
	    .colorAttachmentCount = 1,
	    .pColorAttachments = &colorAttachmentRef,
	};

	const auto dependency = vk::SubpassDependency{
	    .srcSubpass = VK_SUBPASS_EXTERNAL,
	    .dstSubpass = 0,
	    .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
	    .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
	    .srcAccessMask = vk::AccessFlags{},
	    .dstAccessMask = vk ::AccessFlagBits::eColorAttachmentWrite,
	};

	const auto createInfo = vk::RenderPassCreateInfo{
	    .attachmentCount = 1,
	    .pAttachments = &colorAttachment,
	    .subpassCount = 1,
	    .pSubpasses = &subpass,
	    .dependencyCount = 1,
	    .pDependencies = &dependency,
	};

	return device.createRenderPassUnique(createInfo, s_allocator);
}

std::vector<vk::UniqueFramebuffer> CreateFramebuffers(
    std::span<const vk::UniqueImageView> imageViews, vk::RenderPass renderPass, vk::Extent2D surfaceExtent, vk::Device device)
{
	auto framebuffers = std::vector<vk::UniqueFramebuffer>(imageViews.size());

	for (const size_t i : std::views::iota(0u, imageViews.size()))
	{
		const auto createInfo = vk::FramebufferCreateInfo{
		    .renderPass = renderPass,
		    .attachmentCount = 1,
		    .pAttachments = &imageViews[i].get(),
		    .width = surfaceExtent.width,
		    .height = surfaceExtent.height,
		    .layers = 1,
		};

		framebuffers[i] = device.createFramebufferUnique(createInfo, s_allocator);
	}

	return framebuffers;
}

std::vector<vk::UniqueCommandBuffer> CreateCommandBuffers(
    vk::CommandPool commandPool, std::span<const vk::UniqueFramebuffer> framebuffers, vk::RenderPass renderPass,
    vk::Extent2D surfaceExtent, vk::Pipeline pipeline, vk::Device device)
{
	const auto allocateInfo = vk::CommandBufferAllocateInfo{
	    .commandPool = commandPool,
	    .level = vk::CommandBufferLevel::ePrimary,
	    .commandBufferCount = static_cast<uint32_t>(framebuffers.size()),
	};

	auto commandBuffers = device.allocateCommandBuffersUnique(allocateInfo);

	for (const size_t i : std::views::iota(0u, commandBuffers.size()))
	{
		const auto& commandBuffer = commandBuffers[i].get();

		commandBuffer.begin(vk::CommandBufferBeginInfo{});

		const auto clearColor = std::array{0.0f, 0.0f, 0.0f, 1.0f};
		const auto clearValue = vk::ClearValue{clearColor};

		const auto renderPassBegin = vk::RenderPassBeginInfo{
		    .renderPass = renderPass,
		    .framebuffer = framebuffers[i].get(),
		    .renderArea = {.offset = {0, 0}, .extent = surfaceExtent},
		    .clearValueCount = 1,
		    .pClearValues = &clearValue,
		};

		commandBuffer.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		commandBuffer.draw(3, 1, 0, 0);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

	return commandBuffers;
}

} // namespace

class VulkanApp
{
public:
	explicit VulkanApp(SDL_Window* window) : m_window{window}
	{
		vk::DynamicLoader dl;
		VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

		m_instance = CreateInstance(window);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());

		const auto debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(DebugCreateInfo, s_allocator);

		m_surface = CreateVulkanSurface(window, m_instance.get());

		std::array deviceExtensions = {"VK_KHR_swapchain"};

		m_physicalDevice = FindPhysicalDevice(m_instance.get(), m_surface.get(), deviceExtensions);
		const auto queueFamilies = FindQueueFamilies(m_physicalDevice, m_surface.get());
		m_graphicsQueueFamily = queueFamilies.graphicsFamily.value();
		m_presentQueueFamily = queueFamilies.presentFamily.value();
		const auto queueFamilyIndices = std::array{m_graphicsQueueFamily, m_presentQueueFamily};
		m_device = CreateDevice(m_physicalDevice, queueFamilyIndices, deviceExtensions);
		m_graphicsQueue = m_device->getQueue(m_graphicsQueueFamily, 0);
		m_presentQueue = m_device->getQueue(m_presentQueueFamily, 0);

		m_graphicsCommandPool = CreateCommandPool(m_graphicsQueueFamily, m_device.get());
		std::ranges::generate(m_imageAvailable, [this] { return CreateSemaphore(m_device.get()); });
		std::ranges::generate(m_renderFinished, [this] { return CreateSemaphore(m_device.get()); });
		std::ranges::generate(m_inFlightFences, [this] { return CreateFence(m_device.get()); });

		m_pipelineLayout = CreateGraphicsPipelineLayout(m_device.get());

		m_vertexShader = CreateShaderModule(TriangleVertexShader, ShaderStage::Vertex, ShaderLang, m_device.get());
		m_pixelShader = CreateShaderModule(TrianglePixelShader, ShaderStage::Pixel, ShaderLang, m_device.get());

		Init();

		spdlog::info("Vulkan initialised successfully");
	}

	void OnRender()
	{
		constexpr auto timeout = std::numeric_limits<uint64_t>::max();

		const auto waitResult = m_device->waitForFences(m_inFlightFences[m_currentFrame].get(), true, timeout);
		assert(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

		// Acquire an image from the swap chain
		const auto [result, imageIndex]
		    = m_device->acquireNextImageKHR(m_swapchain.get(), timeout, m_imageAvailable[m_currentFrame].get());
		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			RecreateSwapchain();
			return;
		}
		else if (result == vk::Result::eSuboptimalKHR)
		{
			spdlog::warn("Suboptimal swapchain image");
		}
		else if (result != vk::Result::eSuccess)
		{
			spdlog::error("Couldn't acquire swapchain image");
			return;
		}

		// Check if a previous frame is using this image (i.e. there is its fence to wait on)
		if (m_imagesInFlight[imageIndex])
		{
			const auto waitResult2 = m_device->waitForFences(m_imagesInFlight[imageIndex], true, timeout);
			assert(waitResult2 == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout
		}
		// Mark the image as in flight
		m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame].get();

		// Submit the command buffer
		const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		const auto submitInfo = vk::SubmitInfo{
		    .waitSemaphoreCount = 1,
		    .pWaitSemaphores = &m_imageAvailable[m_currentFrame].get(),
		    .pWaitDstStageMask = &waitStage,
		    .commandBufferCount = 1,
		    .pCommandBuffers = &m_commandBuffers[imageIndex].get(),
		    .signalSemaphoreCount = 1,
		    .pSignalSemaphores = &m_renderFinished[m_currentFrame].get(),
		};

		m_device->resetFences(m_inFlightFences[m_currentFrame].get());
		m_graphicsQueue.submit(submitInfo, m_inFlightFences[m_currentFrame].get());

		// Present
		const auto presentInfo = vk::PresentInfoKHR{
		    .waitSemaphoreCount = 1,
		    .pWaitSemaphores = &m_renderFinished[m_currentFrame].get(),
		    .swapchainCount = 1,
		    .pSwapchains = &m_swapchain.get(),
		    .pImageIndices = &imageIndex,
		};

		const auto presentResult = m_graphicsQueue.presentKHR(presentInfo);
		if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || m_windowResized)
		{
			m_windowResized = false;
			RecreateSwapchain();
		}
		if (presentResult != vk::Result::eSuccess)
		{
			spdlog::error("Couldn't present");
		}

		m_currentFrame = (m_currentFrame + 1) % MaxFramesInFlight;
	}

	void OnResize() { m_windowResized = true; }

	bool OnEvent(const SDL_Event& event)
	{
		switch (event.type)
		{
			case SDL_QUIT:
				return false;

			case SDL_WINDOWEVENT_RESIZED:
				OnResize();
				break;
		}
		return true;
	}

	bool OnUpdate() { return true; }

	void MainLoop()
	{
		SDL_Event event;
		bool running = true;
		while (running)
		{
			while (SDL_PollEvent(&event))
			{
				running &= OnEvent(event);
			}

			running &= OnUpdate();
			OnRender();
		}

		m_device->waitIdle();
	}

private:
	void Init()
	{
		const auto surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
		const auto surfaceFormat = ChooseSurfaceFormat(m_physicalDevice.getSurfaceFormatsKHR(m_surface.get()));
		const auto surfaceExtent = ChooseSwapExtent(surfaceCapabilities, m_window);
		const auto queueFamilyIndices = std::array{m_graphicsQueueFamily, m_presentQueueFamily};
		m_swapchain = CreateSwapchain(
		    m_physicalDevice, queueFamilyIndices, m_surface.get(), surfaceFormat, surfaceExtent, m_device.get());
		m_swapchainImages = m_device->getSwapchainImagesKHR(m_swapchain.get());
		m_swapchainImageViews = CreateSwapchainImageViews(surfaceFormat.format, m_swapchainImages, m_device.get());
		m_imagesInFlight.resize(m_swapchainImages.size());

		m_renderPass = CreateRenderPass(m_device.get(), surfaceFormat.format);

		m_pipeline = CreateGraphicsPipeline(
		    m_vertexShader.get(), m_pixelShader.get(), m_pipelineLayout.get(), m_renderPass.get(), surfaceExtent,
		    m_device.get());

		m_swapchainFramebuffers
		    = CreateFramebuffers(m_swapchainImageViews, m_renderPass.get(), surfaceExtent, m_device.get());
		m_commandBuffers = CreateCommandBuffers(
		    m_graphicsCommandPool.get(), m_swapchainFramebuffers, m_renderPass.get(), surfaceExtent, m_pipeline.get(),
		    m_device.get());
	}

	void RecreateSwapchain()
	{
		m_device->waitIdle();
		m_commandBuffers.clear();
		Init();
	}

	SDL_Window* m_window;
	bool m_windowResized = false;

	vk::UniqueInstance m_instance;
	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_device;
	vk::UniqueSurfaceKHR m_surface;
	uint32_t m_graphicsQueueFamily;
	uint32_t m_presentQueueFamily;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;
	vk::UniqueSwapchainKHR m_swapchain;
	std::vector<vk::Image> m_swapchainImages;
	std::vector<vk::UniqueImageView> m_swapchainImageViews;
	vk::UniqueCommandPool m_graphicsCommandPool;

	// Triangle setup
	vk::UniqueShaderModule m_vertexShader;
	vk::UniqueShaderModule m_pixelShader;
	vk::UniquePipeline m_pipeline;
	vk::UniqueRenderPass m_renderPass;
	vk::UniquePipelineLayout m_pipelineLayout;
	std::vector<vk::UniqueFramebuffer> m_swapchainFramebuffers;
	std::vector<vk::UniqueCommandBuffer> m_commandBuffers;

	// Triangle rendering
	int m_currentFrame = 0;
	static constexpr int MaxFramesInFlight = 2;
	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_imageAvailable;
	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
	std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
	std::vector<vk::Fence> m_imagesInFlight;
};

struct SDLWindowDeleter
{
	void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

int Run()
{
	const auto windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
	const auto window = UniqueSDLWindow(
	    SDL_CreateWindow("Teide", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 768, windowFlags), {});
	if (!window)
	{
		spdlog::critical("SDL error: {}", SDL_GetError());
		std::string message = std::format("The following error occurred when initializing SDL: {}", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}

	try
	{
		auto vulkan = VulkanApp(window.get());

		vulkan.MainLoop();
	}
	catch (const vk::Error& e)
	{
		spdlog::critical("Vulkan error: {}", e.what());
		std::string message = std::format("The following error occurred when initializing Vulkan:\n{}", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}

	return 0;
}

int SDL_main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
#ifdef _WIN32
	if (IsDebuggerPresent())
	{
		// Send log output to Visual Studio's Output window when running in the debugger.
		spdlog::set_default_logger(std::make_shared<spdlog::logger>("msvc", std::make_shared<spdlog::sinks::msvc_sink_mt>()));
	}
#endif

	spdlog::set_pattern("[%Y-%m-%D %H:%M:%S.%e] [%l] %v");

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		spdlog::critical("Couldn't initialise SDL: {}", SDL_GetError());
		return 1;
	}

	spdlog::info("SDL initialised successfully");

	int retcode = Run();

	SDL_Quit();

	return retcode;
}
