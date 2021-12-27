
#include "Definitions.h"
#include "Framework/Buffer.h"
#include "Framework/MemoryAllocator.h"
#include "Framework/Renderer.h"
#include "Framework/Shader.h"
#include "Framework/Surface.h"
#include "Framework/Texture.h"
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
constexpr bool BreakOnVulkanWarning = false;
constexpr bool BreakOnVulkanError = true;

constexpr std::string_view VertexShader = R"--(
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outPosition;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outColor;

layout(set = 1, binding = 0) uniform ViewUniforms {
    mat4 viewProj;
} view;

layout(push_constant) uniform ObjectUniforms {
    mat4 model;
} object;

void main() {
    outPosition = mul(object.model, vec4(inPosition, 1.0)).xyz;
    gl_Position = mul(view.viewProj, vec4(outPosition, 1.0));
    outTexCoord = inTexCoord;
    outNormal = mul(object.model, vec4(inNormal, 0.0)).xyz;
    outColor = inColor;
})--";

constexpr std::string_view PixelShader = R"--(
layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inColor;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2DShadow shadowMapSampler;
layout(set = 2, binding = 1) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform GlobalUniforms {
    vec3 lightDir;
    vec3 lightColor;
    vec3 ambientColorTop;
    vec3 ambientColorBottom;
    mat4 shadowMatrix;
} ubo;

float textureProj(sampler2DShadow shadowMap, vec4 shadowCoord, vec2 off) {
    return texture(shadowMap, shadowCoord.xyz + vec3(off, 0.0)).r;
}

void main() {
    vec4 shadowCoord = mul(ubo.shadowMatrix, vec4(inPosition, 1.0));
    shadowCoord /= shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;

    ivec2 texDim = textureSize(shadowMapSampler, 0);
    float scale = 1.5;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    const int range = 1;
    int count = 0;
    float shadowFactor = 0.0;
    for (int x = -range; x <= range; x++) {
        for (int y = -range; y <= range; y++) {
            count++;
            shadowFactor += textureProj(shadowMapSampler, shadowCoord, vec2(dx*x, dy*y));
        }
    }
    const float lit = shadowFactor / count;

    const vec3 dirLight = clamp(dot(inNormal, -ubo.lightDir), 0.0, 1.0) * ubo.lightColor;
    const vec3 ambLight = mix(ubo.ambientColorBottom, ubo.ambientColorTop, inNormal.z * 0.5 + 0.5);
    const vec3 lighting = lit * dirLight + ambLight;

    const vec3 color = lighting * texture(texSampler, inTexCoord).rgb;
    outColor = vec4(color, 1.0);
})--";

struct Vertex
{
	Geo::Vector3 position;
	Geo::Vector2 texCoord;
	Geo::Vector3 normal;
	Geo::Vector3 color;
};

constexpr auto QuadVertices = std::array<Vertex, 4>{{
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
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
        .offset = offsetof(Vertex, normal),
    },
    vk::VertexInputAttributeDescription{
        .location = 3,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, color),
    },
};

struct GlobalUniforms
{
	Geo::Vector3 lightDir;
	float pad0 [[maybe_unused]];
	Geo::Vector3 lightColor;
	float pad1 [[maybe_unused]];
	Geo::Vector3 ambientColorTop;
	float pad2 [[maybe_unused]];
	Geo::Vector3 ambientColorBottom;
	float pad3 [[maybe_unused]];
	Geo::Matrix4 shadowMatrix;
};

struct ViewUniforms
{
	Geo::Matrix4 viewProj;
};

struct ObjectUniforms
{
	Geo::Matrix4 model;
};

constexpr auto ShaderLang = ShaderLanguage::Glsl;

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

