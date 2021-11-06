
#include "Definitions.h"
#include "GeoLib/Matrix.h"
#include "GeoLib/Vector.h"
#include "ShaderCompiler.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
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

using namespace Geo::Literals;

namespace
{
constexpr bool UseMSAA = true;
constexpr vk::DeviceSize MemoryBlockSize = 64 * 1024 * 1024;

template <class T>
uint32_t size32(const T& cont)
{
	using std::size;
	return static_cast<uint32_t>(size(cont));
}

constexpr std::string_view VertexShader = R"--(
#version 450

layout(location = 0) in vec3 inPosition;
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
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
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
	Geo::Vector3 position;
	Geo::Vector2 texCoord;
	Geo::Vector3 color;
};

constexpr auto QuadVertices = std::array<Vertex, 4>{{
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
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
        .format = vk::Format::eR32G32B32Sfloat,
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
			spdlog::debug("{}{}", prefix, pCallbackData->pMessage);
			break;

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

	vk::ApplicationInfo applicationInfo{
	    .apiVersion = VK_API_VERSION_1_0,
	};

	const auto availableLayers = vk::enumerateInstanceLayerProperties();
	const auto availableExtensions = vk::enumerateInstanceExtensionProperties();

	std::vector<const char*> layers;
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
		    DebugCreateInfo,
		};

		return vk::createInstanceUnique(createInfo.get<vk::InstanceCreateInfo>(), s_allocator);
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
	    .queueCreateInfoCount = size32(queueCreateInfos),
	    .pQueueCreateInfos = data(queueCreateInfos),
	    .enabledLayerCount = size32(layers),
	    .ppEnabledLayerNames = data(layers),
	    .enabledExtensionCount = size32(extensions),
	    .ppEnabledExtensionNames = data(extensions),
	    .pEnabledFeatures = &deviceFeatures};

	return physicalDevice.createDeviceUnique(deviceCreateInfo, s_allocator);
}

class OneShotCommandBuffer
{
public:
	explicit OneShotCommandBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue queue) : m_queue{queue}
	{
		const auto allocInfo = vk::CommandBufferAllocateInfo{
		    .commandPool = commandPool,
		    .level = vk::CommandBufferLevel::ePrimary,
		    .commandBufferCount = 1,
		};
		auto cmdBuffers = device.allocateCommandBuffersUnique(allocInfo);
		m_cmdBuffer = std::move(cmdBuffers.front());

		m_cmdBuffer->begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	}

	~OneShotCommandBuffer()
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
    vk::SurfaceFormatKHR surfaceFormat, vk::Extent2D surfaceExtent, vk::Device device, vk::SwapchainKHR oldSwapchain)
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

	throw CustomError("Failed to find a suitable format");
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

vk::UniqueBuffer CreateBuffer(vk::Device device, vk::DeviceSize size, vk::BufferUsageFlags usage)
{
	const auto createInfo = vk::BufferCreateInfo{
	    .size = size,
	    .usage = usage,
	    .sharingMode = vk::SharingMode::eExclusive,
	};
	return device.createBufferUnique(createInfo, s_allocator);
}

struct MemoryAllocation
{
	vk::DeviceMemory memory;
	vk::DeviceSize offset;
};

// The world's simplest memory allocator
class MemoryAllocator
{
public:
	explicit MemoryAllocator(vk::Device device, vk::PhysicalDevice physicalDevice) :
	    m_device{device}, m_physicalDevice{physicalDevice}
	{}

	MemoryAllocation Allocate(const vk::MemoryRequirements& requirements, vk::MemoryPropertyFlags flags)
	{
		const uint32_t memoryType = FindMemoryType(m_physicalDevice, requirements.memoryTypeBits, flags);

		MemoryBlock& block = FindMemoryBlock(memoryType, requirements.size, requirements.alignment);
		const auto offset = (((block.consumedSize - 1) / requirements.alignment) + 1) * requirements.alignment;
		if (offset + requirements.size > block.capacity)
		{
			throw CustomError("Out of memory!");
		}
		block.consumedSize = offset + requirements.size;
		return {block.memory.get(), offset};
	}

	void DeallocateAll()
	{
		for (MemoryBlock& block : m_memoryBlocks)
		{
			block.consumedSize = 0;
		}
	}

private:
	struct MemoryBlock
	{
		vk::DeviceSize capacity;
		vk::DeviceSize consumedSize;
		uint32_t memoryType;
		vk::UniqueDeviceMemory memory;
	};

