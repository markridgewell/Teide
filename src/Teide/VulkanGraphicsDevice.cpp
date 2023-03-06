
#include "VulkanGraphicsDevice.h"

#include "CommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanMesh.h"
#include "VulkanParameterBlock.h"
#include "VulkanPipeline.h"
#include "VulkanRenderer.h"
#include "VulkanShader.h"
#include "VulkanShaderEnvironment.h"
#include "VulkanSurface.h"
#include "VulkanTexture.h"

#include "Teide/ShaderData.h"
#include "Teide/TextureData.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include <cassert>

namespace Teide
{

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
        const vk::BufferCopy copyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size,
        };
        cmdBuffer.copyBuffer(source, destination, copyRegion);
    }

    VulkanBuffer CreateBufferWithData(
        BytesView data, BufferUsage usage, ResourceLifetime lifetime, vk::Device device, MemoryAllocator& allocator,
        CommandBuffer& cmdBuffer)
    {
        const vk::BufferUsageFlags usageFlags = GetBufferUsageFlags(usage);

        if (lifetime == ResourceLifetime::Permanent)
        {
            // Create staging buffer
            auto stagingBuffer = std::make_shared<VulkanBuffer>(CreateBufferUninitialized(
                data.size(), vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, device, allocator));
            cmdBuffer.AddBuffer(stagingBuffer);
            SetBufferData(*stagingBuffer, data);

            // Create device-local buffer
            VulkanBuffer ret = CreateBufferUninitialized(
                data.size(), usageFlags | vk::BufferUsageFlagBits::eTransferDst,
                vk::MemoryPropertyFlagBits::eDeviceLocal, device, allocator);
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
            VulkanBuffer ret = CreateBufferUninitialized(
                data.size(), usageFlags | vk::BufferUsageFlagBits::eTransferDst,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, device, allocator);
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
        const auto imageExtent = vk::Extent3D{data.size.x, data.size.y, 1};
        const vk::ImageCreateInfo imageInfo = {
            .imageType = vk::ImageType::e2D,
            .format = ToVulkan(data.format),
            .extent = imageExtent,
            .mipLevels = data.mipLevelCount,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits{data.sampleCount},
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
                cmdBuffer, image.get(), data.format, data.mipLevelCount, imageInfo.initialLayout,
                vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eTransfer);
            initialState = {
                .layout = vk::ImageLayout::eTransferDstOptimal,
                .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTransfer,
            };
            CopyBufferToImage(cmdBuffer, stagingBuffer.buffer.get(), image.get(), data.format, imageExtent);

            cmdBuffer.TakeOwnership(std::move(stagingBuffer.buffer));
        }

        // Create image view
        const vk::ImageViewCreateInfo viewInfo = {
        .image = image.get(),
        .viewType = vk::ImageViewType::e2D,
        .format = imageInfo.format,
        .subresourceRange = {
            .aspectMask =  GetImageAspect(data.format),
            .baseMipLevel = 0,
            .levelCount = data.mipLevelCount,
            .baseArrayLayer = 0,
            .layerCount = 1,
            },
        };
        auto imageView = device.createImageViewUnique(viewInfo, s_allocator);

        const auto& ss = data.samplerState;
        const vk::SamplerCreateInfo samplerInfo = {
            .magFilter = ToVulkan(ss.magFilter),
            .minFilter = ToVulkan(ss.minFilter),
            .mipmapMode = ToVulkan(ss.mipmapMode),
            .addressModeU = ToVulkan(ss.addressModeU),
            .addressModeV = ToVulkan(ss.addressModeV),
            .addressModeW = ToVulkan(ss.addressModeW),
            .anisotropyEnable = ss.maxAnisotropy.has_value(),
            .maxAnisotropy = ss.maxAnisotropy.value_or(0.0f),
            .compareEnable = ss.compareOp.has_value(),
            .compareOp = ToVulkan(ss.compareOp.value_or(CompareOp::Never)),
        };
        auto sampler = device.createSamplerUnique(samplerInfo, s_allocator);

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
        if (layoutBindings.empty())
        {
            return {};
        }

        const vk::DescriptorSetLayoutCreateInfo createInfo = {
            .bindingCount = size32(layoutBindings),
            .pBindings = data(layoutBindings),
        };

        return device.createDescriptorSetLayoutUnique(createInfo, s_allocator);
    }

    vk::UniquePipelineLayout CreateGraphicsPipelineLayout(vk::Device device, const VulkanShaderBase& shader)
    {
        std::vector<vk::DescriptorSetLayout> setLayouts = {
            shader.scenePblockLayout->setLayout.get(),
            shader.viewPblockLayout->setLayout.get(),
            shader.materialPblockLayout->setLayout.get(),
            shader.objectPblockLayout->setLayout.get(),
        };
        std::erase(setLayouts, vk::DescriptorSetLayout{});

        const bool hasPushConstants = shader.objectPblockLayout->pushConstantRange.has_value();

        const vk::PipelineLayoutCreateInfo createInfo = {
            .setLayoutCount = size32(setLayouts),
            .pSetLayouts = data(setLayouts),
            .pushConstantRangeCount = hasPushConstants ? 1u : 0u,
            .pPushConstantRanges = hasPushConstants ? &shader.objectPblockLayout->pushConstantRange.value() : nullptr,
        };

        return device.createPipelineLayoutUnique(createInfo, s_allocator);
    }

    vk::PipelineColorBlendAttachmentState MakeBlendState(const std::optional<BlendState>& state, ColorMask colorMask)
    {
        vk::PipelineColorBlendAttachmentState ret;
        if (state)
        {
            const auto colorBlendFunc = state->blendFunc;
            const auto alphaBlendFunc = state->alphaBlendFunc.value_or(state->blendFunc);

            ret.blendEnable = true;
            ret.srcColorBlendFactor = ToVulkan(colorBlendFunc.source);
            ret.dstColorBlendFactor = ToVulkan(colorBlendFunc.dest);
            ret.colorBlendOp = ToVulkan(colorBlendFunc.op);
            ret.srcAlphaBlendFactor = ToVulkan(alphaBlendFunc.source);
            ret.dstAlphaBlendFactor = ToVulkan(alphaBlendFunc.dest);
            ret.alphaBlendOp = ToVulkan(alphaBlendFunc.op);
        }
        else
        {
            ret.blendEnable = false;
        }
        ret.colorWriteMask = ToVulkan(colorMask);
        return ret;
    }

    vk::UniquePipeline CreateGraphicsPipeline(
        const VulkanShader& shader, const VertexLayout& vertexLayout, const RenderStates& renderStates,
        const RenderPassDesc& renderPass, VulkanGraphicsDevice& device)
    {
        const auto vertexShader = shader.vertexShader.get();
        const auto pixelShader = shader.pixelShader.get();

        const auto renderPassLayout = device.CreateRenderPassLayout(renderPass.framebufferLayout);

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        shaderStages.push_back({.stage = vk::ShaderStageFlagBits::eVertex, .module = vertexShader, .pName = "main"});
        if (renderPass.framebufferLayout.colorFormat.has_value())
        {
            shaderStages.push_back({.stage = vk::ShaderStageFlagBits::eFragment, .module = pixelShader, .pName = "main"});
        }

        std::vector<vk::VertexInputBindingDescription> vertexInputBindings(vertexLayout.bufferBindings.size());
        for (uint32 i = 0; i < vertexInputBindings.size(); i++)
        {
            vertexInputBindings[i] = {
                .binding = i,
                .stride = vertexLayout.bufferBindings[i].stride,
                .inputRate = ToVulkan(vertexLayout.bufferBindings[i].vertexClass),
            };
        }

        std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes(vertexLayout.attributes.size());
        for (uint32 i = 0; i < vertexInputAttributes.size(); i++)
        {
            const auto& attribute = vertexLayout.attributes[i];

            vertexInputAttributes[i] = {
                .location = shader.GetAttributeLocation(attribute.name),
                .binding = attribute.bufferIndex,
                .format = ToVulkan(attribute.format),
                .offset = attribute.offset,
            };
        }

        const vk::PipelineVertexInputStateCreateInfo vertexInput = {
            .vertexBindingDescriptionCount = size32(vertexInputBindings),
            .pVertexBindingDescriptions = data(vertexInputBindings),
            .vertexAttributeDescriptionCount = size32(vertexInputAttributes),
            .pVertexAttributeDescriptions = data(vertexInputAttributes),
        };

        const float depthBiasConstant
            = renderPass.renderOverrides.depthBiasConstant.value_or(renderStates.rasterState.depthBiasConstant);
        const float depthBiasSlope
            = renderPass.renderOverrides.depthBiasSlope.value_or(renderStates.rasterState.depthBiasSlope);

        const auto& rasterState = renderStates.rasterState;
        const vk::PipelineRasterizationStateCreateInfo rasterizationState = {
            .depthClampEnable = false,
            .rasterizerDiscardEnable = false,
            .polygonMode = ToVulkan(rasterState.fillMode),
            .cullMode = ToVulkan(rasterState.cullMode),
            .frontFace = vk::FrontFace::eCounterClockwise,
            .depthBiasEnable = depthBiasConstant != 0.0f || depthBiasSlope != 0.0f,
            .depthBiasConstantFactor = depthBiasConstant,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = depthBiasSlope,
            .lineWidth = rasterState.lineWidth,
        };

        const vk::PipelineDepthStencilStateCreateInfo depthStencilState = {
            .depthTestEnable = true,
            .depthWriteEnable = true,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = false,
            .stencilTestEnable = false,
        };

        const auto colorBlendAttachment = MakeBlendState(renderStates.blendState, renderStates.colorWriteMask);

        // Viewport and scissor will be dynamic states, so their initial values don't matter
        const auto viewport = vk::Viewport{};
        const auto scissor = vk::Rect2D{};

        const vk::PipelineViewportStateCreateInfo viewportState = {
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor,
        };

        const vk::PipelineMultisampleStateCreateInfo multisampleState = {
            .rasterizationSamples = vk::SampleCountFlagBits{renderPass.framebufferLayout.sampleCount},
            .sampleShadingEnable = false,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = false,
            .alphaToOneEnable = false,
        };

        const vk::PipelineColorBlendStateCreateInfo colorBlendState = {
            .logicOpEnable = false,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
        };

        const auto dynamicStates = std::array{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

        const vk::PipelineDynamicStateCreateInfo dynamicState = {
            .dynamicStateCount = size32(dynamicStates),
            .pDynamicStates = data(dynamicStates),
        };

        const vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {
            .topology = ToVulkan(vertexLayout.topology),
        };

        const vk::GraphicsPipelineCreateInfo createInfo = {
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
            .layout = shader.pipelineLayout.get(),
            .renderPass = renderPassLayout,
            .subpass = 0,
        };

        auto result = device.GetVulkanDevice().createGraphicsPipelineUnique(nullptr, createInfo, s_allocator);
        if (result.result != vk::Result::eSuccess)
        {
            throw VulkanError("Couldn't create graphics pipeline");
        }
        return std::move(result.value);
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

VulkanGraphicsDevice::VulkanGraphicsDevice(SDL_Window* window, uint32 numThreads) : m_workerDescriptorPools(numThreads)
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
    const std::array poolSizes = {
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = MaxFramesInFlight * 10,
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = MaxFramesInFlight * 10,
        },
    };

    const vk::DescriptorPoolCreateInfo poolCreateInfo = {
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

RendererPtr VulkanGraphicsDevice::CreateRenderer(ShaderEnvironmentPtr shaderEnvironment)
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
    auto ret = CreateBufferWithData(data.data, data.usage, data.lifetime, m_device.get(), *m_allocator, cmdBuffer);
    SetDebugName(ret.buffer, name);
    return std::make_shared<const VulkanBuffer>(std::move(ret));
}

ShaderEnvironmentPtr
VulkanGraphicsDevice::CreateShaderEnvironment(const ShaderEnvironmentData& data, const char* name [[maybe_unused]])
{
    auto shader = VulkanShaderEnvironmentBase{
        .scenePblockLayout = CreateParameterBlockLayout(data.scenePblock, 0),
        .viewPblockLayout = CreateParameterBlockLayout(data.viewPblock, 1),
    };

    return std::make_shared<const VulkanShaderEnvironment>(std::move(shader));
}

ShaderPtr VulkanGraphicsDevice::CreateShader(const ShaderData& data, const char* name)
{
    const vk::ShaderModuleCreateInfo vertexCreateInfo = {
        .codeSize = data.vertexShader.spirv.size() * sizeof(uint32_t),
        .pCode = data.vertexShader.spirv.data(),
    };
    const vk::ShaderModuleCreateInfo pixelCreateInfo = {
        .codeSize = data.pixelShader.spirv.size() * sizeof(uint32_t),
        .pCode = data.pixelShader.spirv.data(),
    };

    auto shader = VulkanShaderBase{
        .vertexShader = m_device->createShaderModuleUnique(vertexCreateInfo, s_allocator),
        .pixelShader = m_device->createShaderModuleUnique(pixelCreateInfo, s_allocator),
        .vertexShaderInputs = data.vertexShader.inputs,
        .scenePblockLayout = CreateParameterBlockLayout(data.environment.scenePblock, 0),
        .viewPblockLayout = CreateParameterBlockLayout(data.environment.viewPblock, 1),
        .materialPblockLayout = CreateParameterBlockLayout(data.materialPblock, 2),
        .objectPblockLayout = CreateParameterBlockLayout(data.objectPblock, 3),
    };

    shader.pipelineLayout = CreateGraphicsPipelineLayout(m_device.get(), shader);

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

TexturePtr VulkanGraphicsDevice::CreateRenderableTexture(const TextureData& data, const char* name)
{
    auto task = m_scheduler->ScheduleGpu([=, this](CommandBuffer& cmdBuffer) { //
        return CreateRenderableTexture(data, name, cmdBuffer);
    });
    return task.get().value();
}

TexturePtr VulkanGraphicsDevice::CreateRenderableTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer)
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

MeshPtr VulkanGraphicsDevice::CreateMesh(const MeshData& data, const char* name)
{
    auto task = m_scheduler->ScheduleGpu([data, name, this](CommandBuffer& cmdBuffer) { //
        return CreateMesh(data, name, cmdBuffer);
    });
    return task.get().value();
}

MeshPtr VulkanGraphicsDevice::CreateMesh(const MeshData& data, const char* name, CommandBuffer& cmdBuffer)
{
    VulkanMesh mesh;

    mesh.vertexLayout = data.vertexLayout;

    mesh.vertexBuffer = std::make_shared<VulkanBuffer>(CreateBufferWithData(
        data.vertexData, BufferUsage::Vertex, data.lifetime, m_device.get(), m_allocator.value(), cmdBuffer));
    SetDebugName(mesh.vertexBuffer->buffer, "{}:vbuffer", name);
    mesh.vertexCount = data.vertexCount;

    if (!data.indexData.empty())
    {
        mesh.indexBuffer = std::make_shared<VulkanBuffer>(CreateBufferWithData(
            data.indexData, BufferUsage::Index, data.lifetime, m_device.get(), m_allocator.value(), cmdBuffer));
        SetDebugName(mesh.indexBuffer->buffer, "{}:ibuffer", name);
        mesh.indexCount = static_cast<uint32>(data.indexData.size()) / sizeof(uint16);
    }

    mesh.aabb = data.aabb;

    return std::make_shared<VulkanMesh>(std::move(mesh));
}

PipelinePtr VulkanGraphicsDevice::CreatePipeline(const PipelineData& data)
{
    const auto shaderImpl = GetImpl(data.shader);

    const auto pipeline = std::make_shared<VulkanPipeline>(shaderImpl);

    for (const auto& renderPass : data.renderPasses)
    {
        pipeline->pipelines.push_back(
            {.renderPass = renderPass,
             .pipeline = CreateGraphicsPipeline(*shaderImpl, data.vertexLayout, data.renderStates, renderPass, *this)});
    }
    return pipeline;
}

vk::UniqueDescriptorSet VulkanGraphicsDevice::CreateDescriptorSet(
    vk::DescriptorPool pool, vk::DescriptorSetLayout layout, const Buffer* uniformBuffer,
    std::span<const TexturePtr> textures, const char* name)
{
    // TODO support multiple textures in descriptor sets
    assert(textures.size() <= 1 && "Multiple textures not yet supported!");

    const vk::DescriptorSetAllocateInfo allocInfo = {
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    auto descriptorSet = std::move(m_device->allocateDescriptorSetsUnique(allocInfo).front());
    SetDebugName(descriptorSet, name);

    const auto numUniformBuffers = uniformBuffer ? 1 : 0;

    std::vector<vk::WriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(numUniformBuffers + textures.size());

    if (uniformBuffer)
    {
        const auto& uniformBufferImpl = GetImpl(*uniformBuffer);
        const vk::DescriptorBufferInfo bufferInfo = {
            .buffer = uniformBufferImpl.buffer.get(),
            .offset = 0,
            .range = uniformBufferImpl.size,
        };

        if (uniformBufferImpl.size > 0)
        {
            descriptorWrites.push_back({
                .dstSet = descriptorSet.get(),
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
            .dstSet = descriptorSet.get(),
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfos.back(),
        });
    }

    m_device->updateDescriptorSets(descriptorWrites, {});

    return descriptorSet;
}

VulkanParameterBlockLayoutPtr VulkanGraphicsDevice::CreateParameterBlockLayout(const ParameterBlockDesc& desc, int set)
{
    VulkanParameterBlockLayout ret;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

    const ParameterBlockLayoutData layout = BuildParameterBlockLayout(desc, set);

    if (layout.uniformsSize > 0u)
    {
        if (layout.isPushConstant)
        {
            ret.pushConstantRange = {
                .stageFlags = GetShaderStageFlags(layout.uniformsStages),
                .offset = 0,
                .size = static_cast<uint32_t>(layout.uniformsSize),
            };
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

    for (uint32 i = 0; i < layout.textureCount; i++)
    {
        bindings.push_back({
            .binding = i + 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
        });
    }

    ret.setLayout = CreateDescriptorSetLayout(m_device.get(), bindings);
    ret.uniformsStages = GetShaderStageFlags(layout.uniformsStages);
    return std::make_shared<const VulkanParameterBlockLayout>(std::move(ret));
}

vk::RenderPass VulkanGraphicsDevice::CreateRenderPassLayout(const FramebufferLayout& framebufferLayout)
{
    return CreateRenderPass(framebufferLayout, {});
}

vk::RenderPass VulkanGraphicsDevice::CreateRenderPass(const FramebufferLayout& framebufferLayout, const ClearState& clearState)
{
    const RenderPassInfo renderPassInfo = {
        .colorLoadOp = clearState.colorValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
        .colorStoreOp = vk::AttachmentStoreOp::eStore,
        .depthLoadOp = clearState.depthValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
        .depthStoreOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = clearState.stencilValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eStore,
    };

    const auto desc = RenderPassDesc{framebufferLayout, renderPassInfo};

    const auto lock = std::scoped_lock(m_renderPassCacheMutex);
    const auto [it, inserted] = m_renderPassCache.emplace(desc, nullptr);
    if (inserted)
    {
        it->second = Teide::CreateRenderPass(m_device.get(), framebufferLayout, renderPassInfo);
    }
    return it->second.get();
}

Framebuffer VulkanGraphicsDevice::CreateFramebuffer(
    vk::RenderPass renderPass, const FramebufferLayout& layout, Geo::Size2i size, std::vector<vk::ImageView> attachments)
{
    const auto desc = FramebufferDesc{renderPass, size, std::move(attachments)};

    const auto framebuffer = [&] {
        const auto lock = std::scoped_lock(m_framebufferCacheMutex);

        const auto [it, inserted] = m_framebufferCache.emplace(desc, nullptr);
        if (inserted)
        {
            it->second = Teide::CreateFramebuffer(m_device.get(), desc.renderPass, desc.size, desc.attachments);
        }
        return it->second.get();
    }();

    return {
        .framebuffer = framebuffer,
        .layout = layout,
        .size = size,
    };
}

ParameterBlockPtr VulkanGraphicsDevice::CreateParameterBlock(const ParameterBlockData& data, const char* name)
{
    auto task = m_scheduler->ScheduleGpu([=, this](CommandBuffer& cmdBuffer) {
        return CreateParameterBlock(data, name, cmdBuffer, m_mainDescriptorPool.get());
    });
    return task.get().value();
}

ParameterBlockPtr VulkanGraphicsDevice::CreateParameterBlock(
    const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, uint32 threadIndex)
{
    return CreateParameterBlock(data, name, cmdBuffer, m_workerDescriptorPools[threadIndex].get());
}

ParameterBlockPtr VulkanGraphicsDevice::CreateParameterBlock(
    const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, vk::DescriptorPool descriptorPool)
{
    if (!data.layout)
    {
        return nullptr;
    }

    const auto& layout = GetImpl(*data.layout);
    const bool isPushConstant = layout.pushConstantRange.has_value();

    const auto setLayout = layout.setLayout.get();
    if (!setLayout && !isPushConstant)
    {
        return nullptr;
    }

    const auto descriptorSetName = DebugFormat("{}DescriptorSet", name);

    VulkanParameterBlock ret{layout};
    ret.textures = data.parameters.textures;
    if (setLayout)
    {
        if (!isPushConstant && !data.parameters.uniformData.empty())
        {
            const auto uniformBufferName = DebugFormat("{}UniformBuffer", name);
            ret.uniformBuffer = CreateBuffer(
                BufferData{
                    .usage = BufferUsage::Uniform,
                    .lifetime = data.lifetime,
                    .data = data.parameters.uniformData,
                },
                uniformBufferName.c_str(), cmdBuffer);
        }
        ret.descriptorSet = CreateDescriptorSet(
            descriptorPool, setLayout, ret.uniformBuffer.get(), data.parameters.textures, descriptorSetName.c_str());
    }

    if (isPushConstant)
    {
        ret.pushConstantData = data.parameters.uniformData;
    }

    return std::make_unique<VulkanParameterBlock>(std::move(ret));
}

} // namespace Teide