constexpr auto DebugCreateInfo = [] {
	using MessageType = vk::DebugUtilsMessageTypeFlagBitsEXT;
	using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;

	return vk::DebugUtilsMessengerCreateInfoEXT{
	    .messageSeverity = MessageSeverity ::eError | MessageSeverity ::eWarning | MessageSeverity ::eInfo,
	    .messageType = MessageType::eGeneral | MessageType::eValidation | MessageType::ePerformance,
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
		throw VulkanError("No GPU found!");
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
		throw VulkanError("No suitable GPU found!");
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

template <class T, std::size_t N>
std::vector<T> ToVector(const std::array<T, N>& arr)
{
	auto ret = std::vector<T>(N);
	std::ranges::copy(arr, ret.begin());
	return ret;
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

struct UniformBuffer
{
	std::array<Buffer, MaxFramesInFlight> buffers;
	vk::DeviceSize size = 0;

	void SetData(int currentFrame, BytesView data) { buffers[currentFrame % MaxFramesInFlight].SetData(data); }
};

UniformBuffer CreateUniformBuffer(size_t bufferSize, vk::Device device, MemoryAllocator& allocator)
{
	UniformBuffer ret;
	ret.size = bufferSize;
	for (auto& buffer : ret.buffers)
	{
		buffer = CreateBuffer(
		    device, allocator, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	}
	return ret;
}

DescriptorSet CreateDescriptorSet(
    vk::DescriptorSetLayout layout, vk::DescriptorPool descriptorPool, vk::Device device,
    const UniformBuffer& uniformBuffer, std::span<const Texture* const> textures = {})
{
	const auto layouts = std::vector<vk::DescriptorSetLayout>(MaxFramesInFlight, layout);
	const auto allocInfo = vk::DescriptorSetAllocateInfo{
	    .descriptorPool = descriptorPool,
	    .descriptorSetCount = size32(layouts),
	    .pSetLayouts = data(layouts),
	};

	DescriptorSet descriptorSets = {device.allocateDescriptorSets(allocInfo)};

	for (size_t i = 0; i < layouts.size(); i++)
	{
		std::vector<vk::WriteDescriptorSet> descriptorWrites;

		if (uniformBuffer.size > 0)
		{
			const auto bufferInfo = vk::DescriptorBufferInfo{
			    .buffer = uniformBuffer.buffers[i].buffer.get(),
			    .offset = 0,
			    .range = uniformBuffer.size,
			};

			descriptorWrites.push_back({
			    .dstSet = descriptorSets.sets[i],
			    .dstBinding = 0,
			    .dstArrayElement = 0,
			    .descriptorCount = 1,
			    .descriptorType = vk::DescriptorType::eUniformBuffer,
			    .pBufferInfo = &bufferInfo,
			});
		}

		std::vector<vk::DescriptorImageInfo> imageInfos;
		imageInfos.resize(textures.size());
		for (const auto& texture : textures)
		{
			imageInfos.push_back({
			    .sampler = texture->sampler.get(),
			    .imageView = texture->imageView.get(),
			    .imageLayout = HasDepthOrStencilComponent(texture->format) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
			                                                               : vk::ImageLayout::eShaderReadOnlyOptimal,
			});

			descriptorWrites.push_back({
			    .dstSet = descriptorSets.sets[i],
			    .dstBinding = 1,
			    .dstArrayElement = 0,
			    .descriptorCount = 1,
			    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
			    .pImageInfo = &imageInfos.back(),
			});
		}

		device.updateDescriptorSets(descriptorWrites, {});
	}

	return descriptorSets;
}

vk::UniqueDescriptorSetLayout
CreateDescriptorSetLayout(vk::Device device, std::span<const vk::DescriptorSetLayoutBinding> layoutBindings)
{
	const auto createInfo = vk::DescriptorSetLayoutCreateInfo{
	    .bindingCount = size32(layoutBindings),
	    .pBindings = data(layoutBindings),
	};

	return device.createDescriptorSetLayoutUnique(createInfo, s_allocator);
}

vk::UniquePipeline CreateGraphicsPipeline(
    vk::ShaderModule vertexShader, vk::ShaderModule pixelShader, vk::PipelineLayout layout, vk::RenderPass renderPass,
    vk::SampleCountFlagBits sampleCount, vk::Device device, float depthBiasConstant = 0.0f, float depthBiasSlope = 0.0f)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	if (vertexShader)
	{
		shaderStages.push_back({.stage = vk::ShaderStageFlagBits::eVertex, .module = vertexShader, .pName = "main"});
	}
	if (pixelShader)
	{
		shaderStages.push_back({.stage = vk::ShaderStageFlagBits::eFragment, .module = pixelShader, .pName = "main"});
	}

	const auto vertexInput = vk::PipelineVertexInputStateCreateInfo{}
	                             .setVertexBindingDescriptions(VertexBindingDescription)
	                             .setVertexAttributeDescriptions(VertexAttributeDescriptions);

	const auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo{
	    .topology = vk::PrimitiveTopology::eTriangleList,
	    .primitiveRestartEnable = false,
	};

	// Viewport and scissor will be dynamic states, so their initial values don't matter
	const auto viewport = vk::Viewport{};
	const auto scissor = vk::Rect2D{};
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
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .frontFace = vk::FrontFace::eClockwise,
	    .depthBiasEnable = depthBiasConstant != 0.0f || depthBiasSlope != 0.0f,
	    .depthBiasConstantFactor = depthBiasConstant,
	    .depthBiasClamp = 0.0f,
	    .depthBiasSlopeFactor = depthBiasSlope,
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

	const auto dynamicStates = std::array{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

	const auto dynamicState = vk::PipelineDynamicStateCreateInfo{
	    .dynamicStateCount = size32(dynamicStates),
	    .pDynamicStates = data(dynamicStates),
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
	    .pColorBlendState = pixelShader ? &colorBlendState : nullptr,
	    .pDynamicState = &dynamicState,
	    .layout = layout,
	    .renderPass = renderPass,
	    .subpass = 0,
	};

	auto [result, pipeline] = device.createGraphicsPipelineUnique(nullptr, createInfo, s_allocator);
	if (result != vk::Result::eSuccess)
	{
		throw VulkanError("Couldn't create graphics pipeline");
	}
	return std::move(pipeline);
}

vk::UniquePipelineLayout
CreateGraphicsPipelineLayout(vk::Device device, Shader& shader, std::span<const vk::PushConstantRange> pushConstantRanges)
{
	const auto setLayouts = std::array{
	    shader.sceneDescriptorSetLayout.get(), shader.viewDescriptorSetLayout.get(),
	    shader.materialDescriptorSetLayout.get()};

	const auto createInfo = vk::PipelineLayoutCreateInfo{
	    .setLayoutCount = size32(setLayouts),
	    .pSetLayouts = data(setLayouts),
	    .pushConstantRangeCount = size32(pushConstantRanges),
	    .pPushConstantRanges = data(pushConstantRanges),
	};

	return device.createPipelineLayoutUnique(createInfo, s_allocator);
}

vk::UniqueRenderPass CreateShadowRenderPass(vk::Device device, vk::Format format)
{
	const auto attachment = vk::AttachmentDescription{
	    .format = format,
	    .samples = vk::SampleCountFlagBits::e1,
	    .loadOp = vk::AttachmentLoadOp::eClear,
	    .storeOp = vk::AttachmentStoreOp::eStore,
	    .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
	    .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
	    .initialLayout = vk::ImageLayout::eUndefined,
	    .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
	};

	const auto attachmentRef = vk::AttachmentReference{
	    .attachment = 0,
	    .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
	};

	const auto subpass = vk::SubpassDescription{
	    .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
	    .colorAttachmentCount = 0,
	    .pColorAttachments = nullptr,
	    .pResolveAttachments = nullptr,
	    .pDepthStencilAttachment = &attachmentRef,
	};

	const auto createInfo = vk::RenderPassCreateInfo{
	    .attachmentCount = 1,
	    .pAttachments = &attachment,
	    .subpassCount = 1,
	    .pSubpasses = &subpass,
	};

	return device.createRenderPassUnique(createInfo, s_allocator);
}

} // namespace

class Application
{
public:
	static constexpr Geo::Angle CameraRotateSpeed = 0.1_deg;
	static constexpr float CameraZoomSpeed = 0.002f;
	static constexpr float CameraMoveSpeed = 0.001f;

	explicit Application(SDL_Window* window, const char* imageFilename, const char* modelFilename) : m_window{window}
	{
		vk::DynamicLoader dl;
		VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

		m_instance = CreateInstance(window);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());

		if constexpr (IsDebugBuild)
		{
			m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(DebugCreateInfo, s_allocator);
		}

		auto surface = CreateVulkanSurface(window, m_instance.get());

		std::array deviceExtensions = {"VK_KHR_swapchain"};

		m_physicalDevice = FindPhysicalDevice(m_instance.get(), surface.get(), deviceExtensions);

		const auto queueFamilies = FindQueueFamilies(m_physicalDevice, surface.get());
		m_graphicsQueueFamily = queueFamilies.graphicsFamily.value();
		m_presentQueueFamily = queueFamilies.presentFamily.value();
		const auto queueFamilyIndices = std::vector{m_graphicsQueueFamily, m_presentQueueFamily};
		m_device = CreateDevice(m_physicalDevice, queueFamilyIndices, deviceExtensions);
		m_graphicsQueue = m_device->getQueue(m_graphicsQueueFamily, 0);
		m_generalAllocator.emplace(m_device.get(), m_physicalDevice);

		m_setupCommandPool = CreateCommandPool(m_graphicsQueueFamily, m_device.get());

		m_surface.emplace(
		    m_window, std::move(surface), m_device.get(), m_physicalDevice, queueFamilyIndices,
		    m_setupCommandPool.get(), m_graphicsQueue, UseMSAA);
		m_renderer.emplace(m_device.get(), m_graphicsQueueFamily, m_presentQueueFamily);

		const auto shaderData = CompileShader(VertexShader, PixelShader, ShaderLang);
		m_shader = std::make_unique<Shader>(CreateShader(shaderData, "ModelShader"));

		m_pipeline = CreateGraphicsPipeline(
		    m_shader->vertexShader.get(), m_shader->pixelShader.get(), m_shader->pipelineLayout.get(),
		    m_surface->GetVulkanRenderPass(), m_surface->GetSampleCount(), m_device.get());

		CreateMesh(modelFilename);
		CreateUniformBuffers();
		LoadTexture(imageFilename);
		CreateShadowMap();
		CreateDescriptorSets();

		spdlog::info("Vulkan initialised successfully");
	}

	void OnRender()
	{
		vk::Fence fence = m_renderer->BeginFrame(m_currentFrame);

		const auto result = m_surface->AcquireNextImage(fence);
		if (!result.has_value())
		{
			return;
		}
		const auto& image = result.value();

		const Geo::Matrix4 lightRotation = Geo::Matrix4::RotationZ(m_lightYaw) * Geo::Matrix4::RotationX(m_lightPitch);
		const Geo::Vector3 lightDirection = Geo::Normalise(lightRotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
		const Geo::Vector3 lightUp = Geo::Normalise(lightRotation * Geo::Vector3{0.0f, 0.0f, 1.0f});
		const Geo::Point3 shadowCameraPosition = Geo::Point3{} - lightDirection;

		const auto modelMatrix = Geo::Matrix4::RotationX(180.0_deg);

		const auto shadowCamView = Geo::LookAt(shadowCameraPosition, Geo::Point3{}, lightUp);
		const auto shadowCamProj = Geo::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

		const auto shadowMVP = shadowCamProj * shadowCamView * modelMatrix;

		const std::array<Geo::Point3, 8> bboxCorners = {
		    shadowMVP * Geo::Point3(m_meshBoundsMin.x, m_meshBoundsMin.y, m_meshBoundsMin.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMin.x, m_meshBoundsMin.y, m_meshBoundsMax.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMin.x, m_meshBoundsMax.y, m_meshBoundsMin.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMin.x, m_meshBoundsMax.y, m_meshBoundsMax.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMax.x, m_meshBoundsMin.y, m_meshBoundsMin.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMax.x, m_meshBoundsMin.y, m_meshBoundsMax.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMax.x, m_meshBoundsMax.y, m_meshBoundsMin.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMax.x, m_meshBoundsMax.y, m_meshBoundsMax.z),
		};

		const auto [minX, maxX] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::x));
		const auto [minY, maxY] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::y));
		const auto [minZ, maxZ] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::z));

		const auto shadowCamProjFitted = Geo::Orthographic(minX, maxX, maxY, minY, minZ, maxZ);

		m_shadowMatrix = shadowCamProjFitted * shadowCamView;

		// Update global uniforms
		const auto globalUniforms = GlobalUniforms{
		    .lightDir = Geo::Normalise(lightDirection),
		    .lightColor = {1.0f, 1.0f, 1.0f},
		    .ambientColorTop = {0.08f, 0.1f, 0.1f},
		    .ambientColorBottom = {0.003f, 0.003f, 0.002f},
		    .shadowMatrix = m_shadowMatrix,
		};
		m_globalUniformBuffer.SetData(m_currentFrame, globalUniforms);

		// Update object uniforms
		m_objectUniforms = {
		    .model = modelMatrix,
		};


		//
		// Pass 0. Draw shadows
		//
		{
			const auto clearDepthStencil = vk::ClearDepthStencilValue{1.0f, 0};
			const auto clearValues = std::array{
			    vk::ClearValue{clearDepthStencil},
			};

			// Update view uniforms
			const auto viewUniforms = ViewUniforms{
			    .viewProj = m_shadowMatrix,
			};

			m_viewUniformBuffers[0].SetData(m_currentFrame, viewUniforms);

			RenderList renderList = {
			    .renderPass = m_shadowRenderPass.get(),
			    .clearValues = clearValues,
			    .sceneDescriptorSet = &m_globalDescriptorSet,
			    .viewDescriptorSet = &m_viewDescriptorSets[0],
			    .objects = {{
			        .vertexBuffer = m_vertexBuffer.buffer.get(),
			        .indexBuffer = m_indexBuffer.buffer.get(),
			        .indexCount = m_indexCount,
			        .pipeline = m_shadowMapPipeline.get(),
			        .pipelineLayout = m_shader->pipelineLayout.get(),
			        .materialDescriptorSet = &m_materialDescriptorSet,
			        .pushConstants = m_objectUniforms,
			    }},
			};

			m_renderer->RenderToTexture(*m_shadowMap, m_shadowMapFramebuffer.get(), renderList);
		}

		//
		// Pass 1. Draw scene
		//
		{
			const auto clearColor = std::array{0.0f, 0.0f, 0.0f, 1.0f};
			const auto clearDepthStencil = vk::ClearDepthStencilValue{1.0f, 0};
			const auto clearValues = std::array{
			    vk::ClearValue{clearColor},
			    vk::ClearValue{clearDepthStencil},
			};

			// Update view uniforms
			const auto extent = m_surface->GetExtent();
			const float aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);

			const Geo::Matrix4 rotation = Geo::Matrix4::RotationZ(m_cameraYaw) * Geo::Matrix4::RotationX(m_cameraPitch);
			const Geo::Vector3 cameraDirection = Geo::Normalise(rotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
			const Geo::Vector3 cameraUp = Geo::Normalise(rotation * Geo::Vector3{0.0f, 0.0f, 1.0f});

			const Geo::Point3 cameraPosition = m_cameraTarget - cameraDirection * m_cameraDistance;

			const Geo::Matrix view = Geo::LookAt(cameraPosition, m_cameraTarget, cameraUp);
			const Geo::Matrix proj = Geo::Perspective(45.0_deg, aspectRatio, 0.1f, 10.0f);
			const Geo::Matrix viewProj = proj * view;

			const auto viewUniforms = ViewUniforms{
			    .viewProj = viewProj,
			};
			m_viewUniformBuffers[1].SetData(m_currentFrame, viewUniforms);

			RenderList renderList = {
			    .renderPass = m_surface->GetVulkanRenderPass(),
			    .clearValues = clearValues,
			    .sceneDescriptorSet = &m_globalDescriptorSet,
			    .viewDescriptorSet = &m_viewDescriptorSets[1],
			    .objects = {{
			        .vertexBuffer = m_vertexBuffer.buffer.get(),
			        .indexBuffer = m_indexBuffer.buffer.get(),
			        .indexCount = m_indexCount,
			        .pipeline = m_pipeline.get(),
			        .pipelineLayout = m_shader->pipelineLayout.get(),
			        .materialDescriptorSet = &m_materialDescriptorSet,
			        .pushConstants = m_objectUniforms,
			    }},
			};

			m_renderer->RenderToSurface(image, renderList);
		}

		m_renderer->EndFrame(image);

		m_currentFrame = (m_currentFrame + 1) % MaxFramesInFlight;
	}

	void OnResize() { m_surface->OnResize(); }

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

		if (SDL_GetModState() & KMOD_CTRL)
		{
			if (mouseButtonMask & SDL_BUTTON_LMASK)
			{
				m_lightYaw += static_cast<float>(mouseX) * CameraRotateSpeed;
				m_lightPitch -= static_cast<float>(mouseY) * CameraRotateSpeed;
			}
		}
		else
		{
			if (mouseButtonMask & SDL_BUTTON_LMASK)
			{
				m_cameraYaw += static_cast<float>(mouseX) * CameraRotateSpeed;
				m_cameraPitch -= static_cast<float>(mouseY) * CameraRotateSpeed;
			}
			if (mouseButtonMask & SDL_BUTTON_RMASK)
			{
				m_cameraDistance -= static_cast<float>(mouseX) * CameraZoomSpeed;
			}
			if (mouseButtonMask & SDL_BUTTON_MMASK)
			{
				const auto rotation = Geo::Matrix4::RotationZ(m_cameraYaw) * Geo::Matrix4::RotationX(m_cameraPitch);
				const auto cameraDirection = Geo::Normalise(rotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
				const auto cameraUp = Geo::Normalise(rotation * Geo::Vector3{0.0f, 0.0f, 1.0f});
				const auto cameraRight = Geo::Normalise(Geo::Cross(cameraUp, cameraDirection));

				m_cameraTarget += cameraRight * static_cast<float>(-mouseX) * CameraMoveSpeed;
				m_cameraTarget += cameraUp * static_cast<float>(mouseY) * CameraMoveSpeed;
			}
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
	auto OneShotCommands() { return OneShotCommandBuffer(m_device.get(), m_setupCommandPool.get(), m_graphicsQueue); }

	void CreateVertexBuffer(BytesView data)
	{
		m_vertexBuffer = CreateBufferWithData(
		    m_device.get(), m_generalAllocator.value(), m_setupCommandPool.get(), m_graphicsQueue, data,
		    vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
		SetDebugName(m_vertexBuffer.buffer, "VertexBuffer");
	}

	void CreateIndexBuffer(std::span<const uint16_t> data)
	{
		m_indexBuffer = CreateBufferWithData(
		    m_device.get(), m_generalAllocator.value(), m_setupCommandPool.get(), m_graphicsQueue, data,
		    vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
		SetDebugName(m_indexBuffer.buffer, "IndexBuffer");
		m_indexCount = size32(data);
	}

	void CreateMesh(const char* filename)
	{
		if (filename == nullptr)
		{
			CreateVertexBuffer(QuadVertices);
			CreateIndexBuffer(QuadIndices);

			m_meshBoundsMin = {-0.5f, -0.5f, 0.0f};
			m_meshBoundsMax = {0.5f, 0.5f, 0.0f};
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
				throw VulkanError(importer.GetErrorString());
			}

			if (scene->mNumMeshes == 0)
			{
				throw VulkanError(fmt::format("Model file '{}' contains no meshes", filename));
			}
			if (scene->mNumMeshes > 1)
			{
				throw VulkanError(fmt::format("Model file '{}' contains too many meshes", filename));
			}

			const aiMesh& mesh = **scene->mMeshes;

			std::vector<Vertex> vertices;
			vertices.reserve(mesh.mNumVertices);

			m_meshBoundsMin
			    = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
			m_meshBoundsMax
			    = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
			       std::numeric_limits<float>::lowest()};
			for (unsigned int i = 0; i < mesh.mNumVertices; i++)
			{
				const auto pos = mesh.mVertices[i];
				const auto uv = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][i] : aiVector3D{};
				const auto norm = mesh.HasNormals() ? mesh.mNormals[i] : aiVector3D{};
				const auto color = mesh.HasVertexColors(0) ? mesh.mColors[0][i] : aiColor4D{1, 1, 1, 1};
				vertices.push_back(Vertex{
				    .position = {pos.x, pos.y, pos.z},
				    .texCoord = {uv.x, uv.y},
				    .normal = {norm.x, norm.y, norm.z},
				    .color = {color.r, color.g, color.b},
				});

				m_meshBoundsMin.x = std::min(m_meshBoundsMin.x, pos.x);
				m_meshBoundsMin.y = std::min(m_meshBoundsMin.y, pos.y);
				m_meshBoundsMin.z = std::min(m_meshBoundsMin.z, pos.z);
				m_meshBoundsMax.x = std::max(m_meshBoundsMax.x, pos.x);
				m_meshBoundsMax.y = std::max(m_meshBoundsMax.y, pos.y);
				m_meshBoundsMax.z = std::max(m_meshBoundsMax.z, pos.z);
			}

			CreateVertexBuffer(vertices);

			std::vector<uint16_t> indices;
			indices.reserve(static_cast<std::size_t>(mesh.mNumFaces) * 3);

			for (unsigned int i = 0; i < mesh.mNumFaces; i++)
			{
				const auto& face = mesh.mFaces[i];
				assert(face.mNumIndices == 3);
				for (int j = 0; j < 3; j++)
				{
					if (face.mIndices[j] > std::numeric_limits<uint16_t>::max())
					{
						throw VulkanError("Too many vertices for 16-bit indices");
					}
					indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
				}
			}

			CreateIndexBuffer(indices);
		}
	}

	void CreateUniformBuffers()
	{
		m_globalUniformBuffer = CreateUniformBuffer(sizeof(GlobalUniforms), m_device.get(), *m_generalAllocator);

		for (uint32_t pass = 0; pass < m_passCount; pass++)
		{
			m_viewUniformBuffers.push_back(CreateUniformBuffer(sizeof(ViewUniforms), m_device.get(), *m_generalAllocator));
		}
	}

	void CreateDescriptorSets()
	{
		const auto poolSizes = std::array{
		    vk::DescriptorPoolSize{
		        .type = vk::DescriptorType::eUniformBuffer,
		        .descriptorCount = MaxFramesInFlight * (2 + m_passCount),
		    },
		    vk::DescriptorPoolSize{
		        .type = vk::DescriptorType::eCombinedImageSampler,
		        .descriptorCount = MaxFramesInFlight * (1 + m_passCount),
		    },
		};

		const auto poolCreateInfo = vk::DescriptorPoolCreateInfo{
			.maxSets = MaxFramesInFlight * (2 + m_passCount),
		}.setPoolSizes(poolSizes);

		auto descriptorPool = m_device->createDescriptorPoolUnique(poolCreateInfo, s_allocator);

		// Global descriptor set
		m_globalDescriptorSet = CreateDescriptorSet(
		    m_shader->sceneDescriptorSetLayout.get(), descriptorPool.get(), m_device.get(), m_globalUniformBuffer);

		// View desriptor sets
		m_viewDescriptorSets.clear();
		const auto shadowMapImages = std::array{m_shadowMap.get()};
		for (size_t i = 0; i < m_passCount; i++)
		{
			// Include shadow maps only after shadow pass
			const auto images = i >= 1 ? shadowMapImages : std::span<Texture* const>{};

			m_viewDescriptorSets.push_back(CreateDescriptorSet(
			    m_shader->viewDescriptorSetLayout.get(), descriptorPool.get(), m_device.get(), m_viewUniformBuffers[i],
			    images));
		}

		// Material desriptor set
		const auto materialTextures = std::array{m_texture.get()};
		m_materialDescriptorSet = CreateDescriptorSet(
		    m_shader->materialDescriptorSetLayout.get(), descriptorPool.get(), m_device.get(), UniformBuffer{},
		    materialTextures);

		m_descriptorPool = std::move(descriptorPool);
	}

	void LoadTexture(const char* filename)
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
			throw VulkanError(fmt::format("Error loading texture '{}'", filename));
		}

		const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(width) * height * 4;

		const auto data = TextureData{
		    .size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
		    .format = vk::Format::eR8G8B8A8Srgb,
		    .mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1,
		    .pixels = std::span(pixels.get(), imageSize),
		};

		m_texture = std::make_unique<Texture>(CreateTexture(data, filename));
	}

	void CreateShadowMap()
	{
		constexpr uint32_t shadowMapSize = 2048;

		const auto data = TextureData{
		    .size = {shadowMapSize, shadowMapSize},
		    .format = vk::Format::eD16Unorm,
		    .samplerInfo = {
		        .magFilter = vk::Filter::eLinear,
		        .minFilter = vk::Filter::eLinear,
		        .mipmapMode = vk::SamplerMipmapMode::eNearest,
		        .addressModeU = vk::SamplerAddressMode::eRepeat,
		        .addressModeV = vk::SamplerAddressMode::eRepeat,
		        .addressModeW = vk::SamplerAddressMode::eRepeat,
		        .anisotropyEnable = true,
		        .maxAnisotropy = m_physicalDevice.getProperties().limits.maxSamplerAnisotropy,
		        .compareEnable = true,
		        .compareOp = vk::CompareOp::eLess,
		        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
		    },
		};

		m_shadowMap = std::make_unique<Texture>(CreateRenderableTexture(data, "ShadowMap"));

		// Create render pass
		m_shadowRenderPass = CreateShadowRenderPass(m_device.get(), m_shadowMap->format);
		SetDebugName(m_shadowRenderPass, "ShadowRenderPass");

		// Create framebuffer
		const auto framebufferCreateInfo = vk::FramebufferCreateInfo{
		    .renderPass = m_shadowRenderPass.get(),
		    .attachmentCount = 1,
		    .pAttachments = &m_shadowMap->imageView.get(),
		    .width = m_shadowMap->size.width,
		    .height = m_shadowMap->size.height,
		    .layers = 1,
		};

		m_shadowMapFramebuffer = m_device->createFramebufferUnique(framebufferCreateInfo, s_allocator);

		// Depth bias (and slope) are used to avoid shadowing artifacts
		// Constant depth bias factor (always applied)
		float depthBiasConstant = 1.25f;
		// Slope depth bias factor, applied depending on polygon's slope
		float depthBiasSlope = 2.75f;

		// Create pipeline
		m_shadowMapPipeline = CreateGraphicsPipeline(
		    m_shader->vertexShader.get(), nullptr, m_shader->pipelineLayout.get(), m_shadowRenderPass.get(),
		    vk::SampleCountFlagBits::e1, m_device.get(), depthBiasConstant, depthBiasSlope);
	}

	//
	// Device functions
	//

	Shader CreateShader(const ShaderData& data, const char* debugName)
	{
		const auto vertexCreateInfo = vk::ShaderModuleCreateInfo{
		    .codeSize = data.vertexShaderSpirv.size() * sizeof(uint32_t),
		    .pCode = data.vertexShaderSpirv.data(),
		};
		const auto pixelCreateInfo = vk::ShaderModuleCreateInfo{
		    .codeSize = data.pixelShaderSpirv.size() * sizeof(uint32_t),
		    .pCode = data.pixelShaderSpirv.data(),
		};

		auto shader = Shader{
		    .vertexShader = m_device->createShaderModuleUnique(vertexCreateInfo, s_allocator),
		    .pixelShader = m_device->createShaderModuleUnique(pixelCreateInfo, s_allocator),
		    .sceneDescriptorSetLayout = CreateDescriptorSetLayout(m_device.get(), data.sceneBindings),
		    .viewDescriptorSetLayout = CreateDescriptorSetLayout(m_device.get(), data.viewBindings),
		    .materialDescriptorSetLayout = CreateDescriptorSetLayout(m_device.get(), data.materialBindings),
		};

		shader.pipelineLayout = CreateGraphicsPipelineLayout(m_device.get(), shader, data.pushConstantRanges);

		SetDebugName(shader.vertexShader, "{}:Vertex", debugName);
		SetDebugName(shader.pixelShader, "{}:Pixel", debugName);

		return shader;
	}

	Texture CreateTexture(const TextureData& data, const char* debugName)
	{
		auto cmdBuffer = OneShotCommands();

		auto texture = CreateTextureImpl(data, vk::ImageUsageFlagBits::eSampled, cmdBuffer, debugName);

		if (data.mipLevelCount > 1)
		{
			texture.GenerateMipmaps(cmdBuffer);
		}
		else
		{
			// Transition into samplable image, but only if mipmaps were not generated
			// (if so, the mipmap generation already transitioned to image to be samplable)
			texture.TransitionToShaderInput(cmdBuffer);
		}

		return texture;
	}

	Texture CreateRenderableTexture(const TextureData& data, const char* debugName)
	{
		const auto renderUsage = HasDepthOrStencilComponent(data.format) ? vk::ImageUsageFlagBits::eDepthStencilAttachment
		                                                                 : vk::ImageUsageFlagBits::eColorAttachment;

		auto cmdBuffer = OneShotCommands();

		auto texture = CreateTextureImpl(data, renderUsage | vk::ImageUsageFlagBits::eSampled, cmdBuffer, debugName);

		if (renderUsage & vk::ImageUsageFlagBits::eColorAttachment)
		{
			texture.TransitionToColorTarget(cmdBuffer);
		}
		else if (renderUsage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
		{
			texture.TransitionToDepthStencilTarget(cmdBuffer);
		}

		return texture;
	}

	Texture CreateTextureImpl(const TextureData& data, vk::ImageUsageFlags usage, OneShotCommandBuffer& cmdBuffer, const char* debugName)
	{
		if (!data.pixels.empty())
		{
			// Need to transfer image data into image
			usage |= vk::ImageUsageFlagBits::eTransferDst;
		}

		if (data.mipLevelCount > 1)
		{
			// Need to transfer pixels between mip levels in order to generate mipmaps
			usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
		}

		auto initialLayout = vk::ImageLayout::eUndefined;

		// Create image
		const auto imageExtent = vk::Extent3D{data.size.width, data.size.height, 1};
		const auto imageInfo = vk::ImageCreateInfo{
		    .imageType = vk::ImageType::e2D,
		    .format = data.format,
		    .extent = imageExtent,
		    .mipLevels = data.mipLevelCount,
		    .arrayLayers = 1,
		    .samples = data.samples,
		    .tiling = vk::ImageTiling::eOptimal,
		    .usage = usage,
		    .sharingMode = vk::SharingMode::eExclusive,
		    .initialLayout = initialLayout,
		};

		auto image = m_device->createImageUnique(imageInfo, s_allocator);
		const auto memory = m_generalAllocator->Allocate(
		    m_device->getImageMemoryRequirements(image.get()), vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_device->bindImageMemory(image.get(), memory.memory, memory.offset);

		if (!data.pixels.empty())
		{
			// Create staging buffer
			auto stagingBuffer = CreateBuffer(
			    m_device.get(), *m_generalAllocator, data.pixels.size(), vk::BufferUsageFlagBits::eTransferSrc,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			stagingBuffer.SetData(data.pixels);

			// Copy staging buffer to image
			TransitionImageLayout(
			    cmdBuffer, image.get(), imageInfo.format, data.mipLevelCount, imageInfo.initialLayout,
			    vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
			    vk::PipelineStageFlagBits::eTransfer);
			initialLayout = vk::ImageLayout::eTransferDstOptimal;
			CopyBufferToImage(cmdBuffer, stagingBuffer.buffer.get(), image.get(), imageExtent);

			cmdBuffer.TakeOwnership(std::move(stagingBuffer.buffer));
		}

		// Create image view
		const auto viewInfo = vk::ImageViewCreateInfo{
			.image = image.get(),
			.viewType = vk::ImageViewType::e2D,
			.format = data.format,
			.subresourceRange = {
				.aspectMask =  GetImageAspect(data.format),
				.baseMipLevel = 0,
				.levelCount = data.mipLevelCount,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
		auto imageView = m_device->createImageViewUnique(viewInfo, s_allocator);
		auto sampler = m_device->createSamplerUnique(data.samplerInfo, s_allocator);

		auto ret = Texture{
		    .image = std::move(image),
		    .memory = memory,
		    .imageView = std::move(imageView),
		    .sampler = std::move(sampler),
		    .size = {imageExtent.width, imageExtent.height},
		    .format = data.format,
		    .mipLevelCount = data.mipLevelCount,
		    .samples = data.samples,
		    .layout = initialLayout,
		};

		SetDebugName(ret.image, debugName);
		SetDebugName(ret.imageView, "{}:View", debugName);
		SetDebugName(ret.sampler, "{}:Sampler", debugName);

		return ret;
	}

	SDL_Window* m_window;

	std::chrono::high_resolution_clock::time_point m_startTime = std::chrono::high_resolution_clock::now();

	vk::UniqueInstance m_instance;
	vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_device;

	std::optional<MemoryAllocator> m_generalAllocator; // Allocator for objects that live a long time

	std::optional<Surface> m_surface;
	uint32_t m_graphicsQueueFamily;
	uint32_t m_presentQueueFamily;
	vk::Queue m_graphicsQueue;
	vk::UniqueDescriptorPool m_descriptorPool;
	vk::UniqueCommandPool m_setupCommandPool;

	// Object setup
	std::unique_ptr<Shader> m_shader;
	vk::UniquePipeline m_pipeline;
	Geo::Point3 m_meshBoundsMin;
	Geo::Point3 m_meshBoundsMax;
	Buffer m_vertexBuffer;
	Buffer m_indexBuffer;
	uint32_t m_indexCount;
	std::unique_ptr<Texture> m_texture;

	const uint32_t m_passCount = 2;
	UniformBuffer m_globalUniformBuffer;
	std::vector<UniformBuffer> m_viewUniformBuffers;
	ObjectUniforms m_objectUniforms;
	DescriptorSet m_globalDescriptorSet;
	std::vector<DescriptorSet> m_viewDescriptorSets;
	DescriptorSet m_materialDescriptorSet;

	// Lights
	Geo::Angle m_lightYaw = 45.0_deg;
	Geo::Angle m_lightPitch = -45.0_deg;
	std::unique_ptr<Texture> m_shadowMap;
	vk::UniqueRenderPass m_shadowRenderPass;
	vk::UniqueFramebuffer m_shadowMapFramebuffer;
	vk::UniquePipeline m_shadowMapPipeline;
	Geo::Matrix4 m_shadowMatrix;

	// Camera
	Geo::Point3 m_cameraTarget = {0.0f, 0.0f, 0.0f};
	Geo::Angle m_cameraYaw = 45.0_deg;
	Geo::Angle m_cameraPitch = -45.0_deg;
	float m_cameraDistance = 3.0f;

	// Rendering
	std::optional<Renderer> m_renderer;
	uint32_t m_currentFrame = 0;
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
		std::string message = fmt::format("The following error occurred when initializing SDL: {}", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}

	std::optional<Application> application;
	try
	{
		application.emplace(window.get(), imageFilename, modelFilename);
	}
	catch (const vk::Error& e)
	{
		spdlog::critical("Vulkan error: {}", e.what());
		std::string message = fmt::format("The following error occurred when initializing Vulkan:\n{}", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}
	catch (const CompileError& e)
	{
		spdlog::critical("Shader compilation error: {}", e.what());
		std::string message = fmt::format("The following error occurred when compiling shaders:\n{}", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}

	application->MainLoop();

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