	MemoryBlock& FindMemoryBlock(uint32_t memoryType, vk::DeviceSize availableSize, vk::DeviceSize alignment)
	{
		const auto it = std::ranges::find_if(m_memoryBlocks, [=](const MemoryBlock& block) {
			if (block.memoryType != memoryType)
			{
				return false;
			}

			// Check if the block has enough space for the allocation
			const auto offset = (((block.consumedSize - 1) / alignment) + 1) * alignment;
			return offset + availableSize <= block.capacity;
		});

		if (it == m_memoryBlocks.end())
		{
			// No compatible memory block found; create a new one

			// Make sure the new block has at least enough space for the allocation
			const auto newBlockSize = std::max(MemoryBlockSize, availableSize);

			const auto allocInfo = vk::MemoryAllocateInfo{
			    .allocationSize = newBlockSize,
			    .memoryTypeIndex = memoryType,
			};

			spdlog::info("Allocating {} bytes of memory in memory type {}", allocInfo.allocationSize, allocInfo.memoryTypeIndex);

			m_memoryBlocks.push_back({
			    .capacity = newBlockSize,
			    .consumedSize = 0,
			    .memoryType = memoryType,
			    .memory = m_device.allocateMemoryUnique(allocInfo),
			});

			return m_memoryBlocks.back();
		}
		return *it;
	}

	vk::Device m_device;
	vk::PhysicalDevice m_physicalDevice;
	std::vector<MemoryBlock> m_memoryBlocks;
};


template <class T>
concept Span = requires(T t)
{
	{std::span{t}};
};

template <class T>
concept TrivialSpan
    = Span<T> && std::is_trivially_copyable_v<typename T::value_type> && std::is_standard_layout_v<typename T::value_type>;

template <class T>
concept TrivialObject
    = !Span<T> && std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> && !std::is_pointer_v<T>;

class BytesView
{
public:
	BytesView() = default;
	BytesView(const BytesView&) = default;
	BytesView(BytesView&&) = default;
	BytesView& operator=(const BytesView&) = default;
	BytesView& operator=(BytesView&&) = default;

	BytesView(std::span<const std::byte> bytes) : m_span{bytes} {}

	template <TrivialSpan T>
	BytesView(const T& data) : m_span{std::as_bytes(std::span(data))}
	{}

	template <TrivialObject T>
	BytesView(const T& data) : m_span{std::as_bytes(std::span(&data, 1))}
	{}

	auto data() const { return m_span.data(); }
	auto size() const { return m_span.size(); }
	auto begin() const { return m_span.begin(); }
	auto end() const { return m_span.end(); }
	auto rbegin() const { return m_span.rbegin(); }
	auto rend() const { return m_span.rend(); }

private:
	std::span<const std::byte> m_span;
};

