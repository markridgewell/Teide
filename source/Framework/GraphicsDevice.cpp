
#include "Framework/GraphicsDevice.h"

#include "Framework/Buffer.h"
#include "Framework/Shader.h"
#include "Framework/Texture.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include <cassert>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace
{
constexpr bool BreakOnVulkanWarning = false;
constexpr bool BreakOnVulkanError = true;

static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

vk::UniqueSurfaceKHR CreateVulkanSurface(SDL_Window* window, vk::Instance instance)
{
	spdlog::info("Creating a Vulkan surface for a window");
	VkSurfaceKHR surfaceTmp = {};
	if (!SDL_Vulkan_CreateSurface(window, instance, &surfaceTmp))
	{
		throw VulkanError("Failed to create Vulkan surface for window");
	}
	assert(surfaceTmp);
	spdlog::info("Surface created successfully");
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

	bool IsComplete(bool needPresent) const
	{
		return graphicsFamily.has_value() && (!needPresent || presentFamily.has_value());
	}
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

		if (surface && physicalDevice.getSurfaceSupportKHR(i, surface))
		{
			indices.presentFamily = i;
		}

		if (indices.IsComplete(surface))
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

		if (surface)
		{
			// Check for adequate swap chain support
			if (device.getSurfaceFormatsKHR(surface).empty())
			{
				return false;
			}
			if (device.getSurfacePresentModesKHR(surface).empty())
			{
				return false;
			}
		}

		// Check that all required queue families are supported
		const auto indices = FindQueueFamilies(device, surface);
		return indices.IsComplete(surface);
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

void SetBufferData(Buffer& buffer, BytesView data)
{
	assert(buffer.allocation.mappedData);
	memcpy(buffer.allocation.mappedData, data.data(), data.size());
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

Buffer CreateBufferWithData(
    BytesView data, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, vk::Device device,
    MemoryAllocator& allocator, vk::CommandPool commandPool, vk::Queue queue)
{
	if ((memoryFlags & vk::MemoryPropertyFlagBits::eHostCoherent) == vk::MemoryPropertyFlags{})
	{
		// Create staging buffer
		auto stagingBuffer = CreateBufferUninitialized(
		    data.size(), vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, device, allocator);
		SetBufferData(stagingBuffer, data);

		// Create device-local buffer
		Buffer ret = CreateBufferUninitialized(
		    data.size(), usage | vk::BufferUsageFlagBits::eTransferDst, memoryFlags, device, allocator);
		const auto cmdBuffer = OneShotCommandBuffer(device, commandPool, queue);
		CopyBuffer(cmdBuffer, stagingBuffer.buffer.get(), ret.buffer.get(), data.size());
		return ret;
	}
	else
	{
		Buffer ret
		    = CreateBufferUninitialized(data.size(), vk::BufferUsageFlagBits::eTransferSrc, memoryFlags, device, allocator);
		SetBufferData(ret, data);
		return ret;
	}
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

vk::UniquePipeline CreateGraphicsPipeline(
    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates, vk::RenderPass renderPass,
    vk::Format format, vk::SampleCountFlagBits sampleCount, vk::Device device)
{
	const auto vertexShader = shader.vertexShader.get();
	const auto pixelShader = shader.pixelShader.get();

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back({.stage = vk::ShaderStageFlagBits::eVertex, .module = vertexShader, .pName = "main"});
	if (!HasDepthOrStencilComponent(format))
	{
		shaderStages.push_back({.stage = vk::ShaderStageFlagBits::eFragment, .module = pixelShader, .pName = "main"});
	}

	const auto vertexInput = vk::PipelineVertexInputStateCreateInfo{
	    .vertexBindingDescriptionCount = size32(vertexLayout.vertexInputBindings),
	    .pVertexBindingDescriptions = data(vertexLayout.vertexInputBindings),
	    .vertexAttributeDescriptionCount = size32(vertexLayout.vertexInputAttributes),
	    .pVertexAttributeDescriptions = data(vertexLayout.vertexInputAttributes),
	};

	const auto viewportState = vk::PipelineViewportStateCreateInfo{
	    .viewportCount = 1,
	    .pViewports = &renderStates.viewport,
	    .scissorCount = 1,
	    .pScissors = &renderStates.scissor,
	};

	const auto multisampleState = vk::PipelineMultisampleStateCreateInfo{
	    .rasterizationSamples = sampleCount,
	    .sampleShadingEnable = false,
	    .minSampleShading = 1.0f,
	    .pSampleMask = nullptr,
	    .alphaToCoverageEnable = false,
	    .alphaToOneEnable = false,
	};

	const auto colorBlendState = vk::PipelineColorBlendStateCreateInfo{
	    .logicOpEnable = false,
	    .attachmentCount = 1,
	    .pAttachments = &renderStates.colorBlendAttachment,
	};

	const auto dynamicState = vk::PipelineDynamicStateCreateInfo{
	    .dynamicStateCount = size32(renderStates.dynamicStates),
	    .pDynamicStates = data(renderStates.dynamicStates),
	};
	const auto createInfo = vk::GraphicsPipelineCreateInfo{
	    .stageCount = size32(shaderStages),
	    .pStages = data(shaderStages),
	    .pVertexInputState = &vertexInput,
	    .pInputAssemblyState = &vertexLayout.inputAssembly,
	    .pViewportState = &viewportState,
	    .pRasterizationState = &renderStates.rasterizationState,
	    .pMultisampleState = &multisampleState,
	    .pDepthStencilState = &renderStates.depthStencilState,
	    .pColorBlendState = pixelShader ? &colorBlendState : nullptr,
	    .pDynamicState = &dynamicState,
	    .layout = shader.pipelineLayout.get(),
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

vk::SubpassDescription MakeSubpassDescription(const vk::AttachmentReference& attachmentRef, bool isColorTarget)
{
	if (isColorTarget)
	{
		return vk::SubpassDescription{
		    .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		    .colorAttachmentCount = 1,
		    .pColorAttachments = &attachmentRef,
		};
	}
	else
	{
		return vk::SubpassDescription{
		    .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		    .pDepthStencilAttachment = &attachmentRef,
		};
	}
};

} // namespace

//---------------------------------------------------------------------------------------------------------------------

GraphicsDevice::GraphicsDevice(SDL_Window* window)
{
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	spdlog::info("Loaded Vulkan library");

	m_instance = CreateInstance(window);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());
	spdlog::info("Creatad Vulkan instance");

	if constexpr (IsDebugBuild)
	{
		m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(DebugCreateInfo, s_allocator);
	}

	vk::UniqueSurfaceKHR surface;
	if (window)
	{
		surface = CreateVulkanSurface(window, m_instance.get());
	}
	else
	{
		spdlog::info("No window provided (headless mode");
	}

	std::vector<const char*> deviceExtensions;
	if (window)
	{
		deviceExtensions.push_back("VK_KHR_swapchain");
	}
	m_physicalDevice = FindPhysicalDevice(m_instance.get(), surface.get(), deviceExtensions);

	const auto queueFamilies = FindQueueFamilies(m_physicalDevice, surface.get());
	m_graphicsQueueFamily = queueFamilies.graphicsFamily.value();
	m_presentQueueFamily = queueFamilies.presentFamily;
	if (window)
	{
		assert(m_presentQueueFamily.has_value());
	}
	std::vector<uint32_t> queueFamilyIndices;
	queueFamilyIndices.push_back(m_graphicsQueueFamily);
	if (m_presentQueueFamily)
	{
		queueFamilyIndices.push_back(*m_presentQueueFamily);
	};
	m_device = CreateDevice(m_physicalDevice, queueFamilyIndices, deviceExtensions);
	m_graphicsQueue = m_device->getQueue(m_graphicsQueueFamily, 0);
	m_allocator.emplace(m_device.get(), m_physicalDevice);

	m_setupCommandPool = CreateCommandPool(m_graphicsQueueFamily, m_device.get());
	SetDebugName(m_setupCommandPool, "SetupCommandPool");

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
	    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
	    .maxSets = MaxFramesInFlight * 10,
	    .poolSizeCount = size32(poolSizes),
	    .pPoolSizes = data(poolSizes),
	};

	m_descriptorPool = m_device->createDescriptorPoolUnique(poolCreateInfo, s_allocator);
	SetDebugName(m_descriptorPool, "DescriptorPool");
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
		spdlog::info("Creating a new surface for a window");
		surface = CreateVulkanSurface(window, m_instance.get());
	}

	if (!m_presentQueueFamily.has_value())
	{
		const auto queueFamilies = FindQueueFamilies(m_physicalDevice, surface.get());
		m_presentQueueFamily = queueFamilies.presentFamily;
		assert(m_presentQueueFamily && "GraphicsDevice cannot create a surface for this window");
	}

	return Surface(
	    window, std::move(surface), m_device.get(), m_physicalDevice, {m_graphicsQueueFamily, *m_presentQueueFamily},
	    m_setupCommandPool.get(), m_graphicsQueue, multisampled);
}

Renderer GraphicsDevice::CreateRenderer()
{
	return Renderer(*this, m_graphicsQueueFamily, m_presentQueueFamily);
}

BufferPtr GraphicsDevice::CreateBuffer(const BufferData& data, const char* name)
{
	auto ret = CreateBufferWithData(
	    data.data, data.usage, data.memoryFlags, m_device.get(), *m_allocator, m_setupCommandPool.get(), m_graphicsQueue);
	SetDebugName(ret.buffer, name);
	return std::make_shared<const Buffer>(std::move(ret));
}

DynamicBufferPtr GraphicsDevice::CreateDynamicBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags usage, const char* name)
{
	DynamicBuffer ret;
	ret.size = bufferSize;
	for (auto& buffer : ret.buffers)
	{
		buffer = CreateBufferUninitialized(
		    bufferSize, usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		    m_device.get(), *m_allocator);
		SetDebugName(buffer.buffer, name);
	}
	return std::make_shared<DynamicBuffer>(std::move(ret));
}

ShaderPtr GraphicsDevice::CreateShader(const ShaderData& data, const char* name)
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
	SetDebugName(shader.pipelineLayout, "{}:PipelineLayout", name);

	return std::make_shared<const Shader>(std::move(shader));
}

