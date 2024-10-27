
#include "VulkanDevice.h"

#include "CommandBuffer.h"
#include "DescriptorPool.h"
#include "VulkanBuffer.h"
#include "VulkanLoader.h"
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
#include "vkex/vkex.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>

namespace Teide
{

namespace
{
    const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

    std::string_view Trim(std::string_view str)
    {
        constexpr const char* whitespace = " \t\n\r";
        const auto strBegin = str.find_first_not_of(whitespace);
        if (strBegin == std::string::npos)
        {
            return {};
        }
        const auto strEnd = str.find_last_not_of(whitespace);
        const auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }

    vk::UniqueSurfaceKHR CreateVulkanSurface(SDL_Window* window, vk::Instance instance)
    {
        spdlog::info("Creating a Vulkan surface for a window");
        VkSurfaceKHR surfaceTmp = {};
        if (!SDL_Vulkan_CreateSurface(window, instance, &surfaceTmp))
        {
            throw VulkanError("Failed to create Vulkan surface for window");
        }
        TEIDE_ASSERT(surfaceTmp);
        spdlog::info("Surface created successfully");
        const auto deleter = vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderDynamic>(
            instance, s_allocator, VULKAN_HPP_DEFAULT_DISPATCHER);
        return vk::UniqueSurfaceKHR(surfaceTmp, deleter);
    }

    template <class F>
    std::optional<uint32> FindQueueFamily(std::span<const vk::QueueFamilyProperties> queueFamilies, const F& predicate)
    {
        uint32 i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (predicate(i, queueFamily))
            {
                return i;
            }
            i++;
        }
        return std::nullopt;
    }

    std::optional<QueueFamilies> FindQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        QueueFamilies ret;

        if (const auto index = FindQueueFamily(queueFamilies, [](uint32, const vk::QueueFamilyProperties& qf) {
                return (qf.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits{};
            }))
        {
            ret.graphicsFamily = *index;
        }
        else
        {
            return std::nullopt;
        }

        if (const auto index = FindQueueFamily(queueFamilies, [](uint32, const vk::QueueFamilyProperties& qf) {
                return (qf.queueFlags & vk::QueueFlagBits::eTransfer) != vk::QueueFlagBits{};
            }))
        {
            ret.transferFamily = *index;
        }
        else
        {
            return std::nullopt;
        }

        if (surface)
        {
            if (const auto index = FindQueueFamily(queueFamilies, [&](uint32 i, const vk::QueueFamilyProperties&) {
                    return physicalDevice.getSurfaceSupportKHR(i, surface);
                }))
            {
                ret.presentFamily = *index;
            }
            else
            {
                return std::nullopt;
            }
        }

        return ret;
    }

    bool IsDeviceSuitable(const PhysicalDevice& device, vk::SurfaceKHR surface)
    {
        // Check that all required extensions are supported
        const auto supportedExtensions = device.physicalDevice.enumerateDeviceExtensionProperties();
        const bool supportsAllExtensions
            = std::ranges::all_of(device.requiredExtensions, [&](std::string_view extensionName) {
                  return std::ranges::count(supportedExtensions, extensionName, GetExtensionName) > 0;
              });

        if (!supportsAllExtensions)
        {
            return false;
        }

        // Check all required features are supported
        const auto supportedFeatures = device.physicalDevice.getFeatures();
        if (!supportedFeatures.samplerAnisotropy)
        {
            return false;
        }

        if (surface)
        {
            // Check for surface support
            if (device.queueFamilies.presentFamily
                && !device.physicalDevice.getSurfaceSupportKHR(*device.queueFamilies.presentFamily, surface))
            {
                return false;
            }

            // Check for adequate swap chain support
            if (device.physicalDevice.getSurfaceFormatsKHR(surface).empty())
            {
                return false;
            }
            if (device.physicalDevice.getSurfacePresentModesKHR(surface).empty())
            {
                return false;
            }
        }

        return true;
    }

