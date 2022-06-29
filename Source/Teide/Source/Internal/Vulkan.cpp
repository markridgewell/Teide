
#include "Teide/Internal/Vulkan.h"

#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace
{
constexpr bool BreakOnVulkanWarning = false;
constexpr bool BreakOnVulkanError = true;

const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData)
{
	using MessageType = vk::DebugUtilsMessageTypeFlagBitsEXT;
	using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;

	// Filter unwanted messages
	if (pCallbackData->messageIdNumber == 767975156)
	{
		// UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension
		return VK_FALSE;
	}

	const char* prefix = "";
	switch (MessageType(messageType))
	{
		case MessageType::eGeneral:
			prefix = "";
			break;
		case MessageType::eValidation:
			prefix = "[validation] ";
			break;
		case MessageType::ePerformance:
			prefix = "[performance] ";
			break;
	}

	switch (MessageSeverity(messageSeverity))
	{
		case MessageSeverity::eVerbose:
			spdlog::debug("{}{}", prefix, pCallbackData->pMessage);
			break;

		case MessageSeverity::eInfo:
			spdlog::info("{}{}", prefix, pCallbackData->pMessage);
			break;

		case MessageSeverity::eWarning:
			spdlog::warn("{}{}", prefix, pCallbackData->pMessage);
			assert(!BreakOnVulkanWarning);
			break;

		case MessageSeverity::eError:
			spdlog::error("{}{}", prefix, pCallbackData->pMessage);
			assert(!BreakOnVulkanError);
			break;
	}
	return VK_FALSE;
}

struct TransitionAccessMasks
{
	vk::AccessFlags source;
	vk::AccessFlags destination;
};

vk::AccessFlags GetTransitionAccessMask(vk::ImageLayout layout)
{
	using enum vk::ImageLayout;
	using Access = vk::AccessFlagBits;

	switch (layout)
	{
		case eUndefined:
			return {};

		case eTransferDstOptimal:
			return Access::eTransferWrite;

		case eColorAttachmentOptimal:
			return Access::eColorAttachmentRead | Access::eColorAttachmentWrite;

		case eDepthStencilAttachmentOptimal:
			return Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite;

		case eDepthAttachmentOptimal:
			return Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite;

		case eStencilAttachmentOptimal:
			return Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite;

		case eShaderReadOnlyOptimal:
			return Access::eShaderRead;

		case eTransferSrcOptimal:
			return Access::eTransferRead;

		case eDepthStencilReadOnlyOptimal:
			return Access::eShaderRead;

		default:
			assert(false && "Unsupported image transition");
			return {};
	}
}

TransitionAccessMasks GetTransitionAccessMasks(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	return {GetTransitionAccessMask(oldLayout), GetTransitionAccessMask(newLayout)};
}
} // namespace

vk::DebugUtilsMessengerCreateInfoEXT GetDebugCreateInfo()
{
	using MessageType = vk::DebugUtilsMessageTypeFlagBitsEXT;
	using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;

	return {
	    .messageSeverity = MessageSeverity::eError | MessageSeverity::eWarning | MessageSeverity::eInfo,
	    .messageType = MessageType::eGeneral | MessageType::eValidation | MessageType::ePerformance,
	    .pfnUserCallback = DebugCallback,
	};
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

vk::UniqueInstance CreateInstance(vk::DynamicLoader& loader, SDL_Window* window)
{
	VULKAN_HPP_DEFAULT_DISPATCHER.init(loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

	std::vector<const char*> extensions;
	if (window)
	{
		uint32_t extensionCount = 0;
		SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
		extensions.resize(extensionCount);
		SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
	}

	vk::ApplicationInfo applicationInfo{
	    .apiVersion = VK_API_VERSION_1_0,
	};

	const auto availableLayers = vk::enumerateInstanceLayerProperties();
	const auto availableExtensions = vk::enumerateInstanceExtensionProperties();

	std::vector<const char*> layers;

	vk::UniqueInstance instance;
	if constexpr (IsDebugBuild)
	{
		EnableOptionalVulkanLayer(layers, availableLayers, "VK_LAYER_KHRONOS_validation");
		EnableRequiredVulkanExtension(extensions, availableExtensions, "VK_EXT_debug_utils");

		const auto enabledFeatures = std::array{
		    vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
		    vk::ValidationFeatureEnableEXT::eBestPractices,
		};

		const auto createInfo = vk::StructureChain{
		    vk::InstanceCreateInfo{
		        .pApplicationInfo = &applicationInfo,
		        .enabledLayerCount = size32(layers),
		        .ppEnabledLayerNames = data(layers),
		        .enabledExtensionCount = size32(extensions),
		        .ppEnabledExtensionNames = data(extensions)},
		    vk::ValidationFeaturesEXT{
		        .enabledValidationFeatureCount = size32(enabledFeatures),
		        .pEnabledValidationFeatures = data(enabledFeatures),
		    },
		    GetDebugCreateInfo(),
		};

		instance = vk::createInstanceUnique(createInfo.get<vk::InstanceCreateInfo>(), s_allocator);
	}
	else
	{
		const auto createInfo = vk::InstanceCreateInfo{
		    .pApplicationInfo = &applicationInfo,
		    .enabledLayerCount = size32(layers),
		    .ppEnabledLayerNames = data(layers),
		    .enabledExtensionCount = size32(extensions),
		    .ppEnabledExtensionNames = data(extensions),
		};

		instance = vk::createInstanceUnique(createInfo, s_allocator);
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance.get());
	return instance;
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
	    .queueCreateInfoCount = size32(queueCreateInfos),
	    .pQueueCreateInfos = data(queueCreateInfos),
	    .enabledLayerCount = size32(layers),
	    .ppEnabledLayerNames = data(layers),
	    .enabledExtensionCount = size32(extensions),
	    .ppEnabledExtensionNames = data(extensions),
	    .pEnabledFeatures = &deviceFeatures};

	return physicalDevice.createDeviceUnique(deviceCreateInfo, s_allocator);
}

bool HasDepthComponent(vk::Format format)
{
	return format == vk::Format::eD16Unorm || format == vk::Format::eD32Sfloat || format == vk::Format::eD16UnormS8Uint
	    || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

bool HasStencilComponent(vk::Format format)
{
	return format == vk::Format::eS8Uint || format == vk::Format::eD16UnormS8Uint
	    || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

bool HasDepthOrStencilComponent(vk::Format format)
{
	return format == vk::Format::eD16Unorm || format == vk::Format::eD32Sfloat || format == vk::Format::eD16UnormS8Uint
	    || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eS8Uint;
}

void TransitionImageLayout(
    vk::CommandBuffer cmdBuffer, vk::Image image, vk::Format format, uint32_t mipLevelCount, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask)
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
	        .aspectMask = GetImageAspect(format),
	        .baseMipLevel = 0,
	        .levelCount = mipLevelCount,
	        .baseArrayLayer = 0,
	        .layerCount = 1,
	    },
	};

	cmdBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
}

