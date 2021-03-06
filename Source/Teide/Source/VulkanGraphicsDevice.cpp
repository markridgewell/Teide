
#include "VulkanGraphicsDevice.h"

#include "CommandBuffer.h"
#include "Types/ShaderData.h"
#include "Types/TextureData.h"
#include "VulkanBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanShader.h"
#include "VulkanSurface.h"
#include "VulkanTexture.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include <cassert>

namespace
{
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

void SetBufferData(VulkanBuffer& buffer, BytesView data)
{
	assert(buffer.mappedData.size() >= data.size());
	std::memcpy(buffer.mappedData.data(), data.data(), data.size());
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

VulkanBuffer CreateBufferWithData(
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
		VulkanBuffer ret = CreateBufferUninitialized(
		    data.size(), usage | vk::BufferUsageFlagBits::eTransferDst, memoryFlags, device, allocator);
		auto cmdBuffer = OneShotCommandBuffer(device, commandPool, queue);
		CopyBuffer(cmdBuffer, stagingBuffer.buffer.get(), ret.buffer.get(), data.size());
		return ret;
	}
	else
	{
		VulkanBuffer ret
		    = CreateBufferUninitialized(data.size(), vk::BufferUsageFlagBits::eTransferSrc, memoryFlags, device, allocator);
		SetBufferData(ret, data);
		return ret;
	}
}

struct TextureAndState
{
	VulkanTexture texture;
	TextureState state;
};

TextureAndState CreateTextureImpl(
    const TextureData& data, vk::Device device, MemoryAllocator& allocator, vk::ImageUsageFlags usage,
    OneShotCommandBuffer& cmdBuffer, const char* debugName)
{
	// For now, all textures will be created with TransferSrc so they can be copied from
	usage |= vk::ImageUsageFlagBits::eTransferSrc;

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

	auto initialState = TextureState{
	    .layout = vk::ImageLayout::eUndefined,
	    .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTopOfPipe,
	};

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
	    .initialLayout = initialState.layout,
	};

	auto image = device.createImageUnique(imageInfo, s_allocator);
	const auto memory
	    = allocator.Allocate(device.getImageMemoryRequirements(image.get()), vk::MemoryPropertyFlagBits::eDeviceLocal);
	device.bindImageMemory(image.get(), memory.memory, memory.offset);