void SetBufferData(vk::Device device, MemoryAllocation allocation, BytesView data)
{
	void* mappedData = device.mapMemory(allocation.memory, allocation.offset, data.size());
	memcpy(mappedData, data.data(), data.size());
	device.unmapMemory(allocation.memory);
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

struct BufferWithAllocation
{
	vk::UniqueBuffer buffer;
	MemoryAllocation allocation;
};

BufferWithAllocation CreateBufferWithData(
    vk::Device device, MemoryAllocator& allocator, vk::CommandPool commandPool, vk::Queue queue, BytesView data,
    vk::BufferUsageFlags usage)
{
	BufferWithAllocation ret{};

	// Create staging buffer
	const auto stagingBuffer = CreateBuffer(device, data.size(), vk::BufferUsageFlagBits::eTransferSrc);
	const auto stagingAlloc = allocator.Allocate(
	    device.getBufferMemoryRequirements(stagingBuffer.get()),
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	device.bindBufferMemory(stagingBuffer.get(), stagingAlloc.memory, stagingAlloc.offset);
	SetBufferData(device, stagingAlloc, data);

	// Create vertex buffer
	ret.buffer = CreateBuffer(device, data.size(), usage | vk::BufferUsageFlagBits::eTransferDst);
	ret.allocation = allocator.Allocate(
	    device.getBufferMemoryRequirements(ret.buffer.get()),
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	device.bindBufferMemory(ret.buffer.get(), ret.allocation.memory, ret.allocation.offset);
	CopyBuffer(OneShotCommandBuffer(device, commandPool, queue), stagingBuffer.get(), ret.buffer.get(), data.size());

	return ret;
}

struct TransitionAccessMasks
{
	vk::AccessFlags source;
	vk::AccessFlags destination;
};

TransitionAccessMasks GetTransitionAccessMasks(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	using enum vk::ImageLayout;
	using Access = vk::AccessFlagBits;

	if (oldLayout == eUndefined && newLayout == eTransferDstOptimal)
	{
		return {{}, Access::eTransferWrite};
	}
	else if (oldLayout == eTransferDstOptimal && newLayout == eShaderReadOnlyOptimal)
	{
		return {Access::eTransferWrite, Access::eShaderRead};
	}
	else if (oldLayout == eUndefined && newLayout == eDepthAttachmentOptimal)
	{
		return {{}, Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite};
	}

	assert("Unsupported image transition");
	return {};
}

vk::ImageAspectFlags GetAspectMask(vk::Format format)
{
	if (HasDepthComponent(format))
	{
		if (HasStencilComponent(format))
		{
			return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		}
		else
		{
			return vk::ImageAspectFlagBits::eDepth;
		}
	}
	else if (HasStencilComponent(format))
	{
		return vk::ImageAspectFlagBits::eStencil;
	}
	else
	{
		return vk::ImageAspectFlagBits::eColor;
	}
}

void TransitionImageLayout(
    vk::CommandBuffer cmdBuffer, vk::Image image, vk::Format format, uint32_t mipLevelCount, vk::ImageLayout oldLayout,
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
	        .aspectMask = GetAspectMask(format),
	        .baseMipLevel = 0,
	        .levelCount = mipLevelCount,
	        .baseArrayLayer = 0,
	        .layerCount = 1,
	    },
	};

	cmdBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
}

void GenerateMipmaps(vk::CommandBuffer cmdBuffer, vk::Image image, uint32_t width, uint32_t height, uint32_t mipLevelCount)
{
	const auto makeBarrier = [&](vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
	                             vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevel) {
		return vk::ImageMemoryBarrier{
		    .srcAccessMask = srcAccessMask,
		    .dstAccessMask = dstAccessMask,
		    .oldLayout = oldLayout,
		    .newLayout = newLayout,
		    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .image = image,
		    .subresourceRange = {
		        .aspectMask = vk::ImageAspectFlagBits::eColor,
		        .baseMipLevel = mipLevel,
		        .levelCount = 1,
		        .baseArrayLayer = 0,
		        .layerCount = 1,
		    }};
	};

	const auto origin = vk::Offset3D{0, 0, 0};
	auto prevMipSize = vk::Offset3D{static_cast<int32_t>(width), static_cast<int32_t>(height), 1};

	// Iterate all mip levels starting at 1
	for (uint32_t i = 1; i < mipLevelCount; i++)
	{
		const auto currMipSize = vk::Offset3D{
		    prevMipSize.x > 1 ? prevMipSize.x / 2 : 1,
		    prevMipSize.y > 1 ? prevMipSize.y / 2 : 1,
		    prevMipSize.z > 1 ? prevMipSize.z / 2 : 1,
		};

		// Transition previous mip level to be a transfer source
		const auto beforeBarrier = makeBarrier(
		    vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, vk::ImageLayout::eTransferDstOptimal,
		    vk::ImageLayout::eTransferSrcOptimal, i - 1);

		cmdBuffer.pipelineBarrier(
		    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, beforeBarrier);

		// Blit previous mip to current mip
		const auto blit = vk::ImageBlit{
		    .srcSubresource = {
		        .aspectMask = vk::ImageAspectFlagBits::eColor,
		        .mipLevel = i - 1,
		        .baseArrayLayer = 0,
		        .layerCount = 1,
		    },
		    .srcOffsets = {{origin, prevMipSize}},
		    .dstSubresource = {
		        .aspectMask = vk::ImageAspectFlagBits::eColor,
		        .mipLevel = i,
		        .baseArrayLayer = 0,
		        .layerCount = 1,
		    },
		    .dstOffsets = {{origin, currMipSize}},
		};

		cmdBuffer.blitImage(
		    image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit,
		    vk::Filter::eLinear);

		// Transition previous mip level to be ready for reading in shader
		const auto afterBarrier = makeBarrier(
		    vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferSrcOptimal,
		    vk::ImageLayout::eShaderReadOnlyOptimal, i - 1);

		cmdBuffer.pipelineBarrier(
		    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, afterBarrier);

		prevMipSize = currMipSize;
	}

	// Transition final mip level to be ready for reading in shader
	const auto finalBarrier = makeBarrier(
	    vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferDstOptimal,
	    vk::ImageLayout::eShaderReadOnlyOptimal, mipLevelCount - 1);

	cmdBuffer.pipelineBarrier(
	    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, finalBarrier);
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
    vk::Extent2D surfaceExtent, vk::SampleCountFlagBits sampleCount, vk::Device device)
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
	    .rasterizationSamples = sampleCount,
	    .sampleShadingEnable = false,
	    .minSampleShading = 1.0f,
	    .pSampleMask = nullptr,
	    .alphaToCoverageEnable = false,
	    .alphaToOneEnable = false,
	};

	const auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo{
	    .depthTestEnable = true,
	    .depthWriteEnable = true,
	    .depthCompareOp = vk::CompareOp::eLess,
	    .depthBoundsTestEnable = false,
	    .stencilTestEnable = false,
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
	    .stageCount = size32(shaderStages),
	    .pStages = data(shaderStages),
	    .pVertexInputState = &vertexInput,
	    .pInputAssemblyState = &inputAssembly,
	    .pViewportState = &viewportState,
	    .pRasterizationState = &rasterizationState,
	    .pMultisampleState = &multisampleState,
	    .pDepthStencilState = &depthStencilState,
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

	for (const size_t i : std::views::iota(0u, imageViews.size()))
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
	}

	return framebuffers;
}

} // namespace

