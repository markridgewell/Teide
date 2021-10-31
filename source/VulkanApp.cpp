
#include "Definitions.h"
#include "GeoLib/Matrix.h"
#include "GeoLib/Vector.h"
#include "ShaderCompiler.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
constexpr std::string_view VertexShader = R"--(
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    outTexCoord = inTexCoord;
    outColor = inColor;
})--";

constexpr std::string_view PixelShader = R"--(
#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, inTexCoord);
})--";

struct Vertex
{
	Geo::Vector2 position;
	Geo::Vector2 texCoord;
	Geo::Vector3 color;
};

constexpr auto QuadVertices = std::array<Vertex, 4>{{
    {{-0.5f, -0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
}};

constexpr auto QuadIndices = std::array<uint16_t, 6>{{0, 1, 2, 2, 3, 0}};

constexpr auto VertexBindingDescription = vk::VertexInputBindingDescription{
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = vk::VertexInputRate::eVertex,
};

constexpr auto VertexAttributeDescriptions = std::array{
    vk::VertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(Vertex, position),
    },
    vk::VertexInputAttributeDescription{
        .location = 1,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, texCoord),
    },
    vk::VertexInputAttributeDescription{
        .location = 2,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, color),
    },
};

struct UniformBufferObject
{
	Geo::Matrix4 model;
	Geo::Matrix4 view;
	Geo::Matrix4 proj;
};

constexpr auto ShaderLang = ShaderLanguage::Glsl;

class CustomError : public vk::Error, public std::exception
{
public:
	explicit CustomError(std::string message) : m_what{std::move(message)} {}

	virtual const char* what() const noexcept { return m_what.c_str(); }

private:
	std::string m_what;
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

constexpr auto DebugCreateInfo = [] {
	using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
	using enum vk::DebugUtilsMessageTypeFlagBitsEXT;

	return vk::DebugUtilsMessengerCreateInfoEXT{
	    .messageSeverity = eError | eWarning | eInfo,
	    .messageType = eGeneral | eValidation | ePerformance,
	    .pfnUserCallback = DebugCallback,
	};
}();

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

		const auto enabledFeatures = std::array{
		    vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
		    // vk::ValidationFeatureEnableEXT::eBestPractices,
		};

		const auto createInfo = vk::StructureChain{
		    vk::InstanceCreateInfo{
		        .pApplicationInfo = &applicationInfo,
		        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
		        .ppEnabledLayerNames = layers.data(),
		        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		        .ppEnabledExtensionNames = extensions.data()},
		    vk::ValidationFeaturesEXT{
		        .enabledValidationFeatureCount = static_cast<uint32_t>(enabledFeatures.size()),
		        .pEnabledValidationFeatures = enabledFeatures.data(),
		    },
		    DebugCreateInfo,
		};

		return vk::createInstanceUnique(createInfo.get<vk::InstanceCreateInfo>(), s_allocator);
	}
	else
	{
		const auto createInfo = vk::InstanceCreateInfo{
		    .pApplicationInfo = &applicationInfo,
		    .enabledLayerCount = static_cast<uint32_t>(layers.size()),
		    .ppEnabledLayerNames = layers.data(),
		    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		    .ppEnabledExtensionNames = extensions.data()};

		return vk::createInstanceUnique(createInfo, s_allocator);
	}
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

		// Check all required features are supported
		const auto supportedFeatures = device.getFeatures();
		if (!supportedFeatures.samplerAnisotropy)
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

	const auto deviceFeatures = vk::PhysicalDeviceFeatures{
	    .samplerAnisotropy = true,
	};

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

uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags flags)
{
	const auto memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags))
		{
			return i;
		}
	}

	throw CustomError("Failed to find suitable memory type");
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

vk::UniqueDeviceMemory CreateDeviceMemory(
    vk::Device device, vk::PhysicalDevice physicalDevice, const vk::MemoryRequirements& memoryRequirements,
    vk::MemoryPropertyFlags flags)
{
	const auto allocInfo = vk::MemoryAllocateInfo{
	    .allocationSize = memoryRequirements.size,
	    .memoryTypeIndex = FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, flags),
	};

	return device.allocateMemoryUnique(allocInfo);
}