TexturePtr GraphicsDevice::CreateTexture(const TextureData& data, const char* name)
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

	return std::make_shared<const Texture>(std::move(texture));
}

RenderableTexturePtr GraphicsDevice::CreateRenderableTexture(const TextureData& data, const char* name)
{
	const bool isColorTarget = !HasDepthOrStencilComponent(data.format);
	const auto renderUsage
	    = isColorTarget ? vk::ImageUsageFlagBits::eColorAttachment : vk::ImageUsageFlagBits::eDepthStencilAttachment;

	const auto usage = renderUsage
	    | vk::ImageUsageFlagBits::eSampled
	    // For now, all renderable textures will be created with TransferSrc so they can be copied from
	    | vk::ImageUsageFlagBits::eTransferSrc;

	auto cmdBuffer = OneShotCommands();

	auto texture = RenderableTexture{{CreateTextureImpl(data, usage, cmdBuffer, name)}};

	if (isColorTarget)
	{
		texture.TransitionToColorTarget(cmdBuffer);
	}
	else
	{
		texture.TransitionToDepthStencilTarget(cmdBuffer);
	}

	// Create render pass
	const auto attachment = vk::AttachmentDescription{
	    .format = texture.format,
	    .samples = vk::SampleCountFlagBits::e1,
	    .loadOp = vk::AttachmentLoadOp::eClear,
	    .storeOp = vk::AttachmentStoreOp::eStore,
	    .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
	    .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
	    .initialLayout = vk::ImageLayout::eUndefined,
	    .finalLayout = texture.layout,
	};

	const auto attachmentRef = vk::AttachmentReference{
	    .attachment = 0,
	    .layout = texture.layout,
	};

	const auto subpass = MakeSubpassDescription(attachmentRef, isColorTarget);

	const auto createInfo = vk::RenderPassCreateInfo{
	    .attachmentCount = 1,
	    .pAttachments = &attachment,
	    .subpassCount = 1,
	    .pSubpasses = &subpass,
	};

	texture.renderPass = m_device->createRenderPassUnique(createInfo);
	SetDebugName(texture.renderPass, "{}:RenderPass", name);

	// Create framebuffer
	const auto framebufferCreateInfo = vk::FramebufferCreateInfo{
	    .renderPass = texture.renderPass.get(),
	    .attachmentCount = 1,
	    .pAttachments = &texture.imageView.get(),
	    .width = texture.size.width,
	    .height = texture.size.height,
	    .layers = 1,
	};

	texture.framebuffer = m_device->createFramebufferUnique(framebufferCreateInfo, s_allocator);
	SetDebugName(texture.framebuffer, "{}:Framebuffer", name);

	return std::make_shared<RenderableTexture>(std::move(texture));
}

