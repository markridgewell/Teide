
#include "Vulkan.h"

#include "VulkanLoader.h"

#include "Teide/Assert.h"
#include "Teide/Definitions.h"
#include "Teide/Pipeline.h"
#include "Teide/Renderer.h"
#include "Teide/TextureData.h"
#include "Teide/Util/StaticMap.h"

#include <SDL_vulkan.h>
#include <cpptrace/cpptrace.hpp>
#include <spdlog/spdlog.h>

#include <memory>

namespace Teide
{

namespace
{
    constexpr auto VulkanApiVersion = VK_API_VERSION_1_1;
    constexpr bool BreakOnVulkanWarning = false;
    constexpr bool BreakOnVulkanError = true;

    const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

    template <std::ranges::input_range R, class V, class P = std::identity>
        requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, P>, const V*>
    constexpr bool contains(R&& range, const V& value, P&& proj = {})
    {
        return std::ranges::count(std::forward<R>(range), value, std::forward<P>(proj)) > 0;
    }

    constexpr StaticMap<Format, vk::Format, FormatCount> VulkanFormats = {
        {Format::Unknown, vk::Format::eUndefined},

        {Format::Byte1, vk::Format::eR8Uint},
        {Format::Byte1Norm, vk::Format::eR8Unorm},
        {Format::Short1, vk::Format::eR16Sint},
        {Format::Short1Norm, vk::Format::eR16Snorm},
        {Format::Ushort1, vk::Format::eR16Uint},
        {Format::Ushort1Norm, vk::Format::eR16Unorm},
        {Format::Half1, vk::Format::eR16Sfloat},
        {Format::Int1, vk::Format::eR32Sint},
        {Format::Uint1, vk::Format::eR32Uint},
        {Format::Float1, vk::Format::eR32Sfloat},

        {Format::Byte2, vk::Format::eR8G8Uint},
        {Format::Byte2Norm, vk::Format::eR8G8Unorm},
        {Format::Short2, vk::Format::eR16G16Sint},
        {Format::Short2Norm, vk::Format::eR16G16Snorm},
        {Format::Ushort2, vk::Format::eR16G16Uint},
        {Format::Ushort2Norm, vk::Format::eR16G16Unorm},
        {Format::Half2, vk::Format::eR16G16Sfloat},
        {Format::Int2, vk::Format::eR32G32Sint},
        {Format::Uint2, vk::Format::eR32G32Uint},
        {Format::Float2, vk::Format::eR32G32Sfloat},

        {Format::Byte3, vk::Format::eR8G8B8Uint},
        {Format::Byte3Norm, vk::Format::eR8G8B8Unorm},
        {Format::Short3, vk::Format::eR16G16B16Sint},
        {Format::Short3Norm, vk::Format::eR16G16B16Snorm},
        {Format::Ushort3, vk::Format::eR16G16B16Uint},
        {Format::Ushort3Norm, vk::Format::eR16G16B16Unorm},
        {Format::Half3, vk::Format::eR16G16B16Sfloat},
        {Format::Int3, vk::Format::eR32G32B32Sint},
        {Format::Uint3, vk::Format::eR32G32B32Uint},
        {Format::Float3, vk::Format::eR32G32B32Sfloat},

        {Format::Byte4, vk::Format::eR8G8B8A8Uint},
        {Format::Byte4Norm, vk::Format::eR8G8B8A8Unorm},
        {Format::Byte4Srgb, vk::Format::eR8G8B8A8Srgb},
        {Format::Byte4SrgbBGRA, vk::Format::eB8G8R8A8Srgb},
        {Format::Short4, vk::Format::eR16G16B16A16Sint},
        {Format::Short4Norm, vk::Format::eR16G16B16A16Snorm},
        {Format::Ushort4, vk::Format::eR16G16B16A16Uint},
        {Format::Ushort4Norm, vk::Format::eR16G16B16A16Unorm},
        {Format::Half4, vk::Format::eR16G16B16A16Sfloat},
        {Format::Int4, vk::Format::eR32G32B32A32Sint},
        {Format::Uint4, vk::Format::eR32G32B32A32Uint},
        {Format::Float4, vk::Format::eR32G32B32A32Sfloat},

        {Format::Depth16, vk::Format::eD16Unorm},
        {Format::Depth32, vk::Format::eD32Sfloat},
        {Format::Depth16Stencil8, vk::Format::eD16UnormS8Uint},
        {Format::Depth24Stencil8, vk::Format::eD24UnormS8Uint},
        {Format::Depth32Stencil8, vk::Format::eD32SfloatS8Uint},
        {Format::Stencil8, vk::Format::eS8Uint},
    };