	if (!data.pixels.empty())
	{
		// Create staging buffer
		auto stagingBuffer = CreateBufferUninitialized(
		    data.pixels.size(), vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, device, allocator);
		SetBufferData(stagingBuffer, data.pixels);

		// Copy staging buffer to image
		TransitionImageLayout(
		    cmdBuffer, image.get(), imageInfo.format, data.mipLevelCount, imageInfo.initialLayout,
		    vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
		    vk::PipelineStageFlagBits::eTransfer);
		initialState = {
		    .layout = vk::ImageLayout::eTransferDstOptimal,
		    .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTransfer,
		};
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
	auto imageView = device.createImageViewUnique(viewInfo, s_allocator);
	auto sampler = device.createSamplerUnique(data.samplerInfo, s_allocator);

	auto ret = VulkanTextureData{
	    .image = std::move(image),
	    .memory = memory,
	    .imageView = std::move(imageView),
	    .sampler = std::move(sampler),
	    .size = {imageExtent.width, imageExtent.height},
	    .format = data.format,
	    .mipLevelCount = data.mipLevelCount,
	    .samples = data.samples,
	};

	SetDebugName(ret.image, debugName);
	SetDebugName(ret.imageView, "{}:View", debugName);
	SetDebugName(ret.sampler, "{}:Sampler", debugName);

	return {std::move(ret), initialState};
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

vk::UniquePipelineLayout CreateGraphicsPipelineLayout(
    vk::Device device, const VulkanShaderBase& shader, std::span<const vk::PushConstantRange> pushConstantRanges)
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
    const VulkanShader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates,
    vk::RenderPass renderPass, vk::Format format, vk::SampleCountFlagBits sampleCount, vk::Device device)
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

GraphicsDevicePtr CreateGraphicsDevice(SDL_Window* window)
{
	return std::make_unique<VulkanGraphicsDevice>(window);
}

VulkanGraphicsDevice::VulkanGraphicsDevice(SDL_Window* window)
{
	m_instance = CreateInstance(m_loader, window);

	if constexpr (IsDebugBuild)
	{
		m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(GetDebugCreateInfo(), s_allocator);
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

VulkanGraphicsDevice::~VulkanGraphicsDevice()
{
	m_device->waitIdle();
}

SurfacePtr VulkanGraphicsDevice::CreateSurface(SDL_Window* window, bool multisampled)
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

	return std::make_unique<VulkanSurface>(
	    window, std::move(surface), m_device.get(), m_physicalDevice,
	    std::vector<uint32_t>{m_graphicsQueueFamily, *m_presentQueueFamily}, m_setupCommandPool.get(), m_graphicsQueue,
	    multisampled);
}

RendererPtr VulkanGraphicsDevice::CreateRenderer()
{
	return std::make_unique<VulkanRenderer>(*this, m_graphicsQueueFamily, m_presentQueueFamily);
}

BufferPtr VulkanGraphicsDevice::CreateBuffer(const BufferData& data, const char* name)
{
	auto ret = CreateBufferWithData(
	    data.data, data.usage, data.memoryFlags, m_device.get(), *m_allocator, m_setupCommandPool.get(), m_graphicsQueue);
	SetDebugName(ret.buffer, name);
	return std::make_shared<const VulkanBuffer>(std::move(ret));
}

DynamicBufferPtr VulkanGraphicsDevice::CreateDynamicBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags usage, const char* name)
{
	VulkanDynamicBuffer ret;
	ret.size = bufferSize;
	for (auto& buffer : ret.buffers)
	{
		buffer = CreateBufferUninitialized(
		    bufferSize, usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		    m_device.get(), *m_allocator);
		SetDebugName(buffer.buffer, name);
	}
	return std::make_shared<VulkanDynamicBuffer>(std::move(ret));
}

ShaderPtr VulkanGraphicsDevice::CreateShader(const ShaderData& data, const char* name)
{
	const auto vertexCreateInfo = vk::ShaderModuleCreateInfo{
	    .codeSize = data.vertexShaderSpirv.size() * sizeof(uint32_t),
	    .pCode = data.vertexShaderSpirv.data(),
	};
	const auto pixelCreateInfo = vk::ShaderModuleCreateInfo{
	    .codeSize = data.pixelShaderSpirv.size() * sizeof(uint32_t),
	    .pCode = data.pixelShaderSpirv.data(),
	};

	auto shader = VulkanShaderBase{
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

	return std::make_shared<const VulkanShader>(std::move(shader));
}

TexturePtr VulkanGraphicsDevice::CreateTexture(const TextureData& data, const char* name)
{
	auto cmdBuffer = OneShotCommands();

	const auto usage = vk::ImageUsageFlagBits::eSampled;

	auto [texture, state] = CreateTextureImpl(data, m_device.get(), m_allocator.value(), usage, cmdBuffer, name);

	if (data.mipLevelCount > 1)
	{
		texture.GenerateMipmaps(state, cmdBuffer);
	}
	else
	{
		// Transition into samplable image, but only if mipmaps were not generated
		// (if so, the mipmap generation already transitioned to image to be samplable)
		texture.TransitionToShaderInput(state, cmdBuffer);
	}

	return std::make_shared<const VulkanTexture>(std::move(texture));
}

DynamicTexturePtr VulkanGraphicsDevice::CreateRenderableTexture(const TextureData& data, const char* name)
{
	const bool isColorTarget = !HasDepthOrStencilComponent(data.format);
	const auto renderUsage
	    = isColorTarget ? vk::ImageUsageFlagBits::eColorAttachment : vk::ImageUsageFlagBits::eDepthStencilAttachment;

	const auto usage = renderUsage | vk::ImageUsageFlagBits::eSampled;

	auto cmdBuffer = OneShotCommands();

	auto [texture, state] = CreateTextureImpl(data, m_device.get(), m_allocator.value(), usage, cmdBuffer, name);

	if (isColorTarget)
	{
		texture.TransitionToColorTarget(state, cmdBuffer);
	}
	else
	{
		texture.TransitionToDepthStencilTarget(state, cmdBuffer);
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
	    .finalLayout = state.layout,
	};

	const auto attachmentRef = vk::AttachmentReference{
	    .attachment = 0,
	    .layout = state.layout,
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

	return std::make_shared<VulkanTexture>(std::move(texture));
}

PipelinePtr VulkanGraphicsDevice::CreatePipeline(
    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates, const Surface& surface)
{
	const auto& shaderImpl = GetImpl(shader);
	const auto& surfaceImpl = GetImpl(surface);
	return std::make_shared<const Pipeline>(
	    CreateGraphicsPipeline(
	        shaderImpl, vertexLayout, renderStates, surfaceImpl.GetVulkanRenderPass(), surface.GetColorFormat(),
	        surface.GetSampleCount(), m_device.get()),
	    shaderImpl.pipelineLayout.get());
}

PipelinePtr VulkanGraphicsDevice::CreatePipeline(
    const Shader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates, const Texture& texture)
{
	const auto& shaderImpl = GetImpl(shader);
	const auto& textureImpl = GetImpl(texture);

	return std::make_shared<const Pipeline>(
	    CreateGraphicsPipeline(
	        shaderImpl, vertexLayout, renderStates, textureImpl.renderPass.get(), textureImpl.format,
	        textureImpl.samples, m_device.get()),
	    shaderImpl.pipelineLayout.get());
}

std::vector<vk::UniqueDescriptorSet> VulkanGraphicsDevice::CreateDescriptorSets(
    vk::DescriptorSetLayout layout, size_t numSets, const DynamicBuffer& uniformBuffer,
    std::span<const Texture* const> textures, const char* name)
{
	const auto& uniformBufferImpl = GetImpl(uniformBuffer);

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

		const auto numUniformBuffers = uniformBufferImpl.size > 0 ? 1 : 0;

		std::vector<vk::WriteDescriptorSet> descriptorWrites;
		descriptorWrites.reserve(numUniformBuffers + textures.size());

		const auto bufferInfo = vk::DescriptorBufferInfo{
		    .buffer = uniformBufferImpl.buffers[i].buffer.get(),
		    .offset = 0,
		    .range = uniformBufferImpl.size,
		};

		if (uniformBufferImpl.size > 0)
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

			const auto& textureImpl = GetImpl(*texture);

			imageInfos.push_back({
			    .sampler = textureImpl.sampler.get(),
			    .imageView = textureImpl.imageView.get(),
			    .imageLayout = HasDepthOrStencilComponent(textureImpl.format) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
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

ParameterBlockPtr VulkanGraphicsDevice::CreateParameterBlock(const ParameterBlockData& data, const char* name)
{
	// TODO support creating parameter blocks with static uniform buffers
	return CreateDynamicParameterBlock(data, name);
}

DynamicParameterBlockPtr VulkanGraphicsDevice::CreateDynamicParameterBlock(const ParameterBlockData& data, const char* name)
{
	const auto descriptorSetName = DebugFormat("{}DescriptorSet", name);

	ParameterBlock ret;
	if (data.uniformBufferSize == 0)
	{
		ret.descriptorSet
		    = CreateDescriptorSets(data.layout, 1, VulkanDynamicBuffer{}, data.textures, descriptorSetName.c_str());
	}
	else
	{
		const auto uniformBufferName = DebugFormat("{}UniformBuffer", name);

		ret.uniformBuffer = CreateDynamicBuffer(
		    data.uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, uniformBufferName.c_str());
		ret.descriptorSet = CreateDescriptorSets(
		    data.layout, GetImpl(*ret.uniformBuffer).buffers.size(), *ret.uniformBuffer, data.textures,
		    descriptorSetName.c_str());
	}
	return std::make_unique<ParameterBlock>(std::move(ret));
}
