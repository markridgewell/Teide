
#include "Framework/GraphicsDevice.h"

#include "Framework/Shader.h"
#include "Framework/Texture.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include <cassert>

namespace
{
constexpr bool BreakOnVulkanWarning = false;
constexpr bool BreakOnVulkanError = true;

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

void CopyBuffer(vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Buffer destination, vk::DeviceSize size)
{
	const auto copyRegion = vk::BufferCopy{
	    .srcOffset = 0,
	    .dstOffset = 0,
	    .size = size,
	};
	cmdBuffer.copyBuffer(source, destination, copyRegion);
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

} // namespace

//---------------------------------------------------------------------------------------------------------------------

GraphicsDevice::GraphicsDevice(SDL_Window* window)
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
	const auto queueFamilyIndices = std::array{m_graphicsQueueFamily, m_presentQueueFamily};
	m_device = CreateDevice(m_physicalDevice, queueFamilyIndices, deviceExtensions);
	m_graphicsQueue = m_device->getQueue(m_graphicsQueueFamily, 0);
	m_allocator.emplace(m_device.get(), m_physicalDevice);

	m_setupCommandPool = CreateCommandPool(m_graphicsQueueFamily, m_device.get());

	m_pendingWindowSurfaces[window] = std::move(surface);

	// TODO: Don't hardcode descriptor pool sizes
	const auto poolSizes = std::array{
	    vk::DescriptorPoolSize{
	        .type = vk::DescriptorType::eUniformBuffer,
	        .descriptorCount = MaxFramesInFlight * 10,
	    },
	    vk::DescriptorPoolSize{
	        .type = vk::DescriptorType::eCombinedImageSampler,
	        .descriptorCount = MaxFramesInFlight * 10,
	    },
	};

	const auto poolCreateInfo = vk::DescriptorPoolCreateInfo{
			.maxSets = MaxFramesInFlight * 10,
		}.setPoolSizes(poolSizes);

	m_descriptorPool = m_device->createDescriptorPoolUnique(poolCreateInfo, s_allocator);
}

Surface GraphicsDevice::CreateSurface(SDL_Window* window, bool multisampled)
{
	vk::UniqueSurfaceKHR surface;

	const auto it = m_pendingWindowSurfaces.find(window);
	if (it != m_pendingWindowSurfaces.end())
	{
		surface = std::move(it->second);
		m_pendingWindowSurfaces.erase(window);
	}
	else
	{
		surface = CreateVulkanSurface(window, m_instance.get());
	}

	return Surface(
	    window, std::move(surface), m_device.get(), m_physicalDevice, {m_graphicsQueueFamily, m_presentQueueFamily},
	    m_setupCommandPool.get(), m_graphicsQueue, multisampled);
}

Renderer GraphicsDevice::CreateRenderer()
{
	return Renderer(m_device.get(), m_graphicsQueueFamily, m_presentQueueFamily);
}

Buffer GraphicsDevice::CreateBuffer(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, const char* name)
{
	Buffer ret{};

	const auto createInfo = vk::BufferCreateInfo{
	    .size = size,
	    .usage = usage,
	    .sharingMode = vk::SharingMode::eExclusive,
	};
	ret.buffer = m_device->createBufferUnique(createInfo, s_allocator);
	ret.allocation = m_allocator->Allocate(m_device->getBufferMemoryRequirements(ret.buffer.get()), memoryFlags);
	m_device->bindBufferMemory(ret.buffer.get(), ret.allocation.memory, ret.allocation.offset);

	SetDebugName(ret.buffer, name);

	return ret;
}