vk::UniqueSemaphore CreateSemaphore(vk::Device device)
{
	return device.createSemaphoreUnique(vk::SemaphoreCreateInfo{}, s_allocator);
}

vk::UniqueFence CreateFence(vk::Device device, vk::FenceCreateFlags flags)
{
	return device.createFenceUnique(vk::FenceCreateInfo{.flags = flags}, s_allocator);
}

vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device)
{
	const auto createInfo = vk::CommandPoolCreateInfo{
	    .queueFamilyIndex = queueFamilyIndex,
	};

	return device.createCommandPoolUnique(createInfo, s_allocator);
}

vk::ImageAspectFlags GetImageAspect(vk::Format format)
{
	if (HasDepthComponent(format) && HasStencilComponent(format))
	{
		return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	}
	if (HasDepthComponent(format))
	{
		return vk::ImageAspectFlagBits::eDepth;
	}
	if (HasStencilComponent(format))
	{
		return vk::ImageAspectFlagBits::eStencil;
	}
	return vk::ImageAspectFlagBits::eColor;
}

void CopyBufferToImage(vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Image destination, vk::Format imageFormat, vk::Extent3D imageExtent)
{
	const auto copyRegion = vk::BufferImageCopy
	{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = GetImageAspect(imageFormat),
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = {0,0,0},
		.imageExtent = imageExtent,
	};
	cmdBuffer.copyBufferToImage(source, destination, vk::ImageLayout::eTransferDstOptimal, copyRegion);
}

void CopyImageToBuffer(
    vk::CommandBuffer cmdBuffer, vk::Image source, vk::Buffer destination, vk::Format imageFormat,
    vk::Extent3D imageExtent, std::uint32_t numMipLevels)
{
	const auto aspectMask = GetImageAspect(imageFormat);
	const auto pixelSize = GetFormatElementSize(imageFormat);

	// Copy each mip level with no gaps in between
	std::uint32_t offset = 0;
	vk::Extent3D mipExtent = imageExtent;
	for (auto i = 0u; i < numMipLevels; i++)
	{
		const auto copyRegion = vk::BufferImageCopy
		{
			.bufferOffset = offset,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = aspectMask,
				.mipLevel = i,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageOffset = {0,0,0},
			.imageExtent = mipExtent,
		};
		cmdBuffer.copyImageToBuffer(source, vk::ImageLayout::eTransferSrcOptimal, destination, copyRegion);

		offset += mipExtent.width * mipExtent.height * mipExtent.depth * pixelSize;
		mipExtent.width = std::max(1u, mipExtent.width / 2);
		mipExtent.height = std::max(1u, mipExtent.height / 2);
		mipExtent.depth = std::max(1u, mipExtent.depth / 2);
	}
}

std::uint32_t GetFormatElementSize(vk::Format format)
{
	switch (static_cast<VkFormat>(format))
	{
		default:
			assert("Unknown format!");
			return 0;

		/*
			8-bit
			Block size 1 byte
			1x1x1 block extent
			1 texel/block
		*/
		case VK_FORMAT_R4G4_UNORM_PACK8:
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
			return 1;

		/*
			16-bit
			Block size 2 byte
			1x1x1 block extent
			1 texel/block
		*/
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R10X6_UNORM_PACK16:
		case VK_FORMAT_R12X4_UNORM_PACK16:
			return 2;

		/*
			24-bit
			Block size 3 byte
			1x1x1 block extent
			1 texel/block
		*/
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
			return 3;

		/*
			32-bit
			Block size 4 byte
			1x1x1 block extent
			1 texel/block
		*/
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
		case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
			return 4;
	}
}