vk::UniqueBuffer CreateBuffer(vk::Device device, vk::DeviceSize size, vk::BufferUsageFlags usage)
{
	const auto createInfo = vk::BufferCreateInfo{
	    .size = size,
	    .usage = usage,
	    .sharingMode = vk::SharingMode::eExclusive,
	};
	return device.createBufferUnique(createInfo, s_allocator);
}

template <size_t Extent>
void SetBufferData(vk::Device device, vk::DeviceMemory bufferMemory, std::span<const std::byte, Extent> data)
{
	void* mappedData = device.mapMemory(bufferMemory, 0, data.size());
	memcpy(mappedData, data.data(), data.size());
	device.unmapMemory(bufferMemory);
}

template <class T>
concept Span = requires(T t)
{
	{std::span{t}};
};

template <class T>
concept TrivialSpan
    = Span<T> && std::is_trivially_copyable_v<typename T::value_type> && std::is_standard_layout_v<typename T::value_type>;

template <class T>
concept TrivalObject = !Span<T> && std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

template <TrivialSpan T>
void SetBufferData(vk::Device device, vk::DeviceMemory bufferMemory, const T& data)
{
	SetBufferData(device, bufferMemory, std::as_bytes(std::span(data)));
}

template <TrivalObject T>
void SetBufferData(vk::Device device, vk::DeviceMemory bufferMemory, const T& data)
{
	SetBufferData(device, bufferMemory, std::as_bytes(std::span(&data, 1)));
}

void CopyBuffer(vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Buffer destination, vk::DeviceSize size)
{
	const auto copyRegion = vk::BufferCopy{
	    .srcOffset = 0,
	    .dstOffset = 0,
	    .size = size,
	};
	cmdBuffer.copyBuffer(source, destination, copyRegion);
}

void CopyBufferToImage(vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Image destination, vk::Extent3D extent)
{
	const auto copyRegion = vk::BufferImageCopy
	{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0, .imageSubresource = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = {0,0,0},
		.imageExtent = extent,
	};
	cmdBuffer.copyBufferToImage(source, destination, vk::ImageLayout::eTransferDstOptimal, copyRegion);
}

struct TransitionAccessMasks
{
	vk::AccessFlags source;
	vk::AccessFlags destination;
};

TransitionAccessMasks GetTransitionAccessMasks(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	using enum vk::ImageLayout;
	if (oldLayout == eUndefined && newLayout == eTransferDstOptimal)
	{
		return {{}, vk::AccessFlagBits::eTransferWrite};
	}
	else if (oldLayout == eTransferDstOptimal && newLayout == eShaderReadOnlyOptimal)
	{
		return {vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead};
	}

	return {};
}

void TransitionImageLayout(
    vk::CommandBuffer cmdBuffer, vk::Image image, [[maybe_unused]] vk::Format format, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::PipelineStageFlagBits srcStageMask, vk::PipelineStageFlagBits dstStageMask)
{
	const auto accessMasks = GetTransitionAccessMasks(oldLayout, newLayout);

	const auto barrier = vk::ImageMemoryBarrier{
	    .srcAccessMask = accessMasks.source,
	    .dstAccessMask = accessMasks.destination,
	    .oldLayout = oldLayout,
	    .newLayout = newLayout,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = image,
	    .subresourceRange = {
	        .aspectMask = vk::ImageAspectFlagBits::eColor,
	        .baseMipLevel = 0,
	        .levelCount = 1,
	        .baseArrayLayer = 0,
	        .layerCount = 1,
	    },
	};

	cmdBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
}

