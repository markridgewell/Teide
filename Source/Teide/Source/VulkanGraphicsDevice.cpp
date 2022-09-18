
#include "VulkanGraphicsDevice.h"

#include "CommandBuffer.h"
#include "Types/ShaderData.h"
#include "Types/TextureData.h"
#include "VulkanBuffer.h"
#include "VulkanParameterBlock.h"
#include "VulkanPipeline.h"
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
    MemoryAllocator& allocator, CommandBuffer& cmdBuffer)
{
	if ((memoryFlags & vk::MemoryPropertyFlagBits::eHostCoherent) == vk::MemoryPropertyFlags{})
	{
		// Create staging buffer
		auto stagingBuffer = std::make_shared<VulkanBuffer>(CreateBufferUninitialized(
		    data.size(), vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, device, allocator));
		cmdBuffer.AddBuffer(stagingBuffer);
		SetBufferData(*stagingBuffer, data);

		// Create device-local buffer
		VulkanBuffer ret = CreateBufferUninitialized(
		    data.size(), usage | vk::BufferUsageFlagBits::eTransferDst, memoryFlags, device, allocator);
		CopyBuffer(cmdBuffer, stagingBuffer->buffer.get(), ret.buffer.get(), data.size());

		// Add pipeline barrier to make the buffer usable in shader
		cmdBuffer->pipelineBarrier(
		    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexShader, {}, {},
		    vk::BufferMemoryBarrier{
		        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
		        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		        .buffer = ret.buffer.get(),
		        .offset = 0,
		        .size = VK_WHOLE_SIZE,
		    },
		    {});
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
    CommandBuffer& cmdBuffer, const char* debugName)
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
	    .samples = data.sampleCount,
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
	    .sampleCount = data.sampleCount,
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
	    shader.sceneDescriptorSet.layout.get(),
	    shader.viewDescriptorSet.layout.get(),
	    shader.materialDescriptorSet.layout.get(),
	    shader.objectDescriptorSet.layout.get(),
	};

	const auto createInfo = vk::PipelineLayoutCreateInfo{
	    .setLayoutCount = size32(setLayouts),
	    .pSetLayouts = data(setLayouts),
	    .pushConstantRangeCount = size32(pushConstantRanges),
	    .pPushConstantRanges = data(pushConstantRanges),
	};

	return device.createPipelineLayoutUnique(createInfo, s_allocator);
}