Buffer GraphicsDevice::CreateBufferWithData(
    BytesView data, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, const char* name)
{
	if ((memoryFlags & vk::MemoryPropertyFlagBits::eHostCoherent) == vk::MemoryPropertyFlags{})
	{
		// Create staging buffer
		auto stagingBuffer = CreateBuffer(
		    data.size(), vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "");
		stagingBuffer.SetData(data);

		// Create device-local buffer
		Buffer ret = CreateBuffer(data.size(), usage | vk::BufferUsageFlagBits::eTransferDst, memoryFlags, name);
		CopyBuffer(OneShotCommands(), stagingBuffer.buffer.get(), ret.buffer.get(), data.size());
		return ret;
	}
	else
	{
		Buffer ret = CreateBuffer(data.size(), vk::BufferUsageFlagBits::eTransferSrc, memoryFlags, name);
		ret.SetData(data);
		return ret;
	}
}

UniformBuffer GraphicsDevice::CreateUniformBuffer(vk::DeviceSize bufferSize, const char* name)
{
	UniformBuffer ret;
	ret.size = bufferSize;
	for (auto& buffer : ret.buffers)
	{
		buffer = CreateBuffer(
		    bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, name);
	}
	return ret;
}

Shader GraphicsDevice::CreateShader(const ShaderData& data, const char* name)
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

	SetDebugName(shader.vertexShader, "{}:Vertex", name);
	SetDebugName(shader.pixelShader, "{}:Pixel", name);

	return shader;
}

Texture GraphicsDevice::CreateTexture(const TextureData& data, const char* name)
{
	auto cmdBuffer = OneShotCommands();

	auto texture = CreateTextureImpl(data, vk::ImageUsageFlagBits::eSampled, cmdBuffer, name);

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

Texture GraphicsDevice::CreateRenderableTexture(const TextureData& data, const char* name)
{
	const auto renderUsage = HasDepthOrStencilComponent(data.format) ? vk::ImageUsageFlagBits::eDepthStencilAttachment
	                                                                 : vk::ImageUsageFlagBits::eColorAttachment;

	auto cmdBuffer = OneShotCommands();

	auto texture = CreateTextureImpl(data, renderUsage | vk::ImageUsageFlagBits::eSampled, cmdBuffer, name);

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

Texture GraphicsDevice::CreateTextureImpl(
    const TextureData& data, vk::ImageUsageFlags usage, OneShotCommandBuffer& cmdBuffer, const char* name)
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
	const auto memory
	    = m_allocator->Allocate(m_device->getImageMemoryRequirements(image.get()), vk::MemoryPropertyFlagBits::eDeviceLocal);
	m_device->bindImageMemory(image.get(), memory.memory, memory.offset);

	if (!data.pixels.empty())
	{
		// Create staging buffer
		auto stagingBuffer = CreateBuffer(
		    data.pixels.size(), vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "");
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

	SetDebugName(ret.image, name);
	SetDebugName(ret.imageView, "{}:View", name);
	SetDebugName(ret.sampler, "{}:Sampler", name);

	return ret;
}

DescriptorSet GraphicsDevice::CreateDescriptorSet(
    vk::DescriptorSetLayout layout, const UniformBuffer& uniformBuffer, std::span<const Texture* const> textures)
{
	const auto layouts = std::vector<vk::DescriptorSetLayout>(MaxFramesInFlight, layout);
	const auto allocInfo = vk::DescriptorSetAllocateInfo{
	    .descriptorPool = m_descriptorPool.get(),
	    .descriptorSetCount = size32(layouts),
	    .pSetLayouts = data(layouts),
	};

	DescriptorSet descriptorSets = {m_device->allocateDescriptorSets(allocInfo)};

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

		m_device->updateDescriptorSets(descriptorWrites, {});
	}

	return descriptorSets;
}

vk::UniqueFramebuffer GraphicsDevice::CreateFramebuffer(const vk::FramebufferCreateInfo& createInfo) const
{
	return m_device->createFramebufferUnique(createInfo);
}

vk::UniqueRenderPass GraphicsDevice::CreateRenderPass(const vk::RenderPassCreateInfo& createInfo) const
{
	return m_device->createRenderPassUnique(createInfo, s_allocator);
}