class VulkanApp
{
public:
	static constexpr Geo::Angle CameraRotateSpeed = 0.1_deg;
	static constexpr float CameraZoomSpeed = 0.002f;
	static constexpr float CameraMoveSpeed = 0.001f;

	explicit VulkanApp(SDL_Window* window, const char* imageFilename, const char* modelFilename) : m_window{window}
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

		if (UseMSAA)
		{
			const auto deviceLimits = m_physicalDevice.getProperties().limits;
			const auto supportedSampleCounts
			    = deviceLimits.framebufferColorSampleCounts & deviceLimits.framebufferDepthSampleCounts;
			m_msaaSampleCount = vk::SampleCountFlagBits{std::bit_floor(static_cast<uint32_t>(supportedSampleCounts))};
		}

		const auto queueFamilies = FindQueueFamilies(m_physicalDevice, m_surface.get());
		m_graphicsQueueFamily = queueFamilies.graphicsFamily.value();
		m_presentQueueFamily = queueFamilies.presentFamily.value();
		const auto queueFamilyIndices = std::array{m_graphicsQueueFamily, m_presentQueueFamily};
		m_device = CreateDevice(m_physicalDevice, queueFamilyIndices, deviceExtensions);
		m_graphicsQueue = m_device->getQueue(m_graphicsQueueFamily, 0);
		m_presentQueue = m_device->getQueue(m_presentQueueFamily, 0);
		m_generalAllocator.emplace(m_device.get(), m_physicalDevice);
		m_swapchainAllocator.emplace(m_device.get(), m_physicalDevice);

		m_graphicsCommandPool = CreateCommandPool(m_graphicsQueueFamily, m_device.get());
		std::ranges::generate(m_imageAvailable, [this] { return CreateSemaphore(m_device.get()); });
		std::ranges::generate(m_renderFinished, [this] { return CreateSemaphore(m_device.get()); });
		std::ranges::generate(m_inFlightFences, [this] { return CreateFence(m_device.get()); });

		m_descriptorSetLayout = CreateDescriptorSetLayout(m_device.get());
		m_pipelineLayout = CreateGraphicsPipelineLayout(m_device.get(), m_descriptorSetLayout.get());

		m_vertexShader = CreateShaderModule(VertexShader, ShaderStage::Vertex, ShaderLang, m_device.get());
		m_pixelShader = CreateShaderModule(PixelShader, ShaderStage::Pixel, ShaderLang, m_device.get());

		CreateSwapchainAndImages();

		CreateMesh(modelFilename);
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