vk::UniquePipeline
CreateGraphicsPipeline(const VulkanShader& shader, const PipelineData& piplineData, vk::RenderPass renderPass, vk::Device device)
{
	const auto vertexShader = shader.vertexShader.get();
	const auto pixelShader = shader.pixelShader.get();

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back({.stage = vk::ShaderStageFlagBits::eVertex, .module = vertexShader, .pName = "main"});
	if (piplineData.framebufferLayout.colorFormat != vk::Format::eUndefined)
	{
		shaderStages.push_back({.stage = vk::ShaderStageFlagBits::eFragment, .module = pixelShader, .pName = "main"});
	}

	const auto vertexInput = vk::PipelineVertexInputStateCreateInfo{
	    .vertexBindingDescriptionCount = size32(piplineData.vertexLayout.vertexInputBindings),
	    .pVertexBindingDescriptions = data(piplineData.vertexLayout.vertexInputBindings),
	    .vertexAttributeDescriptionCount = size32(piplineData.vertexLayout.vertexInputAttributes),
	    .pVertexAttributeDescriptions = data(piplineData.vertexLayout.vertexInputAttributes),
	};

	const auto viewportState = vk::PipelineViewportStateCreateInfo{
	    .viewportCount = 1,
	    .pViewports = &piplineData.renderStates.viewport,
	    .scissorCount = 1,
	    .pScissors = &piplineData.renderStates.scissor,
	};

	const auto multisampleState = vk::PipelineMultisampleStateCreateInfo{
	    .rasterizationSamples = piplineData.framebufferLayout.sampleCount,
	    .sampleShadingEnable = false,
	    .minSampleShading = 1.0f,
	    .pSampleMask = nullptr,
	    .alphaToCoverageEnable = false,
	    .alphaToOneEnable = false,
	};

	const auto colorBlendState = vk::PipelineColorBlendStateCreateInfo{
	    .logicOpEnable = false,
	    .attachmentCount = 1,
	    .pAttachments = &piplineData.renderStates.colorBlendAttachment,
	};

	const auto dynamicState = vk::PipelineDynamicStateCreateInfo{
	    .dynamicStateCount = size32(piplineData.renderStates.dynamicStates),
	    .pDynamicStates = data(piplineData.renderStates.dynamicStates),
	};
	const auto createInfo = vk::GraphicsPipelineCreateInfo{
	    .stageCount = size32(shaderStages),
	    .pStages = data(shaderStages),
	    .pVertexInputState = &vertexInput,
	    .pInputAssemblyState = &piplineData.vertexLayout.inputAssembly,
	    .pViewportState = &viewportState,
	    .pRasterizationState = &piplineData.renderStates.rasterizationState,
	    .pMultisampleState = &multisampleState,
	    .pDepthStencilState = &piplineData.renderStates.depthStencilState,
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

vk::ShaderStageFlags GetShaderStageFlags(ShaderStageFlags flags)
{
	vk::ShaderStageFlags ret{};
	if ((flags & ShaderStageFlags::Vertex) != ShaderStageFlags{})
	{
		ret |= vk::ShaderStageFlagBits::eVertex;
	}
	if ((flags & ShaderStageFlags::Pixel) != ShaderStageFlags{})
	{
		ret |= vk::ShaderStageFlagBits::eFragment;
	}
	return ret;
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------

GraphicsDevicePtr CreateGraphicsDevice(SDL_Window* window)
{
	return std::make_unique<VulkanGraphicsDevice>(window);
}

VulkanGraphicsDevice::VulkanGraphicsDevice(SDL_Window* window, std::uint32_t numThreads) :
    m_workerDescriptorPools(numThreads)
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

	m_surfaceCommandPool = CreateCommandPool(m_graphicsQueueFamily, m_device.get());
	SetDebugName(m_surfaceCommandPool, "SurfaceCommandPool");

	m_pendingWindowSurfaces[window] = std::move(surface);

	m_scheduler.emplace(numThreads, m_device.get(), m_graphicsQueue, m_graphicsQueueFamily);

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

	m_mainDescriptorPool = m_device->createDescriptorPoolUnique(poolCreateInfo, s_allocator);
	SetDebugName(m_mainDescriptorPool, "DescriptorPool");

	int i = 0;
	for (auto& pool : m_workerDescriptorPools)
	{
		pool = m_device->createDescriptorPoolUnique(poolCreateInfo, s_allocator);
		SetDebugName(pool, "WorkerDescriptorPool{}", i);
		i++;
	}
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
	    std::vector<uint32_t>{m_graphicsQueueFamily, *m_presentQueueFamily}, m_surfaceCommandPool.get(),
	    m_graphicsQueue, multisampled);
}

RendererPtr VulkanGraphicsDevice::CreateRenderer(ShaderPtr shaderEnvironment)
{
	return std::make_unique<VulkanRenderer>(*this, m_graphicsQueueFamily, m_presentQueueFamily, std::move(shaderEnvironment));
}

BufferPtr VulkanGraphicsDevice::CreateBuffer(const BufferData& data, const char* name)
{
	auto task = m_scheduler->ScheduleGpu([=, this](CommandBuffer& cmdBuffer) { //
		return CreateBuffer(data, name, cmdBuffer);
	});
	return task.get().value();
}

BufferPtr VulkanGraphicsDevice::CreateBuffer(const BufferData& data, const char* name, CommandBuffer& cmdBuffer)
{
	auto ret = CreateBufferWithData(data.data, data.usage, data.memoryFlags, m_device.get(), *m_allocator, cmdBuffer);
	SetDebugName(ret.buffer, name);
	return std::make_shared<const VulkanBuffer>(std::move(ret));
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

	std::vector<vk::PushConstantRange> pushConstantRanges;

	const auto createSetLayout = [this, &pushConstantRanges](const ParameterBlockDesc& desc, int set) {
		DescriptorSetInfo ret;
		std::vector<vk::DescriptorSetLayoutBinding> bindings;

		const ParameterBlockLayout layout = BuildParameterBlockLayout(desc, set);

		if (layout.uniformsSize > 0u)
		{
			if (layout.isPushConstant)
			{
				pushConstantRanges.push_back(vk::PushConstantRange{
				    .stageFlags = GetShaderStageFlags(layout.uniformsStages),
				    .offset = 0,
				    .size = static_cast<uint32_t>(layout.uniformsSize),
				});
			}
			else
			{
				bindings.push_back({
				    .binding = 0,
				    .descriptorType = vk::DescriptorType::eUniformBuffer,
				    .descriptorCount = 1,
				    .stageFlags = GetShaderStageFlags(layout.uniformsStages),
				});
			}
		}

		for (std::uint32_t i = 0; i < layout.textureCount; i++)
		{
			bindings.push_back({
			    .binding = i + 1,
			    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
			    .descriptorCount = 1,
			    .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
			});
		}

		ret.layout = CreateDescriptorSetLayout(m_device.get(), bindings);
		ret.uniformsStages = GetShaderStageFlags(layout.uniformsStages);
		return ret;
	};

	auto shader = VulkanShaderBase{
	    .vertexShader = m_device->createShaderModuleUnique(vertexCreateInfo, s_allocator),
	    .pixelShader = m_device->createShaderModuleUnique(pixelCreateInfo, s_allocator),
	    .sceneDescriptorSet = createSetLayout(data.scenePblock, 0),
	    .viewDescriptorSet = createSetLayout(data.viewPblock, 1),
	    .materialDescriptorSet = createSetLayout(data.materialPblock, 2),
	    .objectDescriptorSet = createSetLayout(data.objectPblock, 3),
	};

	shader.pipelineLayout = CreateGraphicsPipelineLayout(m_device.get(), shader, pushConstantRanges);

	SetDebugName(shader.vertexShader, "{}:Vertex", name);
	SetDebugName(shader.pixelShader, "{}:Pixel", name);
	SetDebugName(shader.pipelineLayout, "{}:PipelineLayout", name);

	return std::make_shared<const VulkanShader>(std::move(shader));
}

TexturePtr VulkanGraphicsDevice::CreateTexture(const TextureData& data, const char* name)
{
	auto task = m_scheduler->ScheduleGpu([=, this](CommandBuffer& cmdBuffer) { //
		return CreateTexture(data, name, cmdBuffer);
	});
	return task.get().value();
}

TexturePtr VulkanGraphicsDevice::CreateTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer)
{
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
	auto task = m_scheduler->ScheduleGpu([=, this](CommandBuffer& cmdBuffer) { //
		return CreateRenderableTexture(data, name, cmdBuffer);
	});
	return task.get().value();
}