    bool IsUnwantedMessage(const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData)
    {
        // Filter unwanted messages
        constexpr int32 UnwantedMessages[] = {
            0,           // Loader Message
            767975156,   // UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension
            -2111305990, // UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension-debugging
            -671457468,  // UNASSIGNED-khronos-validation-createinstance-status-message
            -1993852625, // UNASSIGNED-BestPractices-NonSuccess-Result
        };
        return contains(UnwantedMessages, pCallbackData->messageIdNumber);
    }

    std::string GetLogPrefix(vk::DebugUtilsMessageTypeFlagsEXT type)
    {
        using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
        std::string ret;
        if (type & eValidation)
        {
            ret += "[validation] ";
        }
        if (type & ePerformance)
        {
            ret += "[performance] ";
        }
        return ret;
    }

    spdlog::level::level_enum GetLogLevel(vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
    {
        using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        switch (severity)
        {
            case eVerbose: return spdlog::level::debug;
            case eInfo: return spdlog::level::info;
            case eWarning: return spdlog::level::warn;
            case eError: return spdlog::level::err;
        }
        Unreachable();
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
    {
        using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;

        // Don't hide unwanted messages, just force them to only be visible at highest verbosity level
        const auto severity = IsUnwantedMessage(pCallbackData) ? MessageSeverity::eVerbose : messageSeverity;
        const auto prefix = GetLogPrefix(type);
        const auto logLevel = GetLogLevel(severity);

        spdlog::log(logLevel, "{}{}", prefix, pCallbackData->pMessage);

        if constexpr (BreakOnVulkanWarning)
        {
            // Vulkan warning triggered a debug break
            TEIDE_ASSERT(severity != MessageSeverity::eWarning, "{}", pCallbackData->pMessage);
        }
        if constexpr (BreakOnVulkanError)
        {
            // Vulkan error triggered a debug break
            TEIDE_ASSERT(severity != MessageSeverity::eError, "{}", pCallbackData->pMessage);
        }

        if (severity == MessageSeverity::eError)
        {
            std::cout << "Stack trace:\n";
            cpptrace::generate_trace().print();
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
            case eUndefined: return {};

            case eTransferDstOptimal: return Access::eTransferWrite;
            case eColorAttachmentOptimal: return Access::eColorAttachmentRead | Access::eColorAttachmentWrite;

            case eDepthStencilAttachmentOptimal:
            case eDepthAttachmentOptimal:
            case eStencilAttachmentOptimal:
                return Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite;

            case eShaderReadOnlyOptimal: return Access::eShaderRead;
            case eTransferSrcOptimal: return Access::eTransferRead;
            case eDepthStencilReadOnlyOptimal: return Access::eShaderRead;
            case ePresentSrcKHR: return Access::eNoneKHR;

            default: TEIDE_BREAK("Unsupported image transition"); return {};
        }
    }

    TransitionAccessMasks GetTransitionAccessMasks(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
    {
        return {.source = GetTransitionAccessMask(oldLayout), .destination = GetTransitionAccessMask(newLayout)};
    }

    vk::ImageLayout GetColorImageLayout(FramebufferUsage usage)
    {
        switch (usage)
        {
            case FramebufferUsage::PresentSrc: return vk::ImageLayout::ePresentSrcKHR;
            case FramebufferUsage::ShaderInput: return vk::ImageLayout::eShaderReadOnlyOptimal;
        }
        return vk::ImageLayout::eUndefined;
    }

    vk::ImageLayout GetDepthStencilImageLayout(FramebufferUsage usage)
    {
        switch (usage)
        {
            case FramebufferUsage::PresentSrc: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            case FramebufferUsage::ShaderInput: return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        }
        return vk::ImageLayout::eUndefined;
    }

    template <class T>
    T* ToPointer(std::optional<T>& opt)
    {
        return opt.has_value() ? &opt.value() : nullptr;
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

PhysicalDevice::PhysicalDevice(vk::PhysicalDevice pd, const QueueFamilies& qf) : physicalDevice{pd}, queueFamilies{qf}
{
    const auto props = pd.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceDepthStencilResolveProperties>();

    properties = props.get<vk::PhysicalDeviceProperties2>().properties;
    depthStencilResolveProperties = props.get<vk::PhysicalDeviceDepthStencilResolveProperties>();
}

void EnableVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& availableLayers,
    const char* layerName, Required required)
{
    if (contains(availableLayers, std::string_view(layerName), GetLayerName))
    {
        spdlog::info("Enabling Vulkan layer {}", layerName);
        enabledLayers.push_back(layerName);
    }
    else if (required == Required::True)
    {
        throw vk::LayerNotPresentError(layerName);
    }
    else
    {
        spdlog::warn("Vulkan layer {} not enabled!", layerName);
    }
}

void EnableVulkanExtension(
    std::vector<const char*>& enabledExtensions, const std::vector<vk::ExtensionProperties>& availableExtensions,
    const char* extensionName, Required required)
{
    if (contains(availableExtensions, std::string_view(extensionName), GetExtensionName))
    {
        spdlog::info("Enabling Vulkan extension {}", extensionName);
        enabledExtensions.push_back(extensionName);
    }
    else if (required == Required::True)
    {
        throw vk::ExtensionNotPresentError(extensionName);
    }
    else
    {
        spdlog::warn("Vulkan extension {} not enabled!", extensionName);
    }
}

vk::UniqueInstance CreateInstance(VulkanLoader& loader, SDL_Window* window)
{
    std::vector<const char*> extensions;
    if (window)
    {
        uint32_t extensionCount = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
        extensions.resize(extensionCount);
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
    }

    const vk::ApplicationInfo applicationInfo{
        .apiVersion = VulkanApiVersion,
    };

    const auto availableLayers = vk::enumerateInstanceLayerProperties();
    const auto availableExtensions = vk::enumerateInstanceExtensionProperties();

    std::vector<const char*> layers;

    vk::UniqueInstance instance;
    if constexpr (IsDebugBuild)
    {
        EnableVulkanLayer(layers, availableLayers, "VK_LAYER_KHRONOS_validation", Required::False);
        EnableVulkanExtension(extensions, availableExtensions, "VK_EXT_debug_utils", Required::False);

        const std::array enabledFeatures = {
            vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
            vk::ValidationFeatureEnableEXT::eBestPractices,
        };

        const vk::StructureChain createInfo = {
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
        const vk::InstanceCreateInfo createInfo = {
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = size32(layers),
            .ppEnabledLayerNames = data(layers),
            .enabledExtensionCount = size32(extensions),
            .ppEnabledExtensionNames = data(extensions),
        };

        instance = vk::createInstanceUnique(createInfo, s_allocator);
    }

    loader.LoadInstanceFunctions(instance.get());
    return instance;
}

vk::UniqueDevice CreateDevice(VulkanLoader& loader, const PhysicalDevice& physicalDevice)
{
    // Make a list of create infos for each unique queue we wish to create
    const float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (const uint32_t index : physicalDevice.queueFamilyIndices)
    {
        if (not contains(queueCreateInfos, index, &vk::DeviceQueueCreateInfo::queueFamilyIndex))
        {
            queueCreateInfos.push_back({.queueFamilyIndex = index, .queueCount = 1, .pQueuePriorities = &queuePriority});
        }
    }

    const vk::PhysicalDeviceFeatures deviceFeatures = {
        .samplerAnisotropy = true,
    };

    const auto availableLayers = physicalDevice.physicalDevice.enumerateDeviceLayerProperties();
    const auto availableExtensions = physicalDevice.physicalDevice.enumerateDeviceExtensionProperties();

    std::vector<const char*> layers;
    if constexpr (IsDebugBuild)
    {
        EnableVulkanLayer(layers, availableLayers, "VK_LAYER_KHRONOS_validation", Required::False);
    }

    std::vector<const char*> extensions = physicalDevice.requiredExtensions;
    EnableVulkanExtension(extensions, availableExtensions, "VK_EXT_descriptor_indexing", Required::True);
    EnableVulkanExtension(extensions, availableExtensions, "VK_KHR_depth_stencil_resolve", Required::True);
    EnableVulkanExtension(
        extensions, availableExtensions, "VK_KHR_create_renderpass2",
        Required::True); // required by "VK_KHR_depth_stencil_resolve"

    const vk::StructureChain createInfo = {
        vk::DeviceCreateInfo{
            .queueCreateInfoCount = size32(queueCreateInfos),
            .pQueueCreateInfos = data(queueCreateInfos),
            .enabledLayerCount = size32(layers),
            .ppEnabledLayerNames = data(layers),
            .enabledExtensionCount = size32(extensions),
            .ppEnabledExtensionNames = data(extensions),
            .pEnabledFeatures = &deviceFeatures,
        },
        vk::PhysicalDeviceDescriptorIndexingFeatures{
            // Enable non uniform array indexing
            // (#extension GL_EXT_nonuniform_qualifier : require)
            .shaderSampledImageArrayNonUniformIndexing = true,
            .shaderStorageBufferArrayNonUniformIndexing = true,
            .shaderStorageImageArrayNonUniformIndexing = true,
            // All of these enables to update after the
            // commandbuffer used the bindDescriptorsSet
            .descriptorBindingSampledImageUpdateAfterBind = true,
            .descriptorBindingStorageImageUpdateAfterBind = true,
            .descriptorBindingStorageBufferUpdateAfterBind = true,
            // Enable non bound descriptors slots
            .descriptorBindingPartiallyBound = true,
            // Enable non sized arrays
            .runtimeDescriptorArray = true,
        },
    };

    auto ret = physicalDevice.physicalDevice.createDeviceUnique(createInfo.get<vk::DeviceCreateInfo>(), s_allocator);
    loader.LoadDeviceFunctions(ret.get());
    return ret;
}

vma::UniqueAllocator CreateAllocator(VulkanLoader& loader, vk::Instance instance, vk::Device device, vk::PhysicalDevice physicalDevice)
{
    return vma::createAllocatorUnique({
        .physicalDevice = physicalDevice,
        .device = device,
        .pAllocationCallbacks = s_allocator,
        .pVulkanFunctions = &loader.GetAllocatorFunctions(),
        .instance = instance,
        .vulkanApiVersion = VulkanApiVersion,
    });
}

void TransitionImageLayout(
    vk::CommandBuffer cmdBuffer, vk::Image image, Format format, uint32_t mipLevelCount, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask)
{
    const auto accessMasks = GetTransitionAccessMasks(oldLayout, newLayout);

    const vk::ImageMemoryBarrier barrier = {
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

vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device, const char* debugName)
{
    const vk::CommandPoolCreateInfo createInfo = {
        .queueFamilyIndex = queueFamilyIndex,
    };

    auto ret = device.createCommandPoolUnique(createInfo, s_allocator);
    SetDebugName(ret, debugName);
    return ret;
}

vk::ImageAspectFlags GetImageAspect(Format format)
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

void CopyBufferToImage(vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Image destination, Format imageFormat, vk::Extent3D imageExtent)
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
        .imageOffset = {.x=0,.y=0,.z=0},
        .imageExtent = imageExtent,
    };
    cmdBuffer.copyBufferToImage(source, destination, vk::ImageLayout::eTransferDstOptimal, copyRegion);
}

void CopyImageToBuffer(
    vk::CommandBuffer cmdBuffer, vk::Image source, vk::Buffer destination, Format imageFormat, vk::Extent3D imageExtent,
    uint32 numMipLevels)
{
    const auto aspectMask = GetImageAspect(imageFormat);
    const auto pixelSize = GetFormatElementSize(imageFormat);

    // Copy each mip level with no gaps in between
    uint32 offset = 0;
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
            .imageOffset = {.x=0,.y=0,.z=0},
            .imageExtent = mipExtent,
        };
        cmdBuffer.copyImageToBuffer(source, vk::ImageLayout::eTransferSrcOptimal, destination, copyRegion);

        offset += mipExtent.width * mipExtent.height * mipExtent.depth * pixelSize;
        mipExtent.width = std::max(1u, mipExtent.width / 2);
        mipExtent.height = std::max(1u, mipExtent.height / 2);
        mipExtent.depth = std::max(1u, mipExtent.depth / 2);
    }
}

vk::UniqueRenderPass CreateRenderPass(vk::Device device, const FramebufferLayout& layout, FramebufferUsage usage)
{
    return CreateRenderPass(device, layout, usage, {});
}

vk::UniqueRenderPass
CreateRenderPass(vk::Device device, const FramebufferLayout& layout, FramebufferUsage usage, const RenderPassInfo& renderPassInfo)
{
    TEIDE_ASSERT(layout.colorFormat.has_value() || layout.depthStencilFormat.has_value());

    const bool multisampling = layout.sampleCount != 1;
    TEIDE_ASSERT(!(multisampling && layout.resolveDepthStencil), "Resolving depth/stencil targets not supported");

    const bool resolveColor = multisampling && layout.resolveColor;
    const bool loadColor = renderPassInfo.colorLoadOp == vk::AttachmentLoadOp::eLoad;

    std::vector<vk::AttachmentDescription> attachments;
    std::vector<vk::AttachmentReference> colorAttachmentRefs;
    std::vector<vk::AttachmentReference> resolveAttachmentRefs;
    std::optional<vk::AttachmentReference> depthStencilAttachmentRef;

    if (layout.colorFormat.has_value())
    {
        TEIDE_ASSERT(*layout.colorFormat != Format::Unknown);

        colorAttachmentRefs.push_back({
            .attachment = size32(attachments),
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
        });

        attachments.push_back({
            .format = ToVulkan(*layout.colorFormat),
            .samples = vk::SampleCountFlagBits{layout.sampleCount},
            .loadOp = renderPassInfo.colorLoadOp,
            .storeOp = multisampling ? vk::AttachmentStoreOp::eDontCare : renderPassInfo.colorStoreOp,
            .initialLayout = loadColor ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eUndefined,
            .finalLayout = resolveColor ? vk::ImageLayout::eColorAttachmentOptimal : GetColorImageLayout(usage),
        });
    }

    if (layout.depthStencilFormat.has_value())
    {
        TEIDE_ASSERT(*layout.depthStencilFormat != Format::Unknown);

        depthStencilAttachmentRef = vk::AttachmentReference{
            .attachment = size32(attachments),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };

        attachments.push_back({
            .format = ToVulkan(*layout.depthStencilFormat),
            .samples = vk::SampleCountFlagBits{layout.sampleCount},
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = multisampling ? vk::AttachmentStoreOp::eDontCare : renderPassInfo.depthStoreOp,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = multisampling ? vk::AttachmentStoreOp::eDontCare : renderPassInfo.stencilStoreOp,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = GetDepthStencilImageLayout(usage),
        });
    }

    if (resolveColor)
    {
        TEIDE_ASSERT(layout.colorFormat.has_value() && layout.captureColor);

        resolveAttachmentRefs.push_back({
            .attachment = size32(attachments),
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
        });

        attachments.push_back({
            .format = ToVulkan(*layout.colorFormat),
            .samples = vk::SampleCountFlagBits::e1,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = renderPassInfo.colorStoreOp,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = GetColorImageLayout(usage),
        });
    }

    const vk::SubpassDescription subpass = {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = size32(colorAttachmentRefs),
        .pColorAttachments = data(colorAttachmentRefs),
        .pResolveAttachments = data(resolveAttachmentRefs),
        .pDepthStencilAttachment = ToPointer(depthStencilAttachmentRef),
    };

    const vk::SubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlags{},
        .dstAccessMask = vk ::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    };

    const vk::RenderPassCreateInfo createInfo = {
        .attachmentCount = size32(attachments),
        .pAttachments = data(attachments),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    return device.createRenderPassUnique(createInfo, s_allocator);
}

vk::UniqueFramebuffer
CreateFramebuffer(vk::Device device, vk::RenderPass renderPass, Geo::Size2i size, std::span<const vk::ImageView> imageViews)
{
    const vk::FramebufferCreateInfo framebufferCreateInfo = {
        .renderPass = renderPass,
        .attachmentCount = size32(imageViews),
        .pAttachments = data(imageViews),
        .width = size.x,
        .height = size.y,
        .layers = 1,
    };

    return device.createFramebufferUnique(framebufferCreateInfo, s_allocator);
}

vk::Format ToVulkan(Format format)
{
    return VulkanFormats.at(format);
}

vk::Filter ToVulkan(Filter filter)
{
    static constexpr StaticMap<Filter, vk::Filter, 2> map{
        {Filter::Nearest, vk::Filter::eNearest},
        {Filter::Linear, vk::Filter::eLinear},
    };

    return map.at(filter);
}

vk::SamplerMipmapMode ToVulkan(MipmapMode mode)
{
    static constexpr StaticMap<MipmapMode, vk::SamplerMipmapMode, 2> map = {
        {MipmapMode::Nearest, vk::SamplerMipmapMode::eNearest},
        {MipmapMode::Linear, vk::SamplerMipmapMode::eLinear},
    };

    return map.at(mode);
}

vk::SamplerAddressMode ToVulkan(SamplerAddressMode mode)
{
    static constexpr StaticMap<SamplerAddressMode, vk::SamplerAddressMode, 4> map = {
        {SamplerAddressMode::Repeat, vk::SamplerAddressMode::eRepeat},
        {SamplerAddressMode::Mirror, vk::SamplerAddressMode::eMirroredRepeat},
        {SamplerAddressMode::Clamp, vk::SamplerAddressMode::eClampToEdge},
        {SamplerAddressMode::Border, vk::SamplerAddressMode::eClampToBorder},
    };

    return map.at(mode);
}

vk::CompareOp ToVulkan(CompareOp op)
{
    static constexpr StaticMap<CompareOp, vk::CompareOp, 8> map = {
        {CompareOp::Never, vk::CompareOp::eNever},       {CompareOp::Less, vk::CompareOp::eLess},
        {CompareOp::Equal, vk::CompareOp::eEqual},       {CompareOp::LessEqual, vk::CompareOp::eLessOrEqual},
        {CompareOp::Greater, vk::CompareOp::eGreater},   {CompareOp::GreaterEqual, vk::CompareOp::eGreaterOrEqual},
        {CompareOp::NotEqual, vk::CompareOp::eNotEqual}, {CompareOp::Always, vk::CompareOp::eAlways},
    };

    return map.at(op);
}

vk::Offset2D ToVulkan(Geo::Point2i point)
{
    return {.x = point.x, .y = point.y};
}

vk::Extent2D ToVulkan(Geo::Size2i vector)
{
    return {.width = vector.x, .height = vector.y};
}

vk::Rect2D ToVulkan(Geo::Box2i box)
{
    return {
        .offset = ToVulkan(box.min),
        .extent = ToVulkan(GetSize(box)),
    };
}

vk::BlendFactor ToVulkan(BlendFactor factor)
{
    static constexpr StaticMap<BlendFactor, vk::BlendFactor, 11> map = {
        {BlendFactor::Zero, vk::BlendFactor::eZero},
        {BlendFactor::One, vk::BlendFactor::eOne},
        {BlendFactor::SrcColor, vk::BlendFactor::eSrcColor},
        {BlendFactor::InvSrcColor, vk::BlendFactor::eOneMinusSrcColor},
        {BlendFactor::SrcAlpha, vk::BlendFactor::eSrcAlpha},
        {BlendFactor::InvSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha},
        {BlendFactor::DestAlpha, vk::BlendFactor::eDstAlpha},
        {BlendFactor::InvDestAlpha, vk::BlendFactor::eOneMinusDstAlpha},
        {BlendFactor::DestColor, vk::BlendFactor::eDstColor},
        {BlendFactor::InvDestColor, vk::BlendFactor::eOneMinusDstColor},
        {BlendFactor::SrcAlphaSaturate, vk::BlendFactor::eSrcAlphaSaturate},
    };

    return map.at(factor);
}

vk::BlendOp ToVulkan(BlendOp op)
{
    static constexpr StaticMap<BlendOp, vk::BlendOp, 5> map = {
        {BlendOp::Add, vk::BlendOp::eAdd},
        {BlendOp::Subtract, vk::BlendOp::eSubtract},
        {BlendOp::RevSubtract, vk::BlendOp::eReverseSubtract},
        {BlendOp::Min, vk::BlendOp::eMin},
        {BlendOp::Max, vk::BlendOp::eMax},
    };

    return map.at(op);
}

vk::StencilOp ToVulkan(StencilOp op)
{
    static constexpr StaticMap<StencilOp, vk::StencilOp, 8> map = {
        {StencilOp::Keep, vk::StencilOp::eKeep},
        {StencilOp::Zero, vk::StencilOp::eZero},
        {StencilOp::Replace, vk::StencilOp::eReplace},
        {StencilOp::Increment, vk::StencilOp::eIncrementAndWrap},
        {StencilOp::Decrement, vk::StencilOp::eDecrementAndWrap},
        {StencilOp::IncrementSaturate, vk::StencilOp::eIncrementAndClamp},
        {StencilOp::DecrementSaturate, vk::StencilOp::eDecrementAndClamp},
        {StencilOp::Invert, vk::StencilOp::eInvert},
    };

    return map.at(op);
}


vk::PolygonMode ToVulkan(FillMode mode)
{
    static constexpr StaticMap<FillMode, vk::PolygonMode, 3> map = {
        {FillMode::Solid, vk::PolygonMode::eFill},
        {FillMode::Wireframe, vk::PolygonMode::eLine},
        {FillMode::Point, vk::PolygonMode::ePoint},
    };

    return map.at(mode);
}

vk::CullModeFlags ToVulkan(CullMode mode)
{
    static constexpr StaticMap<CullMode, vk::CullModeFlags, 3> map = {
        {CullMode::None, vk::CullModeFlagBits::eNone},
        {CullMode::Anticlockwise, vk::CullModeFlagBits::eFront},
        {CullMode::Clockwise, vk::CullModeFlagBits::eBack},
    };

    return map.at(mode);
}

vk::ColorComponentFlags ToVulkan(ColorMask mask)
{
    vk::ColorComponentFlags ret = {};
    if (mask.red)
    {
        ret |= vk::ColorComponentFlagBits::eR;
    }
    if (mask.green)
    {
        ret |= vk::ColorComponentFlagBits::eG;
    }
    if (mask.blue)
    {
        ret |= vk::ColorComponentFlagBits::eB;
    }
    if (mask.alpha)
    {
        ret |= vk::ColorComponentFlagBits::eA;
    }
    return ret;
}

vk::PrimitiveTopology ToVulkan(PrimitiveTopology topology)
{
    static constexpr StaticMap<PrimitiveTopology, vk::PrimitiveTopology, 9> map = {
        {PrimitiveTopology::PointList, vk::PrimitiveTopology::ePointList},
        {PrimitiveTopology::LineList, vk::PrimitiveTopology::eLineList},
        {PrimitiveTopology::LineStrip, vk::PrimitiveTopology::eLineStrip},
        {PrimitiveTopology::TriangleList, vk::PrimitiveTopology::eTriangleList},
        {PrimitiveTopology::TriangleStrip, vk::PrimitiveTopology::eTriangleStrip},
        {PrimitiveTopology::LineListAdj, vk::PrimitiveTopology::eLineListWithAdjacency},
        {PrimitiveTopology::LineStripAdj, vk::PrimitiveTopology::eLineStripWithAdjacency},
        {PrimitiveTopology::TriangleListAdj, vk::PrimitiveTopology::eTriangleListWithAdjacency},
        {PrimitiveTopology::TriangleStripAdj, vk::PrimitiveTopology::eTriangleStripWithAdjacency},
    };

    return map.at(topology);
}


vk::VertexInputRate ToVulkan(VertexClass vc)
{
    static constexpr StaticMap<VertexClass, vk::VertexInputRate, 2> map = {
        {VertexClass::PerVertex, vk::VertexInputRate::eVertex},
        {VertexClass::PerInstance, vk::VertexInputRate::eInstance},
    };

    return map.at(vc);
}

Format FromVulkan(vk::Format format)
{
    return VulkanFormats.inverse_at(format);
}

} // namespace Teide