		const Geo::Matrix4 rotation = Geo::Matrix4::RotationZ(m_cameraYaw) * Geo::Matrix4::RotationX(m_cameraPitch);
		const Geo::Vector4 cameraDir4 = rotation * Geo::Vector4{0.0f, 1.0f, 0.0f, 0.0f};
		const Geo::Vector3 cameraDirection = Geo::Normalise(Geo::Vector3{cameraDir4.x, cameraDir4.y, cameraDir4.z});
		const Geo::Vector4 cameraUp4 = rotation * Geo::Vector4{0.0f, 0.0f, 1.0f, 0.0f};
		const Geo::Vector3 cameraUp = Geo::Normalise(Geo::Vector3{cameraUp4.x, cameraUp4.y, cameraUp4.z});

		const Geo::Point3 cameraPosition = m_cameraTarget - cameraDirection * m_cameraDistance;

		const auto ubo = UniformBufferObject{
		    .model = Geo::Matrix4::Identity(),
		    .view = Geo::LookAt(cameraPosition, m_cameraTarget, cameraUp),
		    .proj = Geo::Perspective(45.0_deg, aspectRatio, 0.1f, 10.0f),
		};

		SetBufferData(m_device.get(), m_uniformBuffersMemory[imageIndex], ubo);

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

	void OnResize() { RecreateSwapchain(); }

	bool OnEvent(const SDL_Event& event)
	{
		switch (event.type)
		{
			case SDL_QUIT:
				return false;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						OnResize();
						break;
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				SDL_SetRelativeMouseMode(SDL_TRUE);
				break;

			case SDL_MOUSEBUTTONUP:
				SDL_SetRelativeMouseMode(SDL_FALSE);
				break;
		}
		return true;
	}

	bool OnUpdate()
	{
		int mouseX{}, mouseY{};
		const uint32_t mouseButtonMask = SDL_GetRelativeMouseState(&mouseX, &mouseY);

		if (mouseButtonMask & SDL_BUTTON_LMASK)
		{
			m_cameraYaw += static_cast<float>(mouseX) * CameraRotateSpeed;
			m_cameraPitch += static_cast<float>(mouseY) * CameraRotateSpeed;
		}
		if (mouseButtonMask & SDL_BUTTON_RMASK)
		{
			m_cameraDistance -= static_cast<float>(mouseX) * CameraZoomSpeed;
		}
		if (mouseButtonMask & SDL_BUTTON_MMASK)
		{
			const auto rotation = Geo::Matrix4::RotationZ(m_cameraYaw) * Geo::Matrix4::RotationX(m_cameraPitch);
			const auto cameraDir4 = rotation * Geo::Vector4{0.0f, 1.0f, 0.0f, 0.0f};
			const auto cameraDirection = Geo::Normalise(Geo::Vector3{cameraDir4.x, cameraDir4.y, cameraDir4.z});
			const auto cameraUp4 = rotation * Geo::Vector4{0.0f, 0.0f, 1.0f, 0.0f};
			const auto cameraUp = Geo::Normalise(Geo::Vector3{cameraUp4.x, cameraUp4.y, cameraUp4.z});
			const auto cameraRight = Geo::Normalise(Geo::Cross(cameraUp, cameraDirection));

			m_cameraTarget += cameraRight * static_cast<float>(-mouseX) * CameraMoveSpeed;
			m_cameraTarget += cameraUp * static_cast<float>(mouseY) * CameraMoveSpeed;
		}
		return true;
	}