    PhysicalDevice FindPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
    {
        const auto makePhysicalDevice = [surface](vk::PhysicalDevice pd) -> std::optional<PhysicalDevice> {
            auto queueFamilies = FindQueueFamilies(pd, surface);
            if (!queueFamilies.has_value())
            {
                return {};
            }

            const auto physicalDevice = PhysicalDevice(pd, *queueFamilies);
            if (!IsDeviceSuitable(physicalDevice, surface))
            {
                return {};
            }

            return {physicalDevice};
        };

        std::vector<const char*> requiredExtensions;
        if (surface)
        {
            requiredExtensions.push_back("VK_KHR_swapchain");
        }

        // Look for a discrete GPU
        std::vector<PhysicalDevice> physicalDevices;
        for (const auto& device : instance.enumeratePhysicalDevices())
        {
            if (auto physicalDevice = makePhysicalDevice(device))
            {
                physicalDevices.push_back(std::move(*physicalDevice));
            }
        }
        if (physicalDevices.empty())
        {
            throw VulkanError("No suitable GPU found!");
        }

        // Prefer discrete GPUs to integrated GPUs
        std::ranges::sort(physicalDevices, [](const PhysicalDevice& a, const PhysicalDevice& b [[maybe_unused]]) {
            return a.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
        });

        if (IsSoftwareRenderingEnabled())
        {
            std::erase_if(physicalDevices, [&](const PhysicalDevice& device) {
                return device.properties.deviceType != vk::PhysicalDeviceType::eCpu;
            });

            if (physicalDevices.empty())
            {
                throw VulkanError("Software rendering was requested, but no Vulkan software implementation was found.");
            }
        }

        auto ret = physicalDevices.front();
        ret.requiredExtensions = std::move(requiredExtensions);

        ret.queueFamilyIndices = {ret.queueFamilies.graphicsFamily, ret.queueFamilies.transferFamily};
        if (ret.queueFamilies.presentFamily)
        {
            ret.queueFamilyIndices.push_back(*ret.queueFamilies.presentFamily);
        }
        std::ranges::sort(ret.queueFamilyIndices);
        ret.queueFamilyIndices.erase(
            std::unique(ret.queueFamilyIndices.begin(), ret.queueFamilyIndices.end()), ret.queueFamilyIndices.end());

        const auto& properties = ret.physicalDevice.getProperties();
        spdlog::info("Selected physical device: {}", Trim(properties.deviceName));
        return ret;
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

    vk::UniquePipelineLayout CreateGraphicsPipelineLayout(vk::Device device, const VulkanShaderBase& shader)
    {
        std::vector<vk::DescriptorSetLayout> setLayouts = {
            shader.scenePblockLayout->setLayout.get(),
            shader.viewPblockLayout->setLayout.get(),
            shader.materialPblockLayout->setLayout.get(),
            shader.objectPblockLayout->setLayout.get(),
        };

        vkex::PipelineLayoutCreateInfo createInfo = {
            .setLayouts = setLayouts,
        };
        if (const auto pushConstantRange = shader.objectPblockLayout->pushConstantRange)
        {
            createInfo.pushConstantRanges = *pushConstantRange;
        }

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
        const RenderPassDesc& renderPass, VulkanDevice& device)
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

} // namespace

//---------------------------------------------------------------------------------------------------------------------

DeviceAndSurface CreateDeviceAndSurface(SDL_Window* window, bool multisampled, const GraphicsSettings& settings)
{
    TEIDE_ASSERT(window);

    spdlog::info("Creating graphics device and surface");
    VulkanLoader loader;
    vk::UniqueInstance instance = CreateInstance(loader, window);

    vk::UniqueSurfaceKHR vksurface = CreateVulkanSurface(window, instance.get());

    auto physicalDevice = FindPhysicalDevice(instance.get(), vksurface.get());

    auto device
        = std::make_unique<VulkanDevice>(std::move(loader), std::move(instance), std::move(physicalDevice), settings);
    auto surface = device->CreateSurface(std::move(vksurface), window, multisampled);

    return {std::move(device), std::move(surface)};
}

DevicePtr CreateHeadlessDevice(const GraphicsSettings& settings)
{
    spdlog::info("Creating headless graphics device");

    VulkanLoader loader;
    vk::UniqueInstance instance = CreateInstance(loader);
    auto physicalDevice = FindPhysicalDevice(instance.get(), {});

    return std::make_unique<VulkanDevice>(std::move(loader), std::move(instance), std::move(physicalDevice), settings);
}

VulkanDevice::VulkanDevice(
    VulkanLoader loader, vk::UniqueInstance instance, Teide::PhysicalDevice physicalDevice, const GraphicsSettings& settings) :
    m_loader{std::move(loader)},
    m_instance{std::move(instance)},
    m_physicalDevice{std::move(physicalDevice)},
    m_device{CreateDevice(m_loader, m_physicalDevice)},
    m_settings{settings},
    m_graphicsQueue{m_device->getQueue(m_physicalDevice.queueFamilies.graphicsFamily, 0)},
    m_workerDescriptorPools(settings.numThreads),
    m_setupCommandPool{CreateCommandPool(m_physicalDevice.queueFamilies.graphicsFamily, m_device.get(), "SetupCommandPool")},
    m_surfaceCommandPool{
        CreateCommandPool(m_physicalDevice.queueFamilies.graphicsFamily, m_device.get(), "SurfaceCommandPool")},
    m_allocator{CreateAllocator(m_loader, m_instance.get(), m_device.get(), m_physicalDevice.physicalDevice)},
    m_textures{"texture"},
    m_scheduler(m_settings.numThreads, m_device.get(), m_graphicsQueue, m_physicalDevice.queueFamilies.graphicsFamily)
{
    if constexpr (IsDebugBuild)
    {
        if (VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateDebugUtilsMessengerEXT != nullptr)
        {
            m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(GetDebugCreateInfo(), s_allocator);
        }
    }

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

VulkanDevice::~VulkanDevice()
{
    m_device->waitIdle();
}

VulkanBuffer VulkanDevice::CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vma::AllocationCreateFlags allocationFlags, vma::MemoryUsage memoryUsage)
{
    return Teide::CreateBufferUninitialized(size, usage, allocationFlags, memoryUsage, m_device.get(), m_allocator.get());
}

VulkanBuffer
VulkanDevice::CreateBufferWithData(BytesView data, BufferUsage usage, ResourceLifetime lifetime, CommandBuffer& cmdBuffer)
{
    const vk::BufferUsageFlags usageFlags = GetBufferUsageFlags(usage);

    if (lifetime == ResourceLifetime::Transient)
    {
        VulkanBuffer ret = CreateBufferUninitialized(
            data.size(), usageFlags | vk::BufferUsageFlagBits::eTransferDst,
            vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);
        SetBufferData(ret, data);
        return ret;
    }

    // Create staging buffer
    auto stagingBuffer = std::make_shared<VulkanBuffer>(CreateBufferUninitialized(
        data.size(), vk::BufferUsageFlagBits::eTransferSrc, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite));
    cmdBuffer.AddReference(stagingBuffer);
    SetBufferData(*stagingBuffer, data);

    // Create device-local buffer
    VulkanBuffer ret = CreateBufferUninitialized(data.size(), usageFlags | vk::BufferUsageFlagBits::eTransferDst);
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

vk::UniqueSampler VulkanDevice::CreateSampler(const SamplerState& ss)
{
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
        .minLod = 0,
        .maxLod = VK_LOD_CLAMP_NONE,
    };
    return m_device->createSamplerUnique(samplerInfo, s_allocator);
}

TextureState VulkanDevice::CreateTextureImpl(VulkanTexture& texture, vk::ImageUsageFlags usage)
{
    // For now, all textures will be created with TransferSrc so they can be copied from
    usage |= vk::ImageUsageFlagBits::eTransferSrc;

    const auto& props = texture.properties;

    auto initialState = TextureState{
        .layout = vk::ImageLayout::eUndefined,
        .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTopOfPipe,
    };

    // Create image
    const auto imageExtent = vk::Extent3D{props.size.x, props.size.y, 1};
    const vk::ImageCreateInfo imageInfo = {
        .imageType = vk::ImageType::e2D,
        .format = ToVulkan(props.format),
        .extent = imageExtent,
        .mipLevels = props.mipLevelCount,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits{props.sampleCount},
        .tiling = vk::ImageTiling::eOptimal,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = initialState.layout,
    };

    const vma::AllocationCreateInfo allocInfo = {
        .usage = vma::MemoryUsage::eAuto,
    };
    auto [image, allocation] = m_allocator->createImageUnique(imageInfo, allocInfo);

    // Create image view
    const vk::ImageViewCreateInfo viewInfo = {
        .image = image.get(),
        .viewType = vk::ImageViewType::e2D,
        .format = imageInfo.format,
        .subresourceRange = {
            .aspectMask =  GetImageAspect(props.format),
            .baseMipLevel = 0,
            .levelCount = props.mipLevelCount,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    auto imageView = m_device->createImageViewUnique(viewInfo, s_allocator);

    texture.image = vk::UniqueImage(image.release(), m_device.get());
    texture.allocation = std::move(allocation);
    texture.imageView = std::move(imageView);

    if (!props.name.empty())
    {
        SetDebugName(texture.image, "{}", props.name);
        SetDebugName(texture.imageView, "{}:View", props.name);
        SetDebugName(texture.sampler, "{}:Sampler", props.name);
    }

    return initialState;
}

void VulkanDevice::SetBufferData(VulkanBuffer& buffer, BytesView data)
{
    const auto allocation = buffer.allocation.get();
    TEIDE_ASSERT(m_allocator->getAllocationInfo(allocation).size >= data.size());

    void* const mappedData = m_allocator->mapMemory(allocation);

    std::memcpy(mappedData, data.data(), data.size());

    m_allocator->unmapMemory(allocation);
}

SurfacePtr VulkanDevice::CreateSurface(SDL_Window* window, bool multisampled)
{
    return CreateSurface(CreateVulkanSurface(window, m_instance.get()), window, multisampled);
}

RendererPtr VulkanDevice::CreateRenderer(ShaderEnvironmentPtr shaderEnvironment)
{
    return std::make_unique<VulkanRenderer>(*this, m_physicalDevice.queueFamilies, std::move(shaderEnvironment));
}

BufferPtr VulkanDevice::CreateBuffer(const BufferData& data, const char* name)
{
    spdlog::debug("Creating buffer '{}' of size {}", name, data.data.size());
    auto task = m_scheduler.ScheduleGpu([data, name, this](CommandBuffer& cmdBuffer) { //
        return CreateBuffer(data, name, cmdBuffer);
    });
    return task.get();
}

SurfacePtr VulkanDevice::CreateSurface(vk::UniqueSurfaceKHR surface, SDL_Window* window, bool multisampled)
{
    spdlog::info("Creating a new surface for a window");

    TEIDE_ASSERT(m_physicalDevice.queueFamilies.presentFamily.has_value());

    return std::make_unique<VulkanSurface>(
        window, std::move(surface), m_device.get(), m_physicalDevice, m_surfaceCommandPool.get(), m_allocator.get(),
        m_graphicsQueue, multisampled);
}

BufferPtr VulkanDevice::CreateBuffer(const BufferData& data, const char* name, CommandBuffer& cmdBuffer)
{
    auto ret = CreateBufferWithData(data.data, data.usage, data.lifetime, cmdBuffer);
    SetDebugName(ret.buffer, name);
    return std::make_shared<const VulkanBuffer>(std::move(ret));
}

VulkanBuffer VulkanDevice::CreateTransientBuffer(const BufferData& data, const char* name)
{
    auto nullCmdBuffer = CommandBuffer(vk::UniqueCommandBuffer{nullptr});
    auto ret = CreateBufferWithData(data.data, data.usage, ResourceLifetime::Transient, nullCmdBuffer);
    SetDebugName(ret.buffer, name);
    return ret;
}

ShaderEnvironmentPtr VulkanDevice::CreateShaderEnvironment(const ShaderEnvironmentData& data, const char* name [[maybe_unused]])
{
    spdlog::debug("Creating shader environment");
    auto shader = VulkanShaderEnvironmentBase{
        .scenePblockLayout = CreateParameterBlockLayout(data.scenePblock, 0),
        .viewPblockLayout = CreateParameterBlockLayout(data.viewPblock, 1),
    };

    return std::make_shared<const VulkanShaderEnvironment>(std::move(shader));
}

ShaderPtr VulkanDevice::CreateShader(const ShaderData& data, const char* name)
{
    spdlog::debug("Creating shader '{}'", name);
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

    if (name)
    {
        SetDebugName(shader.vertexShader, "{}:Vertex", name);
        SetDebugName(shader.pixelShader, "{}:Pixel", name);
        SetDebugName(shader.pipelineLayout, "{}:PipelineLayout", name);
    }

    return std::make_shared<const VulkanShader>(std::move(shader));
}

Texture VulkanDevice::CreateTexture(const TextureData& data, const char* name)
{
    spdlog::debug("Creating texture '{}' of size {}x{}", name, data.size.x, data.size.y);
    auto task = m_scheduler.ScheduleGpu([data, name, this](CommandBuffer& cmdBuffer) { //
        return CreateTexture(data, name, cmdBuffer);
    });
    return task.get();
}

Texture VulkanDevice::CreateTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer)
{
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;

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

    VulkanTexture texture;
    texture.properties = {
        .size = data.size,
        .format = data.format,
        .mipLevelCount = data.mipLevelCount,
        .sampleCount = data.sampleCount,
        .name = name,
    };
    texture.sampler = CreateSampler(data.samplerState);

    auto state = CreateTextureImpl(texture, usage);

    if (!data.pixels.empty())
    {
        const auto imageExtent = vk::Extent3D{data.size.x, data.size.y, 1};

        // Create staging buffer
        auto stagingBuffer = CreateBufferUninitialized(
            data.pixels.size(), vk::BufferUsageFlagBits::eTransferSrc,
            vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);
        SetBufferData(stagingBuffer, data.pixels);

        // Copy staging buffer to image
        TransitionImageLayout(
            cmdBuffer, texture.image.get(), data.format, data.mipLevelCount, state.layout,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer);
        state = {
            .layout = vk::ImageLayout::eTransferDstOptimal,
            .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTransfer,
        };
        CopyBufferToImage(cmdBuffer, stagingBuffer.buffer.get(), texture.image.get(), data.format, imageExtent);

        cmdBuffer.TakeOwnership(std::move(stagingBuffer.buffer));
        cmdBuffer.TakeOwnership(std::move(stagingBuffer.allocation));
    }

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

    return m_textures.Insert(std::move(texture));
}

Texture VulkanDevice::CreateRenderableTexture(const TextureData& data, const char* name)
{
    spdlog::debug("Creating renderable texture '{}' of size {}x{}", name, data.size.x, data.size.y);
    auto task = m_scheduler.ScheduleGpu([data, name, this](CommandBuffer& cmdBuffer) { //
        return CreateRenderableTexture(data, name, cmdBuffer);
    });
    return task.get();
}

Texture VulkanDevice::CreateRenderableTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer)
{
    const bool isColorTarget = !HasDepthOrStencilComponent(data.format);
    const auto renderUsage
        = isColorTarget ? vk::ImageUsageFlagBits::eColorAttachment : vk::ImageUsageFlagBits::eDepthStencilAttachment;

    const vk::ImageUsageFlags usage = renderUsage | vk::ImageUsageFlagBits::eSampled;

    VulkanTexture texture;
    texture.properties = {
        .size = data.size,
        .format = data.format,
        .mipLevelCount = data.mipLevelCount,
        .sampleCount = data.sampleCount,
        .name = name,
    };
    texture.sampler = CreateSampler(data.samplerState);

    auto state = CreateTextureImpl(texture, usage);

    if (isColorTarget)
    {
        texture.TransitionToColorTarget(state, cmdBuffer);
    }
    else
    {
        texture.TransitionToDepthStencilTarget(state, cmdBuffer);
    }

    return m_textures.Insert(std::move(texture));
}

MeshPtr VulkanDevice::CreateMesh(const MeshData& data, const char* name)
{
    spdlog::debug("Creating mesh '{}' with {} vertices and {} indices", name, data.vertexCount, data.indexData.size() / 2);
    auto task = m_scheduler.ScheduleGpu([data, name, this](CommandBuffer& cmdBuffer) { //
        return CreateMesh(data, name, cmdBuffer);
    });
    return task.get();
}

MeshPtr VulkanDevice::CreateMesh(const MeshData& data, const char* name, CommandBuffer& cmdBuffer)
{
    VulkanMesh mesh;

    mesh.vertexLayout = data.vertexLayout;

    mesh.vertexBuffer = std::make_shared<VulkanBuffer>(
        CreateBufferWithData(data.vertexData, BufferUsage::Vertex, data.lifetime, cmdBuffer));
    if (name)
    {
        SetDebugName(mesh.vertexBuffer->buffer, "{}:vbuffer", name);
    }
    mesh.vertexCount = data.vertexCount;

    if (!data.indexData.empty())
    {
        mesh.indexBuffer = std::make_shared<VulkanBuffer>(
            CreateBufferWithData(data.indexData, BufferUsage::Index, data.lifetime, cmdBuffer));
        if (name)
        {
            SetDebugName(mesh.indexBuffer->buffer, "{}:ibuffer", name);
        }
        mesh.indexCount = static_cast<uint32>(data.indexData.size()) / sizeof(uint16);
    }

    mesh.aabb = data.aabb;

    return std::make_shared<VulkanMesh>(std::move(mesh));
}

PipelinePtr VulkanDevice::CreatePipeline(const PipelineData& data)
{
    spdlog::debug("Creating pipeline");
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

vk::UniqueDescriptorSet VulkanDevice::CreateUniqueDescriptorSet(
    vk::DescriptorPool pool, vk::DescriptorSetLayout layout, const Buffer* uniformBuffer,
    std::span<const Texture> textures, const char* name)
{
    const vk::DescriptorSetAllocateInfo allocInfo = {
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    auto descriptorSet = std::move(m_device->allocateDescriptorSetsUnique(allocInfo).front());
    SetDebugName(descriptorSet, name);

    WriteDescriptorSet(descriptorSet.get(), uniformBuffer, textures);

    return descriptorSet;
}

void VulkanDevice::WriteDescriptorSet(vk::DescriptorSet descriptorSet, const Buffer* uniformBuffer, std::span<const Texture> textures)
{
    const auto numUniformBuffers = uniformBuffer ? 1 : 0;

    std::vector<vk::WriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(numUniformBuffers + textures.size());

    vk::DescriptorBufferInfo bufferInfo;
    if (uniformBuffer)
    {
        const auto& uniformBufferImpl = GetImpl(*uniformBuffer);
        bufferInfo = vk::DescriptorBufferInfo{
            .buffer = uniformBufferImpl.buffer.get(),
            .offset = 0,
            .range = uniformBufferImpl.size,
        };

        if (uniformBufferImpl.size > 0)
        {
            descriptorWrites.push_back({
                .dstSet = descriptorSet,
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
        const VulkanTexture& textureImpl = GetImpl(texture);

        imageInfos.push_back({
            .sampler = textureImpl.sampler.get(),
            .imageView = textureImpl.imageView.get(),
            .imageLayout = HasDepthOrStencilComponent(textureImpl.properties.format)
                ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                : vk::ImageLayout::eShaderReadOnlyOptimal,
        });

        descriptorWrites.push_back({
            .dstSet = descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfos.back(),
        });
    }

    m_device->updateDescriptorSets(descriptorWrites, {});
}

VulkanParameterBlockLayoutPtr VulkanDevice::CreateParameterBlockLayout(const ParameterBlockDesc& desc, int set)
{
    return std::make_shared<const VulkanParameterBlockLayout>(BuildParameterBlockLayout(desc, set), m_device.get());
}

vk::RenderPass VulkanDevice::CreateRenderPassLayout(const FramebufferLayout& framebufferLayout)
{
    return CreateRenderPass(framebufferLayout, {}, {});
}

vk::RenderPass
VulkanDevice::CreateRenderPass(const FramebufferLayout& framebufferLayout, const ClearState& clearState, FramebufferUsage usage)
{
    const RenderPassInfo renderPassInfo = {
        .colorLoadOp = clearState.colorValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
        .colorStoreOp = framebufferLayout.captureColor ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare,
        .depthLoadOp = clearState.depthValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
        .depthStoreOp = framebufferLayout.captureDepthStencil ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = clearState.stencilValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp
        = framebufferLayout.captureDepthStencil ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare,
    };

    const auto desc = RenderPassDesc{framebufferLayout, renderPassInfo, usage};

    const auto lock = std::scoped_lock(m_renderPassCacheMutex);
    const auto [it, inserted] = m_renderPassCache.emplace(desc, nullptr);
    if (inserted)
    {
        it->second = Teide::CreateRenderPass(m_device.get(), framebufferLayout, usage, renderPassInfo);
    }
    return it->second.get();
}

Framebuffer VulkanDevice::CreateFramebuffer(
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

ParameterBlockPtr VulkanDevice::CreateParameterBlock(const ParameterBlockData& data, const char* name)
{
    spdlog::debug("Creating parameter block '{}'", name);
    auto task = m_scheduler.ScheduleGpu([data, name, this](CommandBuffer& cmdBuffer) {
        return CreateParameterBlock(data, name, cmdBuffer, m_mainDescriptorPool.get());
    });
    return task.get();
}

ParameterBlockPtr
VulkanDevice::CreateParameterBlock(const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, uint32 threadIndex)
{
    return CreateParameterBlock(data, name, cmdBuffer, m_workerDescriptorPools[threadIndex].get());
}

ParameterBlockPtr VulkanDevice::CreateParameterBlock(
    const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, vk::DescriptorPool descriptorPool)
{
    if (!data.layout)
    {
        return nullptr;
    }

    const auto& layout = GetImpl(*data.layout);
    const bool isPushConstant = layout.HasPushConstants();

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
            ret.uniformBuffer = MakeHandle(
                CreateBufferWithData(data.parameters.uniformData, BufferUsage::Uniform, data.lifetime, cmdBuffer));
            SetDebugName(ret.uniformBuffer->buffer, "{}UniformBuffer", name);
        }
        ret.descriptorSet = CreateUniqueDescriptorSet(
            descriptorPool, setLayout, ret.uniformBuffer.get(), data.parameters.textures, descriptorSetName.c_str());
    }

    if (isPushConstant)
    {
        ret.pushConstantData = data.parameters.uniformData;
    }

    return std::make_unique<VulkanParameterBlock>(std::move(ret));
}

TransientParameterBlock
VulkanDevice::CreateTransientParameterBlock(const ParameterBlockData& data, const char* name, DescriptorPool& descriptorPool)
{
    TEIDE_ASSERT(data.layout);

    const auto& layout = GetImpl(*data.layout);
    TEIDE_ASSERT(!layout.pushConstantRange.has_value());

    const auto setLayout = layout.setLayout.get();
    TEIDE_ASSERT(setLayout);

    const auto descriptorSetName = DebugFormat("{}DescriptorSet", name);

    TransientParameterBlock ret;
    ret.textures = data.parameters.textures;
    if (setLayout)
    {
        if (layout.uniformBufferSize > 0)
        {
            const auto uniformBufferName = DebugFormat("{}UniformBuffer", name);
            ret.uniformBuffer = MakeHandle(CreateTransientBuffer(
                BufferData{
                    .usage = BufferUsage::Uniform,
                    .lifetime = data.lifetime,
                    .data = data.parameters.uniformData.empty() ? std::vector<byte>(layout.uniformBufferSize)
                                                                : data.parameters.uniformData,
                },
                uniformBufferName.c_str()));
        }
        ret.descriptorSet = descriptorPool.Allocate(descriptorSetName.c_str());
        WriteDescriptorSet(ret.descriptorSet, ret.uniformBuffer.get(), ret.textures);
    }

    return ret;
}

void VulkanDevice::UpdateTransientParameterBlock(TransientParameterBlock& pblock, const ParameterBlockData& data)
{
    pblock.textures = data.parameters.textures;

    if (pblock.uniformBuffer)
    {
        SetBufferData(*pblock.uniformBuffer, data.parameters.uniformData);
    }

    if (pblock.descriptorSet)
    {
        WriteDescriptorSet(pblock.descriptorSet, pblock.uniformBuffer.get(), pblock.textures);
    }
}

} // namespace Teide