DynamicTexturePtr VulkanGraphicsDevice::CreateRenderableTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer)
{
	const bool isColorTarget = !HasDepthOrStencilComponent(data.format);
	const auto renderUsage
	    = isColorTarget ? vk::ImageUsageFlagBits::eColorAttachment : vk::ImageUsageFlagBits::eDepthStencilAttachment;

	const auto usage = renderUsage | vk::ImageUsageFlagBits::eSampled;

	auto [texture, state] = CreateTextureImpl(data, m_device.get(), m_allocator.value(), usage, cmdBuffer, name);

	if (isColorTarget)
	{
		texture.TransitionToColorTarget(state, cmdBuffer);
	}
	else
	{
		texture.TransitionToDepthStencilTarget(state, cmdBuffer);
	}

	return std::make_shared<VulkanTexture>(std::move(texture));
}

PipelinePtr VulkanGraphicsDevice::CreatePipeline(const PipelineData& data)
{
	const auto& shaderImpl = GetImpl(*data.shader);

	const auto pushConstantsShaderStages = shaderImpl.objectDescriptorSet.uniformsStages;

	return std::make_shared<const VulkanPipeline>(
	    CreateGraphicsPipeline(shaderImpl, data, CreateRenderPass(data.framebufferLayout), m_device.get()),
	    shaderImpl.pipelineLayout.get(), pushConstantsShaderStages);
}

vk::UniqueDescriptorSet VulkanGraphicsDevice::CreateDescriptorSet(
    vk::DescriptorPool pool, vk::DescriptorSetLayout layout, const Buffer* uniformBuffer,
    std::span<const Texture* const> textures, const char* name)
{
	return std::move(CreateDescriptorSets(pool, layout, 1, uniformBuffer, textures, name).front());
}