	void MainLoop()
	{
		bool running = true;
		while (running)
		{
			SDL_Event event;
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
	auto OneShotCommands()
	{
		return OneShotCommandBuffer(m_device.get(), m_graphicsCommandPool.get(), m_graphicsQueue);
	}

	void CreateSwapchainAndImages()
	{
		const auto surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
		const auto surfaceFormat = ChooseSurfaceFormat(m_physicalDevice.getSurfaceFormatsKHR(m_surface.get()));
		m_surfaceExtent = ChooseSwapExtent(surfaceCapabilities, m_window);
		const auto queueFamilyIndices = std::array{m_graphicsQueueFamily, m_presentQueueFamily};
		m_swapchain = CreateSwapchain(
		    m_physicalDevice, queueFamilyIndices, m_surface.get(), surfaceFormat, m_surfaceExtent, m_device.get(),
		    m_swapchain.get());
		m_swapchainImages = m_device->getSwapchainImagesKHR(m_swapchain.get());
		m_swapchainImageViews = CreateSwapchainImageViews(surfaceFormat.format, m_swapchainImages, m_device.get());
		m_imagesInFlight.resize(m_swapchainImages.size());

		if (UseMSAA)
		{
			CreateColorBuffer(surfaceFormat.format);
		}
		CreateDepthBuffer();

		m_renderPass = CreateRenderPass(m_device.get(), surfaceFormat.format, m_depthFormat, m_msaaSampleCount);
		m_swapchainFramebuffers = CreateFramebuffers(
		    m_swapchainImageViews, m_colorImageView.get(), m_depthImageView.get(), m_renderPass.get(), m_surfaceExtent,
		    m_device.get());

		m_pipeline = CreateGraphicsPipeline(
		    m_vertexShader.get(), m_pixelShader.get(), m_pipelineLayout.get(), m_renderPass.get(), m_surfaceExtent,
		    m_msaaSampleCount, m_device.get());
	}

	void CreateColorBuffer(vk::Format format)
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

		m_colorImage = m_device->createImageUnique(imageInfo, s_allocator);
		const auto allocation = m_swapchainAllocator->Allocate(
		    m_device->getImageMemoryRequirements(m_colorImage.get()), vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_device->bindImageMemory(m_colorImage.get(), allocation.memory, allocation.offset);

		TransitionImageLayout(
		    OneShotCommands(), m_colorImage.get(), imageInfo.format, 1, vk::ImageLayout::eUndefined,
		    vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
		    vk::PipelineStageFlagBits::eEarlyFragmentTests);

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
		m_colorImageView = m_device->createImageViewUnique(viewInfo, s_allocator);
	}

	void CreateDepthBuffer()
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

		m_depthImage = m_device->createImageUnique(imageInfo, s_allocator);
		const auto allocation = m_swapchainAllocator->Allocate(
		    m_device->getImageMemoryRequirements(m_depthImage.get()), vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_device->bindImageMemory(m_depthImage.get(), allocation.memory, allocation.offset);
		TransitionImageLayout(
		    OneShotCommands(), m_depthImage.get(), imageInfo.format, 1, vk::ImageLayout::eUndefined,
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
		m_depthImageView = m_device->createImageViewUnique(viewInfo, s_allocator);
	}

	void CreateVertexBuffer(BytesView data)
	{
		auto result = CreateBufferWithData(
		    m_device.get(), m_generalAllocator.value(), m_graphicsCommandPool.get(), m_graphicsQueue, data,
		    vk::BufferUsageFlagBits::eVertexBuffer);
		m_vertexBuffer = std::move(result.buffer);
		m_vertexBufferMemory = std::move(result.allocation);
	}

	void CreateIndexBuffer(std::span<const uint16_t> data)
	{
		auto result = CreateBufferWithData(
		    m_device.get(), m_generalAllocator.value(), m_graphicsCommandPool.get(), m_graphicsQueue, data,
		    vk::BufferUsageFlagBits::eIndexBuffer);
		m_indexBuffer = std::move(result.buffer);
		m_indexBufferMemory = std::move(result.allocation);
		m_indexCount = size32(data);
	}

	void CreateMesh(const char* filename)
	{
		if (filename == nullptr)
		{
			CreateVertexBuffer(QuadVertices);
			CreateIndexBuffer(QuadIndices);
		}
		else
		{
			Assimp::Importer importer;

			const auto importFlags = aiProcess_CalcTangentSpace | aiProcess_Triangulate
			    | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_RemoveComponent
			    | aiProcess_RemoveRedundantMaterials | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph
			    | aiProcess_ConvertToLeftHanded | aiProcess_FlipWindingOrder;

			const aiScene* scene = importer.ReadFile(filename, importFlags);
			if (!scene)
			{
				throw CustomError(importer.GetErrorString());
			}

			if (scene->mNumMeshes == 0)
			{
				throw CustomError(std::format("Model file '{}' contains no meshes"));
			}
			if (scene->mNumMeshes > 1)
			{
				throw CustomError(std::format("Model file '{}' contains too many meshes"));
			}

			const aiMesh& mesh = **scene->mMeshes;

			std::vector<Vertex> vertices;
			vertices.reserve(mesh.mNumVertices);

			for (unsigned int i = 0; i < mesh.mNumVertices; i++)
			{
				const auto pos = mesh.mVertices[i];
				const auto uv = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][i] : aiVector3D{};
				const auto color = mesh.HasVertexColors(0) ? mesh.mColors[0][i] : aiColor4D{1, 1, 1, 1};
				vertices.push_back(Vertex{
				    .position = {pos.x, pos.y, pos.z},
				    .texCoord = {uv.x, uv.y},
				    .color = {color.r, color.g, color.b},
				});
			}

			CreateVertexBuffer(vertices);

			std::vector<uint16_t> indices;
			indices.reserve(mesh.mNumFaces * 3);

			for (unsigned int i = 0; i < mesh.mNumFaces; i++)
			{
				const auto& face = mesh.mFaces[i];
				assert(face.mNumIndices == 3);
				for (int j = 0; j < 3; j++)
				{
					if (face.mIndices[j] > std::numeric_limits<uint16_t>::max())
					{
						throw CustomError("Too many vertices for 16-bit indices");
					}
					indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
				}
			}

			CreateIndexBuffer(indices);
		}
	}

	void CreateUniformBuffers()
	{
		const auto bufferSize = sizeof(UniformBufferObject);

		m_uniformBuffers.resize(m_swapchainImages.size());
		m_uniformBuffersMemory.resize(m_swapchainImages.size());
		for (size_t i = 0; i < m_swapchainImages.size(); i++)
		{
			m_uniformBuffers[i] = CreateBuffer(m_device.get(), bufferSize, vk::BufferUsageFlagBits::eUniformBuffer);
			m_uniformBuffersMemory[i] = m_generalAllocator->Allocate(
			    m_device->getBufferMemoryRequirements(m_uniformBuffers[i].get()),
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			m_device->bindBufferMemory(
			    m_uniformBuffers[i].get(), m_uniformBuffersMemory[i].memory, m_uniformBuffersMemory[i].offset);
		}
	}

	void CreateDescriptorSets()
	{
		auto descriptorPool = CreateDescriptorPool(m_device.get(), static_cast<uint32_t>(m_swapchainImages.size()));

		const auto layouts = std::vector<vk::DescriptorSetLayout>(m_swapchainImages.size(), m_descriptorSetLayout.get());
		const auto allocInfo = vk::DescriptorSetAllocateInfo{
		    .descriptorPool = descriptorPool.get(),
		    .descriptorSetCount = size32(layouts),
		    .pSetLayouts = data(layouts),
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

		const vk::DeviceSize imageSize = width * height * 4;
		const auto mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		// Create staging buffer
		const auto stagingBuffer = CreateBuffer(m_device.get(), imageSize, vk::BufferUsageFlagBits::eTransferSrc);
		const auto stagingAlloc = m_generalAllocator->Allocate(
		    m_device->getBufferMemoryRequirements(stagingBuffer.get()),
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		m_device->bindBufferMemory(stagingBuffer.get(), stagingAlloc.memory, stagingAlloc.offset);
		SetBufferData(m_device.get(), stagingAlloc, std::span(pixels.get(), imageSize));

		// Create image
		const auto imageExtent = vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
		const auto imageInfo = vk::ImageCreateInfo{
		    .imageType = vk::ImageType::e2D,
		    .format = vk::Format::eR8G8B8A8Srgb,
		    .extent = imageExtent,
		    .mipLevels = mipLevelCount,
		    .arrayLayers = 1,
		    .samples = vk::SampleCountFlagBits::e1,
		    .tiling = vk::ImageTiling::eOptimal,
		    .usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
		        | vk::ImageUsageFlagBits::eSampled,
		    .sharingMode = vk::SharingMode::eExclusive,
		    .initialLayout = vk::ImageLayout::eUndefined,
		};

		m_textureImage = m_device->createImageUnique(imageInfo, s_allocator);
		m_textureMemory = m_generalAllocator->Allocate(
		    m_device->getImageMemoryRequirements(m_textureImage.get()), vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_device->bindImageMemory(m_textureImage.get(), m_textureMemory.memory, m_textureMemory.offset);
		const auto cmdBuffer = OneShotCommands();
		TransitionImageLayout(
		    cmdBuffer, m_textureImage.get(), imageInfo.format, mipLevelCount, imageInfo.initialLayout,
		    vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
		    vk::PipelineStageFlagBits::eTransfer);
		CopyBufferToImage(cmdBuffer, stagingBuffer.get(), m_textureImage.get(), imageExtent);

		GenerateMipmaps(cmdBuffer, m_textureImage.get(), width, height, mipLevelCount);

		// Create image view
		const auto viewInfo = vk::ImageViewCreateInfo{
			.image = m_textureImage.get(),
			.viewType = vk::ImageViewType::e2D,
			.format = imageInfo.format,
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = mipLevelCount,
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
		const auto clearDepthStencil = vk::ClearDepthStencilValue{1.0f, 0};
		const auto clearValues = std::array{
		    vk::ClearValue{clearColor},
		    vk::ClearValue{clearDepthStencil},
		};

		const auto renderPassBegin = vk::RenderPassBeginInfo{
		    .renderPass = m_renderPass.get(),
		    .framebuffer = framebuffer,
		    .renderArea = {.offset = {0, 0}, .extent = surfaceExtent},
		    .clearValueCount = size32(clearValues),
		    .pClearValues = data(clearValues),
		};

		const auto descriptorSet = m_descriptorSets[imageIndex];

		commandBuffer.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
		commandBuffer.bindVertexBuffers(0, m_vertexBuffer.get(), vk::DeviceSize{0});
		commandBuffer.bindIndexBuffer(m_indexBuffer.get(), vk::DeviceSize{0}, vk::IndexType::eUint16);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSet, {});
		commandBuffer.drawIndexed(m_indexCount, 1, 0, 0, 0);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

	void CreateCommandBuffers()
	{
		const auto allocateInfo = vk::CommandBufferAllocateInfo{
		    .commandPool = m_graphicsCommandPool.get(),
		    .level = vk::CommandBufferLevel::ePrimary,
		    .commandBufferCount = size32(m_swapchainFramebuffers),
		};

		m_commandBuffers = m_device->allocateCommandBuffersUnique(allocateInfo);

		for (const size_t i : std::views::iota(0u, m_commandBuffers.size()))
		{
			DrawObjects(m_commandBuffers[i].get(), m_swapchainFramebuffers[i].get(), m_surfaceExtent, i);
		}
	}

	void RecreateSwapchain()
	{
		m_swapchainAllocator->DeallocateAll();

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

	std::optional<MemoryAllocator> m_generalAllocator; // Allocator for objects that live a long time
	std::optional<MemoryAllocator> m_swapchainAllocator; // Allocator for objects that need to be recreated with the swapchain

	vk::UniqueSurfaceKHR m_surface;
	uint32_t m_graphicsQueueFamily;
	uint32_t m_presentQueueFamily;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;
	vk::UniqueSwapchainKHR m_swapchain;
	std::vector<vk::Image> m_swapchainImages;
	std::vector<vk::UniqueImageView> m_swapchainImageViews;
	vk::SampleCountFlagBits m_msaaSampleCount = vk::SampleCountFlagBits::e1;
	vk::UniqueImage m_colorImage;
	MemoryAllocation m_colorMemory;
	vk::UniqueImageView m_colorImageView;
	vk::Format m_depthFormat;
	vk::UniqueImage m_depthImage;
	MemoryAllocation m_depthMemory;
	vk::UniqueImageView m_depthImageView;
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
	MemoryAllocation m_vertexBufferMemory;
	vk::UniqueBuffer m_indexBuffer;
	uint32_t m_indexCount;
	MemoryAllocation m_indexBufferMemory;
	vk::UniqueImage m_textureImage;
	MemoryAllocation m_textureMemory;
	vk::UniqueImageView m_textureImageView;
	vk::UniqueSampler m_textureSampler;
	std::vector<vk::UniqueBuffer> m_uniformBuffers;
	std::vector<MemoryAllocation> m_uniformBuffersMemory;
	std::vector<vk::DescriptorSet> m_descriptorSets;
	std::vector<vk::UniqueFramebuffer> m_swapchainFramebuffers;
	std::vector<vk::UniqueCommandBuffer> m_commandBuffers;

	// Camera
	Geo::Point3 m_cameraTarget = {0.0f, 0.0f, 0.0f};
	Geo::Angle m_cameraYaw = 45.0_deg;
	Geo::Angle m_cameraPitch = 45.0_deg;
	float m_cameraDistance = 3.0f;

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
	const char* const imageFilename = (argc >= 2) ? argv[1] : nullptr;
	const char* const modelFilename = (argc >= 3) ? argv[2] : nullptr;

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
		vulkan.emplace(window.get(), imageFilename, modelFilename);
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