vk::UniqueDescriptorPool CreateDescriptorPool(vk::Device device, uint32_t numSwapchainImages)
{
	const auto poolSizes = std::array{
	    vk::DescriptorPoolSize{
	        .type = vk::DescriptorType::eUniformBuffer,
	        .descriptorCount = numSwapchainImages,
	    },
	    vk::DescriptorPoolSize{
	        .type = vk::DescriptorType::eCombinedImageSampler,
	        .descriptorCount = numSwapchainImages,
	    },
	};

	const auto createInfo = vk::DescriptorPoolCreateInfo{
	    .maxSets = numSwapchainImages,
	}.setPoolSizes(poolSizes);

	return device.createDescriptorPoolUnique(createInfo, s_allocator);
}

vk::UniqueDescriptorSetLayout CreateDescriptorSetLayout(vk::Device device)
{
	const auto layoutBindings = std::array{
	    vk::DescriptorSetLayoutBinding{
	        .binding = 0,
	        .descriptorType = vk::DescriptorType::eUniformBuffer,
	        .descriptorCount = 1,
	        .stageFlags = vk::ShaderStageFlagBits::eVertex,
	    },
	    vk::DescriptorSetLayoutBinding{
	        .binding = 1,
	        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
	        .descriptorCount = 1,
	        .stageFlags = vk::ShaderStageFlagBits::eFragment,
	        .pImmutableSamplers = nullptr,
	    },
	};

	const auto createInfo = vk::DescriptorSetLayoutCreateInfo{}.setBindings(layoutBindings);

	return device.createDescriptorSetLayoutUnique(createInfo, s_allocator);
}

vk::UniquePipeline CreateGraphicsPipeline(
    vk::ShaderModule vertexShader, vk::ShaderModule pixelShader, vk::PipelineLayout layout, vk::RenderPass renderPass,
    vk::Extent2D surfaceExtent, vk::Device device)
{
	const auto shaderStages = std::array{
	    vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = vertexShader, .pName = "main"},
	    vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = pixelShader, .pName = "main"},
	};

	const auto vertexInput = vk::PipelineVertexInputStateCreateInfo{}
	                             .setVertexBindingDescriptions(VertexBindingDescription)
	                             .setVertexAttributeDescriptions(VertexAttributeDescriptions);

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