std::vector<vk::UniqueDescriptorSet> VulkanGraphicsDevice::CreateDescriptorSets(
    vk::DescriptorPool pool, vk::DescriptorSetLayout layout, size_t numSets, const Buffer* uniformBuffer,
    std::span<const Texture* const> textures, const char* name)
{
	// TODO support multiple textures in descriptor sets
	assert(textures.size() <= 1 && "Multiple textures not yet supported!");

	const auto layouts = std::vector<vk::DescriptorSetLayout>(numSets, layout);
	const auto allocInfo = vk::DescriptorSetAllocateInfo{
	    .descriptorPool = pool,
	    .descriptorSetCount = size32(layouts),
	    .pSetLayouts = data(layouts),
	};

	auto descriptorSets = m_device->allocateDescriptorSetsUnique(allocInfo);

	for (size_t i = 0; i < layouts.size(); i++)
	{
		SetDebugName(descriptorSets[i], name);

		const auto numUniformBuffers = uniformBuffer ? 1 : 0;

		std::vector<vk::WriteDescriptorSet> descriptorWrites;
		descriptorWrites.reserve(numUniformBuffers + textures.size());

		if (uniformBuffer)
		{
			const auto& uniformBufferImpl = GetImpl(*uniformBuffer);
			const auto bufferInfo = vk::DescriptorBufferInfo{
			    .buffer = uniformBufferImpl.buffer.get(),
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

vk::RenderPass VulkanGraphicsDevice::CreateRenderPass(const FramebufferLayout& framebufferLayout, const RenderPassInfo& renderPassInfo)
{
	const auto lock = std::scoped_lock(m_renderPassCacheMutex);

	const auto desc = RenderPassDesc{framebufferLayout, renderPassInfo};
	const auto [it, inserted] = m_renderPassCache.emplace(desc, nullptr);
	if (inserted)
	{
		it->second = ::CreateRenderPass(m_device.get(), framebufferLayout, renderPassInfo);
	}
	return it->second.get();
}

vk::Framebuffer
VulkanGraphicsDevice::CreateFramebuffer(vk::RenderPass renderPass, vk::Extent2D size, std::vector<vk::ImageView> attachments)
{
	const auto lock = std::scoped_lock(m_framebufferCacheMutex);

	const auto desc = FramebufferDesc{renderPass, size, std::move(attachments)};
	const auto [it, inserted] = m_framebufferCache.emplace(desc, nullptr);
	if (inserted)
	{
		it->second = ::CreateFramebuffer(m_device.get(), desc.renderPass, desc.size, desc.attachments);
	}
	return it->second.get();
}

ParameterBlockPtr VulkanGraphicsDevice::CreateParameterBlock(const ParameterBlockData& data, const char* name)
{
	auto task = m_scheduler->ScheduleGpu([=, this](CommandBuffer& cmdBuffer) {
		return CreateParameterBlock(data, name, cmdBuffer, m_mainDescriptorPool.get());
	});
	return task.get().value();
}

ParameterBlockPtr VulkanGraphicsDevice::CreateParameterBlock(
    const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, std::uint32_t threadIndex)
{
	return CreateParameterBlock(data, name, cmdBuffer, m_workerDescriptorPools[threadIndex].get());
}

ParameterBlockPtr VulkanGraphicsDevice::CreateParameterBlock(
    const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, vk::DescriptorPool descriptorPool)
{
	if (!data.shader)
	{
		return nullptr;
	}

	const auto layout = GetImpl(*data.shader).GetDescriptorSetLayout(data.blockType);
	if (!layout)
	{
		return nullptr;
	}

	const auto descriptorSetName = DebugFormat("{}DescriptorSet", name);

	VulkanParameterBlock ret;
	if (!data.parameters.uniformBufferData.empty())
	{
		const auto uniformBufferName = DebugFormat("{}UniformBuffer", name);
		ret.uniformBuffer = CreateBuffer(
		    BufferData{
		        .usage = vk::BufferUsageFlagBits::eUniformBuffer,
		        .memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
		        .data = data.parameters.uniformBufferData,
		    },
		    uniformBufferName.c_str(), cmdBuffer);
	}
	ret.descriptorSet = CreateDescriptorSet(
	    descriptorPool, layout, ret.uniformBuffer.get(), data.parameters.textures, descriptorSetName.c_str());

	return std::make_unique<VulkanParameterBlock>(std::move(ret));
}