PipelinePtr GraphicsDevice::CreatePipeline(
    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates, const Surface& surface)
{
	return std::make_shared<const Pipeline>(
	    CreateGraphicsPipeline(
	        shader, vertexLayout, renderStates, surface.GetVulkanRenderPass(), surface.GetColorFormat(),
	        surface.GetSampleCount(), m_device.get()),
	    shader.pipelineLayout.get());
}

PipelinePtr GraphicsDevice::CreatePipeline(
    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates, const RenderableTexture& texture)
{
	return std::make_shared<const Pipeline>(
	    CreateGraphicsPipeline(
	        shader, vertexLayout, renderStates, texture.renderPass.get(), texture.format, texture.samples, m_device.get()),
	    shader.pipelineLayout.get());
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
	auto initialPipelineStage = vk::PipelineStageFlagBits::eTopOfPipe;

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
		auto stagingBuffer = CreateBufferUninitialized(
		    data.pixels.size(), vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_device.get(),
		    *m_allocator);
		SetBufferData(stagingBuffer, data.pixels);

		// Copy staging buffer to image
		TransitionImageLayout(
		    cmdBuffer, image.get(), imageInfo.format, data.mipLevelCount, imageInfo.initialLayout,
		    vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
		    vk::PipelineStageFlagBits::eTransfer);
		initialLayout = vk::ImageLayout::eTransferDstOptimal;
		initialPipelineStage = vk::PipelineStageFlagBits::eTransfer;
		CopyBufferToImage(cmdBuffer, stagingBuffer.buffer.get(), image.get(), imageInfo.format, imageExtent);

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
	    .lastPipelineStageUsage = initialPipelineStage,
	};

	SetDebugName(ret.image, name);
	SetDebugName(ret.imageView, "{}:View", name);
	SetDebugName(ret.sampler, "{}:Sampler", name);

	return ret;
}