vk::UniquePipelineLayout CreateGraphicsPipelineLayout(vk::Device device, vk::DescriptorSetLayout setLayout)
{
	const auto createInfo = vk::PipelineLayoutCreateInfo{
	    .setLayoutCount = 1,
	    .pSetLayouts = &setLayout,
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

} // namespace

class VulkanApp
{
public:
	explicit VulkanApp(SDL_Window* window, const char* imageFilename) : m_window{window}
	{
		vk::DynamicLoader dl;
		VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

		m_instance = CreateInstance(window);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());

		if constexpr (IsDebugBuild)
		{
			m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(DebugCreateInfo, s_allocator);
		}

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

		m_descriptorSetLayout = CreateDescriptorSetLayout(m_device.get());
		m_pipelineLayout = CreateGraphicsPipelineLayout(m_device.get(), m_descriptorSetLayout.get());

		m_vertexShader = CreateShaderModule(VertexShader, ShaderStage::Vertex, ShaderLang, m_device.get());
		m_pixelShader = CreateShaderModule(PixelShader, ShaderStage::Pixel, ShaderLang, m_device.get());

		CreateSwapchainAndImages();

		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();
		CreateTextureImage(imageFilename);
		CreateTextureSampler();
		CreateDescriptorSets();

		CreateCommandBuffers();

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

		// Update uniforms
		const auto currentTime = std::chrono::high_resolution_clock::now();
		const float timeSeconds
		    = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_startTime).count();
		const float aspectRatio = static_cast<float>(m_surfaceExtent.width) / static_cast<float>(m_surfaceExtent.height);

		using namespace Geo::Literals;
		const auto ubo = UniformBufferObject{
		    .model = Geo::Matrix4::RotationZ(timeSeconds * 90.0_degf),
		    .view = Geo::Matrix4::LookAt({2.0f, 2.0f, 2.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
		    .proj = Geo::Matrix4::Perspective(45.0_degf, aspectRatio, 0.1f, 10.0f),
		};

		SetBufferData(m_device.get(), m_uniformBuffersMemory[imageIndex].get(), ubo);

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

		try
		{
			const auto presentResult = m_graphicsQueue.presentKHR(presentInfo);
			if (presentResult == vk::Result::eSuboptimalKHR || m_windowResized)
			{
				m_windowResized = false;
				RecreateSwapchain();
			}
		}
		catch (const vk::Error&)
		{
			m_windowResized = false;
			RecreateSwapchain();
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
	auto OneShotCommandBuffer()
	{
		class OneShot
		{
		public:
			explicit OneShot(VulkanApp& vulkan) : m_queue{vulkan.m_graphicsQueue}
			{
				const auto allocInfo = vk::CommandBufferAllocateInfo{
				    .commandPool = vulkan.m_graphicsCommandPool.get(),
				    .level = vk::CommandBufferLevel::ePrimary,
				    .commandBufferCount = 1,
				};
				auto cmdBuffers = vulkan.m_device->allocateCommandBuffersUnique(allocInfo);
				m_cmdBuffer = std::move(cmdBuffers.front());

				m_cmdBuffer->begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
			}

			~OneShot()
			{
				m_cmdBuffer->end();

				const auto submitInfo = vk::SubmitInfo{}.setCommandBuffers(m_cmdBuffer.get());
				m_queue.submit(submitInfo);
				m_queue.waitIdle();
			}

			operator vk::CommandBuffer() const { return m_cmdBuffer.get(); }

		private:
			vk::UniqueCommandBuffer m_cmdBuffer;
			vk::Queue m_queue;
		};

		return OneShot(*this);
	}

	void CreateSwapchainAndImages()
	{
		const auto surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
		const auto surfaceFormat = ChooseSurfaceFormat(m_physicalDevice.getSurfaceFormatsKHR(m_surface.get()));
		m_surfaceExtent = ChooseSwapExtent(surfaceCapabilities, m_window);
		const auto queueFamilyIndices = std::array{m_graphicsQueueFamily, m_presentQueueFamily};
		m_swapchain = CreateSwapchain(
		    m_physicalDevice, queueFamilyIndices, m_surface.get(), surfaceFormat, m_surfaceExtent, m_device.get());
		m_swapchainImages = m_device->getSwapchainImagesKHR(m_swapchain.get());
		m_swapchainImageViews = CreateSwapchainImageViews(surfaceFormat.format, m_swapchainImages, m_device.get());
		m_imagesInFlight.resize(m_swapchainImages.size());

		m_renderPass = CreateRenderPass(m_device.get(), surfaceFormat.format);
		m_swapchainFramebuffers
		    = CreateFramebuffers(m_swapchainImageViews, m_renderPass.get(), m_surfaceExtent, m_device.get());

		m_pipeline = CreateGraphicsPipeline(
		    m_vertexShader.get(), m_pixelShader.get(), m_pipelineLayout.get(), m_renderPass.get(), m_surfaceExtent,
		    m_device.get());
	}

	void CreateVertexBuffer()
	{
		const auto bufferSize = QuadVertices.size() * sizeof(Vertex);

		// Create staging buffer for vertices
		const auto stagingBuffer = CreateBuffer(m_device.get(), bufferSize, vk::BufferUsageFlagBits::eTransferSrc);
		const auto stagingMemory = CreateDeviceMemory(
		    m_device.get(), m_physicalDevice, m_device->getBufferMemoryRequirements(stagingBuffer.get()),
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		m_device->bindBufferMemory(stagingBuffer.get(), stagingMemory.get(), 0);
		SetBufferData(m_device.get(), stagingMemory.get(), QuadVertices);

		// Create vertex buffer
		m_vertexBuffer = CreateBuffer(
		    m_device.get(), bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
		m_vertexBufferMemory = CreateDeviceMemory(
		    m_device.get(), m_physicalDevice, m_device->getBufferMemoryRequirements(m_vertexBuffer.get()),
		    vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_device->bindBufferMemory(m_vertexBuffer.get(), m_vertexBufferMemory.get(), 0);
		CopyBuffer(OneShotCommandBuffer(), stagingBuffer.get(), m_vertexBuffer.get(), bufferSize);
	}

	void CreateIndexBuffer()
	{
		const auto bufferSize = QuadIndices.size() * sizeof(decltype(QuadIndices)::value_type);

		// Create staging buffer
		const auto stagingBuffer = CreateBuffer(m_device.get(), bufferSize, vk::BufferUsageFlagBits::eTransferSrc);
		const auto stagingMemory = CreateDeviceMemory(
		    m_device.get(), m_physicalDevice, m_device->getBufferMemoryRequirements(stagingBuffer.get()),
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		m_device->bindBufferMemory(stagingBuffer.get(), stagingMemory.get(), 0);
		SetBufferData(m_device.get(), stagingMemory.get(), QuadIndices);

		// Create index buffer
		m_indexBuffer = CreateBuffer(
		    m_device.get(), bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
		m_indexBufferMemory = CreateDeviceMemory(
		    m_device.get(), m_physicalDevice, m_device->getBufferMemoryRequirements(m_indexBuffer.get()),
		    vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_device->bindBufferMemory(m_indexBuffer.get(), m_indexBufferMemory.get(), 0);
		CopyBuffer(OneShotCommandBuffer(), stagingBuffer.get(), m_indexBuffer.get(), bufferSize);
	}

	void CreateUniformBuffers()
	{
		const auto bufferSize = sizeof(UniformBufferObject);

		m_uniformBuffers.resize(m_swapchainImages.size());
		m_uniformBuffersMemory.resize(m_swapchainImages.size());
		for (size_t i = 0; i < m_swapchainImages.size(); i++)
		{
			m_uniformBuffers[i] = CreateBuffer(m_device.get(), bufferSize, vk::BufferUsageFlagBits::eUniformBuffer);
			m_uniformBuffersMemory[i] = CreateDeviceMemory(
			    m_device.get(), m_physicalDevice, m_device->getBufferMemoryRequirements(m_uniformBuffers[i].get()),
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			m_device->bindBufferMemory(m_uniformBuffers[i].get(), m_uniformBuffersMemory[i].get(), 0);
		}
	}

	void CreateDescriptorSets()
	{
		auto descriptorPool = CreateDescriptorPool(m_device.get(), static_cast<uint32_t>(m_swapchainImages.size()));

		const auto layouts = std::vector<vk::DescriptorSetLayout>(m_swapchainImages.size(), m_descriptorSetLayout.get());
		const auto allocInfo = vk::DescriptorSetAllocateInfo{
		    .descriptorPool = descriptorPool.get(),
		    .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
		    .pSetLayouts = layouts.data(),
		};

		m_descriptorSets = m_device->allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < layouts.size(); i++)
		{
			const auto bufferInfo = vk::DescriptorBufferInfo{
			    .buffer = m_uniformBuffers[i].get(),
			    .offset = 0,
			    .range = sizeof(UniformBufferObject),
			};

			const auto imageInfo = vk::DescriptorImageInfo{
			    .sampler = m_textureSampler.get(),
			    .imageView = m_textureImageView.get(),
			    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
			};

			const auto descriptorWrites = std::array{
			    vk::WriteDescriptorSet{
			        .dstSet = m_descriptorSets[i],
			        .dstBinding = 0,
			        .dstArrayElement = 0,
			        .descriptorCount = 1,
			        .descriptorType = vk::DescriptorType::eUniformBuffer,
			        .pBufferInfo = &bufferInfo,
			    },
			    vk::WriteDescriptorSet{
			        .dstSet = m_descriptorSets[i],
			        .dstBinding = 1,
			        .dstArrayElement = 0,
			        .descriptorCount = 1,
			        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
			        .pImageInfo = &imageInfo,
			    },
			};

			m_device->updateDescriptorSets(descriptorWrites, {});
		}

		m_descriptorPool = std::move(descriptorPool);
	}

	void CreateTextureImage(const char* filename)
	{
		struct StbiDeleter
		{
			void operator()(stbi_uc* p) { stbi_image_free(p); }
		};
		using StbiPtr = std::unique_ptr<stbi_uc, StbiDeleter>;

		// Load image
		int width{}, height{}, channels{};
		const auto pixels = StbiPtr(stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha));
		if (!pixels)
		{
			throw CustomError(std::format("Error loading texture '{}'", filename));
		}

		vk::DeviceSize imageSize = width * height * 4;

		// Create staging buffer
		const auto stagingBuffer = CreateBuffer(m_device.get(), imageSize, vk::BufferUsageFlagBits::eTransferSrc);
		const auto stagingMemory = CreateDeviceMemory(
		    m_device.get(), m_physicalDevice, m_device->getBufferMemoryRequirements(stagingBuffer.get()),
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		m_device->bindBufferMemory(stagingBuffer.get(), stagingMemory.get(), 0);
		SetBufferData(m_device.get(), stagingMemory.get(), std::span(pixels.get(), imageSize));

		// Create image
		const auto imageExtent = vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
		const auto imageInfo = vk::ImageCreateInfo{
		    .imageType = vk::ImageType::e2D,
		    .format = vk::Format::eR8G8B8A8Srgb,
		    .extent = imageExtent,
		    .mipLevels = 1,
		    .arrayLayers = 1,
		    .samples = vk::SampleCountFlagBits::e1,
		    .tiling = vk::ImageTiling::eOptimal,
		    .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		    .sharingMode = vk::SharingMode::eExclusive,
		    .initialLayout = vk::ImageLayout::eUndefined,
		};

		m_textureImage = m_device->createImageUnique(imageInfo, s_allocator);
		m_textureMemory = CreateDeviceMemory(
		    m_device.get(), m_physicalDevice, m_device->getImageMemoryRequirements(m_textureImage.get()),
		    vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_device->bindImageMemory(m_textureImage.get(), m_textureMemory.get(), 0);
		const auto cmdBuffer = OneShotCommandBuffer();
		TransitionImageLayout(
		    cmdBuffer, m_textureImage.get(), imageInfo.format, imageInfo.initialLayout, vk::ImageLayout::eTransferDstOptimal,
		    vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer);
		CopyBufferToImage(cmdBuffer, stagingBuffer.get(), m_textureImage.get(), imageExtent);
		TransitionImageLayout(
		    cmdBuffer, m_textureImage.get(), imageInfo.format, vk::ImageLayout::eTransferDstOptimal,
		    vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTransfer,
		    vk::PipelineStageFlagBits::eFragmentShader);

		// Create image view
		const auto viewInfo = vk::ImageViewCreateInfo{
			.image = m_textureImage.get(),
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
		m_textureImageView = m_device->createImageViewUnique(viewInfo, s_allocator);
	}

	void CreateTextureSampler()
	{
		const auto samplerInfo = vk::SamplerCreateInfo{
		    .magFilter = vk::Filter::eLinear,
		    .minFilter = vk::Filter::eLinear,
		    .mipmapMode = vk::SamplerMipmapMode::eLinear,
		    .addressModeU = vk::SamplerAddressMode::eRepeat,
		    .addressModeV = vk::SamplerAddressMode::eRepeat,
		    .addressModeW = vk::SamplerAddressMode::eRepeat,
		    .anisotropyEnable = true,
		    .maxAnisotropy = m_physicalDevice.getProperties().limits.maxSamplerAnisotropy,
		};

		m_textureSampler = m_device->createSamplerUnique(samplerInfo, s_allocator);
	}

	void DrawObjects(vk::CommandBuffer commandBuffer, vk::Framebuffer framebuffer, vk::Extent2D surfaceExtent, size_t imageIndex)
	{
		commandBuffer.begin(vk::CommandBufferBeginInfo{});

		const auto clearColor = std::array{0.0f, 0.0f, 0.0f, 1.0f};
		const auto clearValue = vk::ClearValue{clearColor};

		const auto renderPassBegin = vk::RenderPassBeginInfo{
		    .renderPass = m_renderPass.get(),
		    .framebuffer = framebuffer,
		    .renderArea = {.offset = {0, 0}, .extent = surfaceExtent},
		    .clearValueCount = 1,
		    .pClearValues = &clearValue,
		};

		const auto descriptorSet = m_descriptorSets[imageIndex];

		commandBuffer.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
		commandBuffer.bindVertexBuffers(0, m_vertexBuffer.get(), vk::DeviceSize{0});
		commandBuffer.bindIndexBuffer(m_indexBuffer.get(), vk::DeviceSize{0}, vk::IndexType::eUint16);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSet, {});
		commandBuffer.drawIndexed(static_cast<uint32_t>(QuadIndices.size()), 1, 0, 0, 0);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

	void CreateCommandBuffers()
	{
		const auto allocateInfo = vk::CommandBufferAllocateInfo{
		    .commandPool = m_graphicsCommandPool.get(),
		    .level = vk::CommandBufferLevel::ePrimary,
		    .commandBufferCount = static_cast<uint32_t>(m_swapchainFramebuffers.size()),
		};

		m_commandBuffers = m_device->allocateCommandBuffersUnique(allocateInfo);

		for (const size_t i : std::views::iota(0u, m_commandBuffers.size()))
		{
			DrawObjects(m_commandBuffers[i].get(), m_swapchainFramebuffers[i].get(), m_surfaceExtent, i);
		}
	}

	void RecreateSwapchain()
	{
		m_device->waitIdle();
		m_commandBuffers.clear();
		CreateSwapchainAndImages();
		CreateUniformBuffers();
		CreateDescriptorSets();
		CreateCommandBuffers();
	}

	SDL_Window* m_window;
	bool m_windowResized = false;
	vk::Extent2D m_surfaceExtent;

	std::chrono::high_resolution_clock::time_point m_startTime = std::chrono::high_resolution_clock::now();

	vk::UniqueInstance m_instance;
	vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
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
	vk::UniqueDescriptorPool m_descriptorPool;

	// Object setup
	vk::UniqueShaderModule m_vertexShader;
	vk::UniqueShaderModule m_pixelShader;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
	vk::UniquePipeline m_pipeline;
	vk::UniqueRenderPass m_renderPass;
	vk::UniquePipelineLayout m_pipelineLayout;
	vk::UniqueBuffer m_vertexBuffer;
	vk::UniqueDeviceMemory m_vertexBufferMemory;
	vk::UniqueBuffer m_indexBuffer;
	vk::UniqueDeviceMemory m_indexBufferMemory;
	vk::UniqueImage m_textureImage;
	vk::UniqueDeviceMemory m_textureMemory;
	vk::UniqueImageView m_textureImageView;
	vk::UniqueSampler m_textureSampler;
	std::vector<vk::UniqueBuffer> m_uniformBuffers;
	std::vector<vk::UniqueDeviceMemory> m_uniformBuffersMemory;
	std::vector<vk::DescriptorSet> m_descriptorSets;
	std::vector<vk::UniqueFramebuffer> m_swapchainFramebuffers;
	std::vector<vk::UniqueCommandBuffer> m_commandBuffers;

	// Rendering
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

int Run(int argc, char* argv[])
{
	const char* const imageFilename = (argc >= 2) ? argv[1] : "";

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

	std::optional<VulkanApp> vulkan;
	try
	{
		vulkan.emplace(window.get(), imageFilename);
	}
	catch (const vk::Error& e)
	{
		spdlog::critical("Vulkan error: {}", e.what());
		std::string message = std::format("The following error occurred when initializing Vulkan:\n{}", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}
	catch (const CompileError& e)
	{
		spdlog::critical("Shader compilation error: {}", e.what());
		std::string message = std::format("The following error occurred when compiling shaders:\n{}", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}

	vulkan->MainLoop();

	return 0;
}

int SDL_main(int argc, char* argv[])
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

	int retcode = Run(argc, argv);

	SDL_Quit();

	return retcode;
}