std::vector<vk::UniqueDescriptorSet> GraphicsDevice::CreateDescriptorSets(
    vk::DescriptorSetLayout layout, size_t numSets, const DynamicBuffer& uniformBuffer,
    std::span<const Texture* const> textures, const char* name)
{
	// TODO support multiple textures in descriptor sets
	assert(textures.size() <= 1 && "Multiple textures not yet supported!");

	const auto layouts = std::vector<vk::DescriptorSetLayout>(numSets, layout);
	const auto allocInfo = vk::DescriptorSetAllocateInfo{
	    .descriptorPool = m_descriptorPool.get(),
	    .descriptorSetCount = size32(layouts),
	    .pSetLayouts = data(layouts),
	};

	auto descriptorSets = m_device->allocateDescriptorSetsUnique(allocInfo);

	for (size_t i = 0; i < layouts.size(); i++)
	{
		SetDebugName(descriptorSets[i], name);

		const auto numUniformBuffers = uniformBuffer.size > 0 ? 1 : 0;

		std::vector<vk::WriteDescriptorSet> descriptorWrites;
		descriptorWrites.reserve(numUniformBuffers + textures.size());

		const auto bufferInfo = vk::DescriptorBufferInfo{
		    .buffer = uniformBuffer.buffers[i].buffer.get(),
		    .offset = 0,
		    .range = uniformBuffer.size,
		};

		if (uniformBuffer.size > 0)
		{
			descriptorWrites.push_back({
			    .dstSet = descriptorSets[i].get(),
			    .dstBinding = 0,
			    .dstArrayElement = 0,
			    .descriptorCount = 1,
			    .descriptorType = vk::DescriptorType::eUniformBuffer,
			    .pBufferInfo = &bufferInfo,
			});
		}

		std::vector<vk::DescriptorImageInfo> imageInfos;
		imageInfos.reserve(textures.size());
		for (const auto& texture : textures)
		{
			if (!texture)
				continue;

			imageInfos.push_back({
			    .sampler = texture->sampler.get(),
			    .imageView = texture->imageView.get(),
			    .imageLayout = HasDepthOrStencilComponent(texture->format) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
			                                                               : vk::ImageLayout::eShaderReadOnlyOptimal,
			});

			descriptorWrites.push_back({
			    .dstSet = descriptorSets[i].get(),
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

ParameterBlockPtr GraphicsDevice::CreateParameterBlock(const ParameterBlockData& data, const char* name)
{
	// TODO support creating parameter blocks with static uniform buffers
	return CreateDynamicParameterBlock(data, name);
}

DynamicParameterBlockPtr GraphicsDevice::CreateDynamicParameterBlock(const ParameterBlockData& data, const char* name)
{
	const auto descriptorSetName = DebugFormat("{}DescriptorSet", name);

	ParameterBlock ret;
	if (data.uniformBufferSize == 0)
	{
		ret.descriptorSet = CreateDescriptorSets(data.layout, 1, DynamicBuffer{}, data.textures, descriptorSetName.c_str());
	}
	else
	{
		const auto uniformBufferName = DebugFormat("{}UniformBuffer", name);

		ret.uniformBuffer = CreateDynamicBuffer(
		    data.uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, uniformBufferName.c_str());
		ret.descriptorSet = CreateDescriptorSets(
		    data.layout, ret.uniformBuffer->buffers.size(), *ret.uniformBuffer, data.textures, descriptorSetName.c_str());
	}
	return std::make_unique<ParameterBlock>(std::move(ret));
}
