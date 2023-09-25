#ifndef VKEX_HPP_
#define VKEX_HPP_

#include "vkex_utils.hpp"

/*
** This header is generated from the Khronos Vulkan XML API Registry.
**
*/


namespace vkex
{


#ifdef VK_VERSION_1_0
/*
struct PipelineCacheHeaderVersionOne
{
    using MappedType = vk::PipelineCacheHeaderVersionOne;

    uint32_t headerSize = {};
    vk::PipelineCacheHeaderVersion headerVersion = {};
    uint32_t vendorID = {};
    uint32_t deviceID = {};
    uint8_t pipelineCacheUUID[ VK_UUID_SIZE] = {};

    MappedType map() const
    {
        MappedType r;
        r.headerSize = headerSize;
        r.headerVersion = headerVersion;
        r.vendorID = vendorID;
        r.deviceID = deviceID;
        r.pipelineCacheUUID = pipelineCacheUUID;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct InstanceCreateInfo
{
    using MappedType = vk::InstanceCreateInfo;

    vk::InstanceCreateFlags flags = {};
    const vk::ApplicationInfo* pApplicationInfo = {};
    Array<const char*> pEnabledLayerNames = {};
    Array<const char*> pEnabledExtensionNames = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.pApplicationInfo = pApplicationInfo;
        r.enabledLayerCount = pEnabledLayerNames.size();
        r.ppEnabledLayerNames = pEnabledLayerNames.data();
        r.enabledExtensionCount = pEnabledExtensionNames.size();
        r.ppEnabledExtensionNames = pEnabledExtensionNames.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PhysicalDeviceLimits
{
    using MappedType = vk::PhysicalDeviceLimits;

    uint32_t maxImageDimension1D = {};
    uint32_t maxImageDimension2D = {};
    uint32_t maxImageDimension3D = {};
    uint32_t maxImageDimensionCube = {};
    uint32_t maxImageArrayLayers = {};
    uint32_t maxTexelBufferElements = {};
    uint32_t maxUniformBufferRange = {};
    uint32_t maxStorageBufferRange = {};
    uint32_t maxPushConstantsSize = {};
    uint32_t maxMemoryAllocationCount = {};
    uint32_t maxSamplerAllocationCount = {};
    vk::DeviceSize bufferImageGranularity = {};
    vk::DeviceSize sparseAddressSpaceSize = {};
    uint32_t maxBoundDescriptorSets = {};
    uint32_t maxPerStageDescriptorSamplers = {};
    uint32_t maxPerStageDescriptorUniformBuffers = {};
    uint32_t maxPerStageDescriptorStorageBuffers = {};
    uint32_t maxPerStageDescriptorSampledImages = {};
    uint32_t maxPerStageDescriptorStorageImages = {};
    uint32_t maxPerStageDescriptorInputAttachments = {};
    uint32_t maxPerStageResources = {};
    uint32_t maxDescriptorSetSamplers = {};
    uint32_t maxDescriptorSetUniformBuffers = {};
    uint32_t maxDescriptorSetUniformBuffersDynamic = {};
    uint32_t maxDescriptorSetStorageBuffers = {};
    uint32_t maxDescriptorSetStorageBuffersDynamic = {};
    uint32_t maxDescriptorSetSampledImages = {};
    uint32_t maxDescriptorSetStorageImages = {};
    uint32_t maxDescriptorSetInputAttachments = {};
    uint32_t maxVertexInputAttributes = {};
    uint32_t maxVertexInputBindings = {};
    uint32_t maxVertexInputAttributeOffset = {};
    uint32_t maxVertexInputBindingStride = {};
    uint32_t maxVertexOutputComponents = {};
    uint32_t maxTessellationGenerationLevel = {};
    uint32_t maxTessellationPatchSize = {};
    uint32_t maxTessellationControlPerVertexInputComponents = {};
    uint32_t maxTessellationControlPerVertexOutputComponents = {};
    uint32_t maxTessellationControlPerPatchOutputComponents = {};
    uint32_t maxTessellationControlTotalOutputComponents = {};
    uint32_t maxTessellationEvaluationInputComponents = {};
    uint32_t maxTessellationEvaluationOutputComponents = {};
    uint32_t maxGeometryShaderInvocations = {};
    uint32_t maxGeometryInputComponents = {};
    uint32_t maxGeometryOutputComponents = {};
    uint32_t maxGeometryOutputVertices = {};
    uint32_t maxGeometryTotalOutputComponents = {};
    uint32_t maxFragmentInputComponents = {};
    uint32_t maxFragmentOutputAttachments = {};
    uint32_t maxFragmentDualSrcAttachments = {};
    uint32_t maxFragmentCombinedOutputResources = {};
    uint32_t maxComputeSharedMemorySize = {};
    uint32_t maxComputeWorkGroupCount[3] = {};
    uint32_t maxComputeWorkGroupInvocations = {};
    uint32_t maxComputeWorkGroupSize[3] = {};
    uint32_t subPixelPrecisionBits = {};
    uint32_t subTexelPrecisionBits = {};
    uint32_t mipmapPrecisionBits = {};
    uint32_t maxDrawIndexedIndexValue = {};
    uint32_t maxDrawIndirectCount = {};
    float maxSamplerLodBias = {};
    float maxSamplerAnisotropy = {};
    uint32_t maxViewports = {};
    uint32_t maxViewportDimensions[2] = {};
    float viewportBoundsRange[2] = {};
    uint32_t viewportSubPixelBits = {};
    size_t minMemoryMapAlignment = {};
    vk::DeviceSize minTexelBufferOffsetAlignment = {};
    vk::DeviceSize minUniformBufferOffsetAlignment = {};
    vk::DeviceSize minStorageBufferOffsetAlignment = {};
    int32_t minTexelOffset = {};
    uint32_t maxTexelOffset = {};
    int32_t minTexelGatherOffset = {};
    uint32_t maxTexelGatherOffset = {};
    float minInterpolationOffset = {};
    float maxInterpolationOffset = {};
    uint32_t subPixelInterpolationOffsetBits = {};
    uint32_t maxFramebufferWidth = {};
    uint32_t maxFramebufferHeight = {};
    uint32_t maxFramebufferLayers = {};
    vk::SampleCountFlags framebufferColorSampleCounts = {};
    vk::SampleCountFlags framebufferDepthSampleCounts = {};
    vk::SampleCountFlags framebufferStencilSampleCounts = {};
    vk::SampleCountFlags framebufferNoAttachmentsSampleCounts = {};
    uint32_t maxColorAttachments = {};
    vk::SampleCountFlags sampledImageColorSampleCounts = {};
    vk::SampleCountFlags sampledImageIntegerSampleCounts = {};
    vk::SampleCountFlags sampledImageDepthSampleCounts = {};
    vk::SampleCountFlags sampledImageStencilSampleCounts = {};
    vk::SampleCountFlags storageImageSampleCounts = {};
    uint32_t maxSampleMaskWords = {};
    vk::Bool32 timestampComputeAndGraphics = {};
    float timestampPeriod = {};
    uint32_t maxClipDistances = {};
    uint32_t maxCullDistances = {};
    uint32_t maxCombinedClipAndCullDistances = {};
    uint32_t discreteQueuePriorities = {};
    float pointSizeRange[2] = {};
    float lineWidthRange[2] = {};
    float pointSizeGranularity = {};
    float lineWidthGranularity = {};
    vk::Bool32 strictLines = {};
    vk::Bool32 standardSampleLocations = {};
    vk::DeviceSize optimalBufferCopyOffsetAlignment = {};
    vk::DeviceSize optimalBufferCopyRowPitchAlignment = {};
    vk::DeviceSize nonCoherentAtomSize = {};

    MappedType map() const
    {
        MappedType r;
        r.maxImageDimension1D = maxImageDimension1D;
        r.maxImageDimension2D = maxImageDimension2D;
        r.maxImageDimension3D = maxImageDimension3D;
        r.maxImageDimensionCube = maxImageDimensionCube;
        r.maxImageArrayLayers = maxImageArrayLayers;
        r.maxTexelBufferElements = maxTexelBufferElements;
        r.maxUniformBufferRange = maxUniformBufferRange;
        r.maxStorageBufferRange = maxStorageBufferRange;
        r.maxPushConstantsSize = maxPushConstantsSize;
        r.maxMemoryAllocationCount = maxMemoryAllocationCount;
        r.maxSamplerAllocationCount = maxSamplerAllocationCount;
        r.bufferImageGranularity = bufferImageGranularity;
        r.sparseAddressSpaceSize = sparseAddressSpaceSize;
        r.maxBoundDescriptorSets = maxBoundDescriptorSets;
        r.maxPerStageDescriptorSamplers = maxPerStageDescriptorSamplers;
        r.maxPerStageDescriptorUniformBuffers = maxPerStageDescriptorUniformBuffers;
        r.maxPerStageDescriptorStorageBuffers = maxPerStageDescriptorStorageBuffers;
        r.maxPerStageDescriptorSampledImages = maxPerStageDescriptorSampledImages;
        r.maxPerStageDescriptorStorageImages = maxPerStageDescriptorStorageImages;
        r.maxPerStageDescriptorInputAttachments = maxPerStageDescriptorInputAttachments;
        r.maxPerStageResources = maxPerStageResources;
        r.maxDescriptorSetSamplers = maxDescriptorSetSamplers;
        r.maxDescriptorSetUniformBuffers = maxDescriptorSetUniformBuffers;
        r.maxDescriptorSetUniformBuffersDynamic = maxDescriptorSetUniformBuffersDynamic;
        r.maxDescriptorSetStorageBuffers = maxDescriptorSetStorageBuffers;
        r.maxDescriptorSetStorageBuffersDynamic = maxDescriptorSetStorageBuffersDynamic;
        r.maxDescriptorSetSampledImages = maxDescriptorSetSampledImages;
        r.maxDescriptorSetStorageImages = maxDescriptorSetStorageImages;
        r.maxDescriptorSetInputAttachments = maxDescriptorSetInputAttachments;
        r.maxVertexInputAttributes = maxVertexInputAttributes;
        r.maxVertexInputBindings = maxVertexInputBindings;
        r.maxVertexInputAttributeOffset = maxVertexInputAttributeOffset;
        r.maxVertexInputBindingStride = maxVertexInputBindingStride;
        r.maxVertexOutputComponents = maxVertexOutputComponents;
        r.maxTessellationGenerationLevel = maxTessellationGenerationLevel;
        r.maxTessellationPatchSize = maxTessellationPatchSize;
        r.maxTessellationControlPerVertexInputComponents = maxTessellationControlPerVertexInputComponents;
        r.maxTessellationControlPerVertexOutputComponents = maxTessellationControlPerVertexOutputComponents;
        r.maxTessellationControlPerPatchOutputComponents = maxTessellationControlPerPatchOutputComponents;
        r.maxTessellationControlTotalOutputComponents = maxTessellationControlTotalOutputComponents;
        r.maxTessellationEvaluationInputComponents = maxTessellationEvaluationInputComponents;
        r.maxTessellationEvaluationOutputComponents = maxTessellationEvaluationOutputComponents;
        r.maxGeometryShaderInvocations = maxGeometryShaderInvocations;
        r.maxGeometryInputComponents = maxGeometryInputComponents;
        r.maxGeometryOutputComponents = maxGeometryOutputComponents;
        r.maxGeometryOutputVertices = maxGeometryOutputVertices;
        r.maxGeometryTotalOutputComponents = maxGeometryTotalOutputComponents;
        r.maxFragmentInputComponents = maxFragmentInputComponents;
        r.maxFragmentOutputAttachments = maxFragmentOutputAttachments;
        r.maxFragmentDualSrcAttachments = maxFragmentDualSrcAttachments;
        r.maxFragmentCombinedOutputResources = maxFragmentCombinedOutputResources;
        r.maxComputeSharedMemorySize = maxComputeSharedMemorySize;
        r.maxComputeWorkGroupCount = maxComputeWorkGroupCount;
        r.maxComputeWorkGroupInvocations = maxComputeWorkGroupInvocations;
        r.maxComputeWorkGroupSize = maxComputeWorkGroupSize;
        r.subPixelPrecisionBits = subPixelPrecisionBits;
        r.subTexelPrecisionBits = subTexelPrecisionBits;
        r.mipmapPrecisionBits = mipmapPrecisionBits;
        r.maxDrawIndexedIndexValue = maxDrawIndexedIndexValue;
        r.maxDrawIndirectCount = maxDrawIndirectCount;
        r.maxSamplerLodBias = maxSamplerLodBias;
        r.maxSamplerAnisotropy = maxSamplerAnisotropy;
        r.maxViewports = maxViewports;
        r.maxViewportDimensions = maxViewportDimensions;
        r.viewportBoundsRange = viewportBoundsRange;
        r.viewportSubPixelBits = viewportSubPixelBits;
        r.minMemoryMapAlignment = minMemoryMapAlignment;
        r.minTexelBufferOffsetAlignment = minTexelBufferOffsetAlignment;
        r.minUniformBufferOffsetAlignment = minUniformBufferOffsetAlignment;
        r.minStorageBufferOffsetAlignment = minStorageBufferOffsetAlignment;
        r.minTexelOffset = minTexelOffset;
        r.maxTexelOffset = maxTexelOffset;
        r.minTexelGatherOffset = minTexelGatherOffset;
        r.maxTexelGatherOffset = maxTexelGatherOffset;
        r.minInterpolationOffset = minInterpolationOffset;
        r.maxInterpolationOffset = maxInterpolationOffset;
        r.subPixelInterpolationOffsetBits = subPixelInterpolationOffsetBits;
        r.maxFramebufferWidth = maxFramebufferWidth;
        r.maxFramebufferHeight = maxFramebufferHeight;
        r.maxFramebufferLayers = maxFramebufferLayers;
        r.framebufferColorSampleCounts = framebufferColorSampleCounts;
        r.framebufferDepthSampleCounts = framebufferDepthSampleCounts;
        r.framebufferStencilSampleCounts = framebufferStencilSampleCounts;
        r.framebufferNoAttachmentsSampleCounts = framebufferNoAttachmentsSampleCounts;
        r.maxColorAttachments = maxColorAttachments;
        r.sampledImageColorSampleCounts = sampledImageColorSampleCounts;
        r.sampledImageIntegerSampleCounts = sampledImageIntegerSampleCounts;
        r.sampledImageDepthSampleCounts = sampledImageDepthSampleCounts;
        r.sampledImageStencilSampleCounts = sampledImageStencilSampleCounts;
        r.storageImageSampleCounts = storageImageSampleCounts;
        r.maxSampleMaskWords = maxSampleMaskWords;
        r.timestampComputeAndGraphics = timestampComputeAndGraphics;
        r.timestampPeriod = timestampPeriod;
        r.maxClipDistances = maxClipDistances;
        r.maxCullDistances = maxCullDistances;
        r.maxCombinedClipAndCullDistances = maxCombinedClipAndCullDistances;
        r.discreteQueuePriorities = discreteQueuePriorities;
        r.pointSizeRange = pointSizeRange;
        r.lineWidthRange = lineWidthRange;
        r.pointSizeGranularity = pointSizeGranularity;
        r.lineWidthGranularity = lineWidthGranularity;
        r.strictLines = strictLines;
        r.standardSampleLocations = standardSampleLocations;
        r.optimalBufferCopyOffsetAlignment = optimalBufferCopyOffsetAlignment;
        r.optimalBufferCopyRowPitchAlignment = optimalBufferCopyRowPitchAlignment;
        r.nonCoherentAtomSize = nonCoherentAtomSize;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct PhysicalDeviceMemoryProperties
{
    using MappedType = vk::PhysicalDeviceMemoryProperties;

    uint32_t memoryTypeCount = {};
    vk::MemoryType memoryTypes[ VK_MAX_MEMORY_TYPES] = {};
    uint32_t memoryHeapCount = {};
    vk::MemoryHeap memoryHeaps[ VK_MAX_MEMORY_HEAPS] = {};

    MappedType map() const
    {
        MappedType r;
        r.memoryTypeCount = memoryTypeCount;
        r.memoryTypes = memoryTypes;
        r.memoryHeapCount = memoryHeapCount;
        r.memoryHeaps = memoryHeaps;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct PhysicalDeviceProperties
{
    using MappedType = vk::PhysicalDeviceProperties;

    uint32_t apiVersion = {};
    uint32_t driverVersion = {};
    uint32_t vendorID = {};
    uint32_t deviceID = {};
    vk::PhysicalDeviceType deviceType = {};
    char deviceName[ VK_MAX_PHYSICAL_DEVICE_NAME_SIZE] = {};
    uint8_t pipelineCacheUUID[ VK_UUID_SIZE] = {};
    vk::PhysicalDeviceLimits limits = {};
    vk::PhysicalDeviceSparseProperties sparseProperties = {};

    MappedType map() const
    {
        MappedType r;
        r.apiVersion = apiVersion;
        r.driverVersion = driverVersion;
        r.vendorID = vendorID;
        r.deviceID = deviceID;
        r.deviceType = deviceType;
        r.deviceName = deviceName;
        r.pipelineCacheUUID = pipelineCacheUUID;
        r.limits = limits;
        r.sparseProperties = sparseProperties;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct DeviceQueueCreateInfo
{
    using MappedType = vk::DeviceQueueCreateInfo;

    vk::DeviceQueueCreateFlags flags = {};
    uint32_t queueFamilyIndex = {};
    Array<float> queuePriorities = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.queueFamilyIndex = queueFamilyIndex;
        r.queueCount = queuePriorities.size();
        r.pQueuePriorities = queuePriorities.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct DeviceCreateInfo
{
    using MappedType = vk::DeviceCreateInfo;

    vk::DeviceCreateFlags flags = {};
    Array<vk::DeviceQueueCreateInfo> queueCreateInfos = {};
    Array<const char*> pEnabledLayerNames = {};
    Array<const char*> pEnabledExtensionNames = {};
    const vk::PhysicalDeviceFeatures* pEnabledFeatures = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.queueCreateInfoCount = queueCreateInfos.size();
        r.pQueueCreateInfos = queueCreateInfos.data();
        r.enabledLayerCount = pEnabledLayerNames.size();
        r.ppEnabledLayerNames = pEnabledLayerNames.data();
        r.enabledExtensionCount = pEnabledExtensionNames.size();
        r.ppEnabledExtensionNames = pEnabledExtensionNames.data();
        r.pEnabledFeatures = pEnabledFeatures;
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct ExtensionProperties
{
    using MappedType = vk::ExtensionProperties;

    char extensionName[ VK_MAX_EXTENSION_NAME_SIZE] = {};
    uint32_t specVersion = {};

    MappedType map() const
    {
        MappedType r;
        r.extensionName = extensionName;
        r.specVersion = specVersion;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct LayerProperties
{
    using MappedType = vk::LayerProperties;

    char layerName[ VK_MAX_EXTENSION_NAME_SIZE] = {};
    uint32_t specVersion = {};
    uint32_t implementationVersion = {};
    char description[ VK_MAX_DESCRIPTION_SIZE] = {};

    MappedType map() const
    {
        MappedType r;
        r.layerName = layerName;
        r.specVersion = specVersion;
        r.implementationVersion = implementationVersion;
        r.description = description;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct SubmitInfo
{
    using MappedType = vk::SubmitInfo;

    Array<vk::Semaphore> waitSemaphores = {};
    Array<vk::PipelineStageFlags> waitDstStageMask = {};
    Array<vk::CommandBuffer> commandBuffers = {};
    Array<vk::Semaphore> signalSemaphores = {};

    MappedType map() const
    {
        VKEX_ASSERT(waitDstStageMask.size() == waitSemaphores.size());
        MappedType r;
        r.waitSemaphoreCount = waitSemaphores.size();
        r.pWaitSemaphores = waitSemaphores.data();
        r.pWaitDstStageMask = waitDstStageMask.data();
        r.commandBufferCount = commandBuffers.size();
        r.pCommandBuffers = commandBuffers.data();
        r.signalSemaphoreCount = signalSemaphores.size();
        r.pSignalSemaphores = signalSemaphores.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SparseBufferMemoryBindInfo
{
    using MappedType = vk::SparseBufferMemoryBindInfo;

    vk::Buffer buffer = {};
    Array<vk::SparseMemoryBind> binds = {};

    MappedType map() const
    {
        MappedType r;
        r.buffer = buffer;
        r.bindCount = binds.size();
        r.pBinds = binds.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SparseImageOpaqueMemoryBindInfo
{
    using MappedType = vk::SparseImageOpaqueMemoryBindInfo;

    vk::Image image = {};
    Array<vk::SparseMemoryBind> binds = {};

    MappedType map() const
    {
        MappedType r;
        r.image = image;
        r.bindCount = binds.size();
        r.pBinds = binds.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SparseImageMemoryBindInfo
{
    using MappedType = vk::SparseImageMemoryBindInfo;

    vk::Image image = {};
    Array<vk::SparseImageMemoryBind> binds = {};

    MappedType map() const
    {
        MappedType r;
        r.image = image;
        r.bindCount = binds.size();
        r.pBinds = binds.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct BindSparseInfo
{
    using MappedType = vk::BindSparseInfo;

    Array<vk::Semaphore> waitSemaphores = {};
    Array<vk::SparseBufferMemoryBindInfo> bufferBinds = {};
    Array<vk::SparseImageOpaqueMemoryBindInfo> imageOpaqueBinds = {};
    Array<vk::SparseImageMemoryBindInfo> imageBinds = {};
    Array<vk::Semaphore> signalSemaphores = {};

    MappedType map() const
    {
        MappedType r;
        r.waitSemaphoreCount = waitSemaphores.size();
        r.pWaitSemaphores = waitSemaphores.data();
        r.bufferBindCount = bufferBinds.size();
        r.pBufferBinds = bufferBinds.data();
        r.imageOpaqueBindCount = imageOpaqueBinds.size();
        r.pImageOpaqueBinds = imageOpaqueBinds.data();
        r.imageBindCount = imageBinds.size();
        r.pImageBinds = imageBinds.data();
        r.signalSemaphoreCount = signalSemaphores.size();
        r.pSignalSemaphores = signalSemaphores.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct BufferCreateInfo
{
    using MappedType = vk::BufferCreateInfo;

    vk::BufferCreateFlags flags = {};
    vk::DeviceSize size = {};
    vk::BufferUsageFlags usage = {};
    vk::SharingMode sharingMode = {};
    Array<uint32_t> queueFamilyIndices = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.size = size;
        r.usage = usage;
        r.sharingMode = sharingMode;
        r.queueFamilyIndexCount = queueFamilyIndices.size();
        r.pQueueFamilyIndices = queueFamilyIndices.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct ImageCreateInfo
{
    using MappedType = vk::ImageCreateInfo;

    vk::ImageCreateFlags flags = {};
    vk::ImageType imageType = {};
    vk::Format format = {};
    vk::Extent3D extent = {};
    uint32_t mipLevels = {};
    uint32_t arrayLayers = {};
    vk::SampleCountFlagBits samples = {};
    vk::ImageTiling tiling = {};
    vk::ImageUsageFlags usage = {};
    vk::SharingMode sharingMode = {};
    Array<uint32_t> queueFamilyIndices = {};
    vk::ImageLayout initialLayout = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.imageType = imageType;
        r.format = format;
        r.extent = extent;
        r.mipLevels = mipLevels;
        r.arrayLayers = arrayLayers;
        r.samples = samples;
        r.tiling = tiling;
        r.usage = usage;
        r.sharingMode = sharingMode;
        r.queueFamilyIndexCount = queueFamilyIndices.size();
        r.pQueueFamilyIndices = queueFamilyIndices.data();
        r.initialLayout = initialLayout;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct ShaderModuleCreateInfo
{
    using MappedType = vk::ShaderModuleCreateInfo;

    vk::ShaderModuleCreateFlags flags = {};
    size_t codeSize = {};
    const uint32_t* pCode = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.codeSize = codeSize;
        r.pCode = pCode;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct PipelineCacheCreateInfo
{
    using MappedType = vk::PipelineCacheCreateInfo;

    vk::PipelineCacheCreateFlags flags = {};
    Array<std::byte> initialData = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.initialDataSize = initialData.size();
        r.pInitialData = initialData.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SpecializationInfo
{
    using MappedType = vk::SpecializationInfo;

    Array<vk::SpecializationMapEntry> mapEntries = {};
    Array<std::byte> data = {};

    MappedType map() const
    {
        MappedType r;
        r.mapEntryCount = mapEntries.size();
        r.pMapEntries = mapEntries.data();
        r.dataSize = data.size();
        r.pData = data.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct PipelineVertexInputStateCreateInfo
{
    using MappedType = vk::PipelineVertexInputStateCreateInfo;

    vk::PipelineVertexInputStateCreateFlags flags = {};
    Array<vk::VertexInputBindingDescription> vertexBindingDescriptions = {};
    Array<vk::VertexInputAttributeDescription> vertexAttributeDescriptions = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
        r.pVertexBindingDescriptions = vertexBindingDescriptions.data();
        r.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
        r.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PipelineViewportStateCreateInfo
{
    using MappedType = vk::PipelineViewportStateCreateInfo;

    vk::PipelineViewportStateCreateFlags flags = {};
    Array<vk::Viewport> viewports = {};
    Array<vk::Rect2D> scissors = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.viewportCount = viewports.size();
        r.pViewports = viewports.data();
        r.scissorCount = scissors.size();
        r.pScissors = scissors.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct PipelineMultisampleStateCreateInfo
{
    using MappedType = vk::PipelineMultisampleStateCreateInfo;

    vk::PipelineMultisampleStateCreateFlags flags = {};
    vk::SampleCountFlagBits rasterizationSamples = {};
    vk::Bool32 sampleShadingEnable = {};
    float minSampleShading = {};
    const vk::SampleMask* pSampleMask = {};
    vk::Bool32 alphaToCoverageEnable = {};
    vk::Bool32 alphaToOneEnable = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.rasterizationSamples = rasterizationSamples;
        r.sampleShadingEnable = sampleShadingEnable;
        r.minSampleShading = minSampleShading;
        r.pSampleMask = pSampleMask;
        r.alphaToCoverageEnable = alphaToCoverageEnable;
        r.alphaToOneEnable = alphaToOneEnable;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct PipelineColorBlendStateCreateInfo
{
    using MappedType = vk::PipelineColorBlendStateCreateInfo;

    vk::PipelineColorBlendStateCreateFlags flags = {};
    vk::Bool32 logicOpEnable = {};
    vk::LogicOp logicOp = {};
    Array<vk::PipelineColorBlendAttachmentState> attachments = {};
    float blendConstants[4] = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.logicOpEnable = logicOpEnable;
        r.logicOp = logicOp;
        r.attachmentCount = attachments.size();
        r.pAttachments = attachments.data();
        r.blendConstants = blendConstants;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct PipelineDynamicStateCreateInfo
{
    using MappedType = vk::PipelineDynamicStateCreateInfo;

    vk::PipelineDynamicStateCreateFlags flags = {};
    Array<vk::DynamicState> dynamicStates = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.dynamicStateCount = dynamicStates.size();
        r.pDynamicStates = dynamicStates.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct GraphicsPipelineCreateInfo
{
    using MappedType = vk::GraphicsPipelineCreateInfo;

    vk::PipelineCreateFlags flags = {};
    Array<vk::PipelineShaderStageCreateInfo> stages = {};
    const vk::PipelineVertexInputStateCreateInfo* pVertexInputState = {};
    const vk::PipelineInputAssemblyStateCreateInfo* pInputAssemblyState = {};
    const vk::PipelineTessellationStateCreateInfo* pTessellationState = {};
    const vk::PipelineViewportStateCreateInfo* pViewportState = {};
    const vk::PipelineRasterizationStateCreateInfo* pRasterizationState = {};
    const vk::PipelineMultisampleStateCreateInfo* pMultisampleState = {};
    const vk::PipelineDepthStencilStateCreateInfo* pDepthStencilState = {};
    const vk::PipelineColorBlendStateCreateInfo* pColorBlendState = {};
    const vk::PipelineDynamicStateCreateInfo* pDynamicState = {};
    vk::PipelineLayout layout = {};
    vk::RenderPass renderPass = {};
    uint32_t subpass = {};
    vk::Pipeline basePipelineHandle = {};
    int32_t basePipelineIndex = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.stageCount = stages.size();
        r.pStages = stages.data();
        r.pVertexInputState = pVertexInputState;
        r.pInputAssemblyState = pInputAssemblyState;
        r.pTessellationState = pTessellationState;
        r.pViewportState = pViewportState;
        r.pRasterizationState = pRasterizationState;
        r.pMultisampleState = pMultisampleState;
        r.pDepthStencilState = pDepthStencilState;
        r.pColorBlendState = pColorBlendState;
        r.pDynamicState = pDynamicState;
        r.layout = layout;
        r.renderPass = renderPass;
        r.subpass = subpass;
        r.basePipelineHandle = basePipelineHandle;
        r.basePipelineIndex = basePipelineIndex;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct PipelineLayoutCreateInfo
{
    using MappedType = vk::PipelineLayoutCreateInfo;

    vk::PipelineLayoutCreateFlags flags = {};
    Array<vk::DescriptorSetLayout> setLayouts = {};
    Array<vk::PushConstantRange> pushConstantRanges = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.setLayoutCount = setLayouts.size();
        r.pSetLayouts = setLayouts.data();
        r.pushConstantRangeCount = pushConstantRanges.size();
        r.pPushConstantRanges = pushConstantRanges.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct DescriptorPoolCreateInfo
{
    using MappedType = vk::DescriptorPoolCreateInfo;

    vk::DescriptorPoolCreateFlags flags = {};
    uint32_t maxSets = {};
    Array<vk::DescriptorPoolSize> poolSizes = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.maxSets = maxSets;
        r.poolSizeCount = poolSizes.size();
        r.pPoolSizes = poolSizes.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct DescriptorSetAllocateInfo
{
    using MappedType = vk::DescriptorSetAllocateInfo;

    vk::DescriptorPool descriptorPool = {};
    Array<vk::DescriptorSetLayout> setLayouts = {};

    MappedType map() const
    {
        MappedType r;
        r.descriptorPool = descriptorPool;
        r.descriptorSetCount = setLayouts.size();
        r.pSetLayouts = setLayouts.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct DescriptorSetLayoutBinding
{
    using MappedType = vk::DescriptorSetLayoutBinding;

    uint32_t binding = {};
    vk::DescriptorType descriptorType = {};
    vk::ShaderStageFlags stageFlags = {};
    Array<vk::Sampler> immutableSamplers = {};

    MappedType map() const
    {
        MappedType r;
        r.binding = binding;
        r.descriptorType = descriptorType;
        r.descriptorCount = immutableSamplers.size();
        r.stageFlags = stageFlags;
        r.pImmutableSamplers = immutableSamplers.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct DescriptorSetLayoutCreateInfo
{
    using MappedType = vk::DescriptorSetLayoutCreateInfo;

    vk::DescriptorSetLayoutCreateFlags flags = {};
    Array<vk::DescriptorSetLayoutBinding> bindings = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.bindingCount = bindings.size();
        r.pBindings = bindings.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct WriteDescriptorSet
{
    using MappedType = vk::WriteDescriptorSet;

    vk::DescriptorSet dstSet = {};
    uint32_t dstBinding = {};
    uint32_t dstArrayElement = {};
    vk::DescriptorType descriptorType = {};
    Array<vk::DescriptorImageInfo> imageInfo = {};
    Array<vk::DescriptorBufferInfo> bufferInfo = {};
    Array<vk::BufferView> texelBufferView = {};

    MappedType map() const
    {
        VKEX_ASSERT(bufferInfo.size() == imageInfo.size());
        VKEX_ASSERT(texelBufferView.size() == imageInfo.size());
        MappedType r;
        r.dstSet = dstSet;
        r.dstBinding = dstBinding;
        r.dstArrayElement = dstArrayElement;
        r.descriptorCount = imageInfo.size();
        r.descriptorType = descriptorType;
        r.pImageInfo = imageInfo.data();
        r.pBufferInfo = bufferInfo.data();
        r.pTexelBufferView = texelBufferView.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct FramebufferCreateInfo
{
    using MappedType = vk::FramebufferCreateInfo;

    vk::FramebufferCreateFlags flags = {};
    vk::RenderPass renderPass = {};
    Array<vk::ImageView> attachments = {};
    uint32_t width = {};
    uint32_t height = {};
    uint32_t layers = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.renderPass = renderPass;
        r.attachmentCount = attachments.size();
        r.pAttachments = attachments.data();
        r.width = width;
        r.height = height;
        r.layers = layers;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct SubpassDescription
{
    using MappedType = vk::SubpassDescription;

    vk::SubpassDescriptionFlags flags = {};
    vk::PipelineBindPoint pipelineBindPoint = {};
    Array<vk::AttachmentReference> inputAttachments = {};
    Array<vk::AttachmentReference> colorAttachments = {};
    Array<vk::AttachmentReference> resolveAttachments = {};
    const vk::AttachmentReference* pDepthStencilAttachment = {};
    Array<uint32_t> preserveAttachments = {};

    MappedType map() const
    {
        VKEX_ASSERT(resolveAttachments.size() == colorAttachments.size());
        MappedType r;
        r.flags = flags;
        r.pipelineBindPoint = pipelineBindPoint;
        r.inputAttachmentCount = inputAttachments.size();
        r.pInputAttachments = inputAttachments.data();
        r.colorAttachmentCount = colorAttachments.size();
        r.pColorAttachments = colorAttachments.data();
        r.pResolveAttachments = resolveAttachments.data();
        r.pDepthStencilAttachment = pDepthStencilAttachment;
        r.preserveAttachmentCount = preserveAttachments.size();
        r.pPreserveAttachments = preserveAttachments.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct RenderPassCreateInfo
{
    using MappedType = vk::RenderPassCreateInfo;

    vk::RenderPassCreateFlags flags = {};
    Array<vk::AttachmentDescription> attachments = {};
    Array<vk::SubpassDescription> subpasses = {};
    Array<vk::SubpassDependency> dependencies = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.attachmentCount = attachments.size();
        r.pAttachments = attachments.data();
        r.subpassCount = subpasses.size();
        r.pSubpasses = subpasses.data();
        r.dependencyCount = dependencies.size();
        r.pDependencies = dependencies.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
union ClearColorValue
{
    using MappedType = vk::ClearColorValue;

    float float32[4] = {};
    int32_t int32[4] = {};
    uint32_t uint32[4] = {};

    MappedType map() const
    {
        MappedType r;
        r.float32 = float32;
        r.int32 = int32;
        r.uint32 = uint32;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct ImageBlit
{
    using MappedType = vk::ImageBlit;

    vk::ImageSubresourceLayers srcSubresource = {};
    vk::Offset3D srcOffsets[2] = {};
    vk::ImageSubresourceLayers dstSubresource = {};
    vk::Offset3D dstOffsets[2] = {};

    MappedType map() const
    {
        MappedType r;
        r.srcSubresource = srcSubresource;
        r.srcOffsets = srcOffsets;
        r.dstSubresource = dstSubresource;
        r.dstOffsets = dstOffsets;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct RenderPassBeginInfo
{
    using MappedType = vk::RenderPassBeginInfo;

    vk::RenderPass renderPass = {};
    vk::Framebuffer framebuffer = {};
    vk::Rect2D renderArea = {};
    Array<vk::ClearValue> clearValues = {};

    MappedType map() const
    {
        MappedType r;
        r.renderPass = renderPass;
        r.framebuffer = framebuffer;
        r.renderArea = renderArea;
        r.clearValueCount = clearValues.size();
        r.pClearValues = clearValues.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_VERSION_1_1
struct DeviceGroupRenderPassBeginInfo
{
    using MappedType = vk::DeviceGroupRenderPassBeginInfo;

    uint32_t deviceMask = {};
    Array<vk::Rect2D> deviceRenderAreas = {};

    MappedType map() const
    {
        MappedType r;
        r.deviceMask = deviceMask;
        r.deviceRenderAreaCount = deviceRenderAreas.size();
        r.pDeviceRenderAreas = deviceRenderAreas.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct DeviceGroupSubmitInfo
{
    using MappedType = vk::DeviceGroupSubmitInfo;

    Array<uint32_t> waitSemaphoreDeviceIndices = {};
    Array<uint32_t> commandBufferDeviceMasks = {};
    Array<uint32_t> signalSemaphoreDeviceIndices = {};

    MappedType map() const
    {
        MappedType r;
        r.waitSemaphoreCount = waitSemaphoreDeviceIndices.size();
        r.pWaitSemaphoreDeviceIndices = waitSemaphoreDeviceIndices.data();
        r.commandBufferCount = commandBufferDeviceMasks.size();
        r.pCommandBufferDeviceMasks = commandBufferDeviceMasks.data();
        r.signalSemaphoreCount = signalSemaphoreDeviceIndices.size();
        r.pSignalSemaphoreDeviceIndices = signalSemaphoreDeviceIndices.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct BindBufferMemoryDeviceGroupInfo
{
    using MappedType = vk::BindBufferMemoryDeviceGroupInfo;

    Array<uint32_t> deviceIndices = {};

    MappedType map() const
    {
        MappedType r;
        r.deviceIndexCount = deviceIndices.size();
        r.pDeviceIndices = deviceIndices.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct BindImageMemoryDeviceGroupInfo
{
    using MappedType = vk::BindImageMemoryDeviceGroupInfo;

    Array<uint32_t> deviceIndices = {};
    Array<vk::Rect2D> splitInstanceBindRegions = {};

    MappedType map() const
    {
        MappedType r;
        r.deviceIndexCount = deviceIndices.size();
        r.pDeviceIndices = deviceIndices.data();
        r.splitInstanceBindRegionCount = splitInstanceBindRegions.size();
        r.pSplitInstanceBindRegions = splitInstanceBindRegions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PhysicalDeviceGroupProperties
{
    using MappedType = vk::PhysicalDeviceGroupProperties;

    uint32_t physicalDeviceCount = {};
    vk::PhysicalDevice physicalDevices[ VK_MAX_DEVICE_GROUP_SIZE] = {};
    vk::Bool32 subsetAllocation = {};

    MappedType map() const
    {
        MappedType r;
        r.physicalDeviceCount = physicalDeviceCount;
        r.physicalDevices = physicalDevices;
        r.subsetAllocation = subsetAllocation;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct DeviceGroupDeviceCreateInfo
{
    using MappedType = vk::DeviceGroupDeviceCreateInfo;

    Array<vk::PhysicalDevice> physicalDevices = {};

    MappedType map() const
    {
        MappedType r;
        r.physicalDeviceCount = physicalDevices.size();
        r.pPhysicalDevices = physicalDevices.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct RenderPassInputAttachmentAspectCreateInfo
{
    using MappedType = vk::RenderPassInputAttachmentAspectCreateInfo;

    Array<vk::InputAttachmentAspectReference> aspectReferences = {};

    MappedType map() const
    {
        MappedType r;
        r.aspectReferenceCount = aspectReferences.size();
        r.pAspectReferences = aspectReferences.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct RenderPassMultiviewCreateInfo
{
    using MappedType = vk::RenderPassMultiviewCreateInfo;

    Array<uint32_t> viewMasks = {};
    Array<int32_t> viewOffsets = {};
    Array<uint32_t> correlationMasks = {};

    MappedType map() const
    {
        MappedType r;
        r.subpassCount = viewMasks.size();
        r.pViewMasks = viewMasks.data();
        r.dependencyCount = viewOffsets.size();
        r.pViewOffsets = viewOffsets.data();
        r.correlationMaskCount = correlationMasks.size();
        r.pCorrelationMasks = correlationMasks.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct DescriptorUpdateTemplateCreateInfo
{
    using MappedType = vk::DescriptorUpdateTemplateCreateInfo;

    vk::DescriptorUpdateTemplateCreateFlags flags = {};
    Array<vk::DescriptorUpdateTemplateEntry> descriptorUpdateEntries = {};
    vk::DescriptorUpdateTemplateType templateType = {};
    vk::DescriptorSetLayout descriptorSetLayout = {};
    vk::PipelineBindPoint pipelineBindPoint = {};
    vk::PipelineLayout pipelineLayout = {};
    uint32_t set = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.descriptorUpdateEntryCount = descriptorUpdateEntries.size();
        r.pDescriptorUpdateEntries = descriptorUpdateEntries.data();
        r.templateType = templateType;
        r.descriptorSetLayout = descriptorSetLayout;
        r.pipelineBindPoint = pipelineBindPoint;
        r.pipelineLayout = pipelineLayout;
        r.set = set;
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PhysicalDeviceIDProperties
{
    using MappedType = vk::PhysicalDeviceIDProperties;

    uint8_t deviceUUID[ VK_UUID_SIZE] = {};
    uint8_t driverUUID[ VK_UUID_SIZE] = {};
    uint8_t deviceLUID[ VK_LUID_SIZE] = {};
    uint32_t deviceNodeMask = {};
    vk::Bool32 deviceLUIDValid = {};

    MappedType map() const
    {
        MappedType r;
        r.deviceUUID = deviceUUID;
        r.driverUUID = driverUUID;
        r.deviceLUID = deviceLUID;
        r.deviceNodeMask = deviceNodeMask;
        r.deviceLUIDValid = deviceLUIDValid;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_VERSION_1_2
/*
struct PhysicalDeviceVulkan11Properties
{
    using MappedType = vk::PhysicalDeviceVulkan11Properties;

    uint8_t deviceUUID[ VK_UUID_SIZE] = {};
    uint8_t driverUUID[ VK_UUID_SIZE] = {};
    uint8_t deviceLUID[ VK_LUID_SIZE] = {};
    uint32_t deviceNodeMask = {};
    vk::Bool32 deviceLUIDValid = {};
    uint32_t subgroupSize = {};
    vk::ShaderStageFlags subgroupSupportedStages = {};
    vk::SubgroupFeatureFlags subgroupSupportedOperations = {};
    vk::Bool32 subgroupQuadOperationsInAllStages = {};
    vk::PointClippingBehavior pointClippingBehavior = {};
    uint32_t maxMultiviewViewCount = {};
    uint32_t maxMultiviewInstanceIndex = {};
    vk::Bool32 protectedNoFault = {};
    uint32_t maxPerSetDescriptors = {};
    vk::DeviceSize maxMemoryAllocationSize = {};

    MappedType map() const
    {
        MappedType r;
        r.deviceUUID = deviceUUID;
        r.driverUUID = driverUUID;
        r.deviceLUID = deviceLUID;
        r.deviceNodeMask = deviceNodeMask;
        r.deviceLUIDValid = deviceLUIDValid;
        r.subgroupSize = subgroupSize;
        r.subgroupSupportedStages = subgroupSupportedStages;
        r.subgroupSupportedOperations = subgroupSupportedOperations;
        r.subgroupQuadOperationsInAllStages = subgroupQuadOperationsInAllStages;
        r.pointClippingBehavior = pointClippingBehavior;
        r.maxMultiviewViewCount = maxMultiviewViewCount;
        r.maxMultiviewInstanceIndex = maxMultiviewInstanceIndex;
        r.protectedNoFault = protectedNoFault;
        r.maxPerSetDescriptors = maxPerSetDescriptors;
        r.maxMemoryAllocationSize = maxMemoryAllocationSize;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct PhysicalDeviceVulkan12Properties
{
    using MappedType = vk::PhysicalDeviceVulkan12Properties;

    vk::DriverId driverID = {};
    char driverName[ VK_MAX_DRIVER_NAME_SIZE] = {};
    char driverInfo[ VK_MAX_DRIVER_INFO_SIZE] = {};
    vk::ConformanceVersion conformanceVersion = {};
    vk::ShaderFloatControlsIndependence denormBehaviorIndependence = {};
    vk::ShaderFloatControlsIndependence roundingModeIndependence = {};
    vk::Bool32 shaderSignedZeroInfNanPreserveFloat16 = {};
    vk::Bool32 shaderSignedZeroInfNanPreserveFloat32 = {};
    vk::Bool32 shaderSignedZeroInfNanPreserveFloat64 = {};
    vk::Bool32 shaderDenormPreserveFloat16 = {};
    vk::Bool32 shaderDenormPreserveFloat32 = {};
    vk::Bool32 shaderDenormPreserveFloat64 = {};
    vk::Bool32 shaderDenormFlushToZeroFloat16 = {};
    vk::Bool32 shaderDenormFlushToZeroFloat32 = {};
    vk::Bool32 shaderDenormFlushToZeroFloat64 = {};
    vk::Bool32 shaderRoundingModeRTEFloat16 = {};
    vk::Bool32 shaderRoundingModeRTEFloat32 = {};
    vk::Bool32 shaderRoundingModeRTEFloat64 = {};
    vk::Bool32 shaderRoundingModeRTZFloat16 = {};
    vk::Bool32 shaderRoundingModeRTZFloat32 = {};
    vk::Bool32 shaderRoundingModeRTZFloat64 = {};
    uint32_t maxUpdateAfterBindDescriptorsInAllPools = {};
    vk::Bool32 shaderUniformBufferArrayNonUniformIndexingNative = {};
    vk::Bool32 shaderSampledImageArrayNonUniformIndexingNative = {};
    vk::Bool32 shaderStorageBufferArrayNonUniformIndexingNative = {};
    vk::Bool32 shaderStorageImageArrayNonUniformIndexingNative = {};
    vk::Bool32 shaderInputAttachmentArrayNonUniformIndexingNative = {};
    vk::Bool32 robustBufferAccessUpdateAfterBind = {};
    vk::Bool32 quadDivergentImplicitLod = {};
    uint32_t maxPerStageDescriptorUpdateAfterBindSamplers = {};
    uint32_t maxPerStageDescriptorUpdateAfterBindUniformBuffers = {};
    uint32_t maxPerStageDescriptorUpdateAfterBindStorageBuffers = {};
    uint32_t maxPerStageDescriptorUpdateAfterBindSampledImages = {};
    uint32_t maxPerStageDescriptorUpdateAfterBindStorageImages = {};
    uint32_t maxPerStageDescriptorUpdateAfterBindInputAttachments = {};
    uint32_t maxPerStageUpdateAfterBindResources = {};
    uint32_t maxDescriptorSetUpdateAfterBindSamplers = {};
    uint32_t maxDescriptorSetUpdateAfterBindUniformBuffers = {};
    uint32_t maxDescriptorSetUpdateAfterBindUniformBuffersDynamic = {};
    uint32_t maxDescriptorSetUpdateAfterBindStorageBuffers = {};
    uint32_t maxDescriptorSetUpdateAfterBindStorageBuffersDynamic = {};
    uint32_t maxDescriptorSetUpdateAfterBindSampledImages = {};
    uint32_t maxDescriptorSetUpdateAfterBindStorageImages = {};
    uint32_t maxDescriptorSetUpdateAfterBindInputAttachments = {};
    vk::ResolveModeFlags supportedDepthResolveModes = {};
    vk::ResolveModeFlags supportedStencilResolveModes = {};
    vk::Bool32 independentResolveNone = {};
    vk::Bool32 independentResolve = {};
    vk::Bool32 filterMinmaxSingleComponentFormats = {};
    vk::Bool32 filterMinmaxImageComponentMapping = {};
    uint64_t maxTimelineSemaphoreValueDifference = {};
    vk::SampleCountFlags framebufferIntegerColorSampleCounts = {};

    MappedType map() const
    {
        MappedType r;
        r.driverID = driverID;
        r.driverName = driverName;
        r.driverInfo = driverInfo;
        r.conformanceVersion = conformanceVersion;
        r.denormBehaviorIndependence = denormBehaviorIndependence;
        r.roundingModeIndependence = roundingModeIndependence;
        r.shaderSignedZeroInfNanPreserveFloat16 = shaderSignedZeroInfNanPreserveFloat16;
        r.shaderSignedZeroInfNanPreserveFloat32 = shaderSignedZeroInfNanPreserveFloat32;
        r.shaderSignedZeroInfNanPreserveFloat64 = shaderSignedZeroInfNanPreserveFloat64;
        r.shaderDenormPreserveFloat16 = shaderDenormPreserveFloat16;
        r.shaderDenormPreserveFloat32 = shaderDenormPreserveFloat32;
        r.shaderDenormPreserveFloat64 = shaderDenormPreserveFloat64;
        r.shaderDenormFlushToZeroFloat16 = shaderDenormFlushToZeroFloat16;
        r.shaderDenormFlushToZeroFloat32 = shaderDenormFlushToZeroFloat32;
        r.shaderDenormFlushToZeroFloat64 = shaderDenormFlushToZeroFloat64;
        r.shaderRoundingModeRTEFloat16 = shaderRoundingModeRTEFloat16;
        r.shaderRoundingModeRTEFloat32 = shaderRoundingModeRTEFloat32;
        r.shaderRoundingModeRTEFloat64 = shaderRoundingModeRTEFloat64;
        r.shaderRoundingModeRTZFloat16 = shaderRoundingModeRTZFloat16;
        r.shaderRoundingModeRTZFloat32 = shaderRoundingModeRTZFloat32;
        r.shaderRoundingModeRTZFloat64 = shaderRoundingModeRTZFloat64;
        r.maxUpdateAfterBindDescriptorsInAllPools = maxUpdateAfterBindDescriptorsInAllPools;
        r.shaderUniformBufferArrayNonUniformIndexingNative = shaderUniformBufferArrayNonUniformIndexingNative;
        r.shaderSampledImageArrayNonUniformIndexingNative = shaderSampledImageArrayNonUniformIndexingNative;
        r.shaderStorageBufferArrayNonUniformIndexingNative = shaderStorageBufferArrayNonUniformIndexingNative;
        r.shaderStorageImageArrayNonUniformIndexingNative = shaderStorageImageArrayNonUniformIndexingNative;
        r.shaderInputAttachmentArrayNonUniformIndexingNative = shaderInputAttachmentArrayNonUniformIndexingNative;
        r.robustBufferAccessUpdateAfterBind = robustBufferAccessUpdateAfterBind;
        r.quadDivergentImplicitLod = quadDivergentImplicitLod;
        r.maxPerStageDescriptorUpdateAfterBindSamplers = maxPerStageDescriptorUpdateAfterBindSamplers;
        r.maxPerStageDescriptorUpdateAfterBindUniformBuffers = maxPerStageDescriptorUpdateAfterBindUniformBuffers;
        r.maxPerStageDescriptorUpdateAfterBindStorageBuffers = maxPerStageDescriptorUpdateAfterBindStorageBuffers;
        r.maxPerStageDescriptorUpdateAfterBindSampledImages = maxPerStageDescriptorUpdateAfterBindSampledImages;
        r.maxPerStageDescriptorUpdateAfterBindStorageImages = maxPerStageDescriptorUpdateAfterBindStorageImages;
        r.maxPerStageDescriptorUpdateAfterBindInputAttachments = maxPerStageDescriptorUpdateAfterBindInputAttachments;
        r.maxPerStageUpdateAfterBindResources = maxPerStageUpdateAfterBindResources;
        r.maxDescriptorSetUpdateAfterBindSamplers = maxDescriptorSetUpdateAfterBindSamplers;
        r.maxDescriptorSetUpdateAfterBindUniformBuffers = maxDescriptorSetUpdateAfterBindUniformBuffers;
        r.maxDescriptorSetUpdateAfterBindUniformBuffersDynamic = maxDescriptorSetUpdateAfterBindUniformBuffersDynamic;
        r.maxDescriptorSetUpdateAfterBindStorageBuffers = maxDescriptorSetUpdateAfterBindStorageBuffers;
        r.maxDescriptorSetUpdateAfterBindStorageBuffersDynamic = maxDescriptorSetUpdateAfterBindStorageBuffersDynamic;
        r.maxDescriptorSetUpdateAfterBindSampledImages = maxDescriptorSetUpdateAfterBindSampledImages;
        r.maxDescriptorSetUpdateAfterBindStorageImages = maxDescriptorSetUpdateAfterBindStorageImages;
        r.maxDescriptorSetUpdateAfterBindInputAttachments = maxDescriptorSetUpdateAfterBindInputAttachments;
        r.supportedDepthResolveModes = supportedDepthResolveModes;
        r.supportedStencilResolveModes = supportedStencilResolveModes;
        r.independentResolveNone = independentResolveNone;
        r.independentResolve = independentResolve;
        r.filterMinmaxSingleComponentFormats = filterMinmaxSingleComponentFormats;
        r.filterMinmaxImageComponentMapping = filterMinmaxImageComponentMapping;
        r.maxTimelineSemaphoreValueDifference = maxTimelineSemaphoreValueDifference;
        r.framebufferIntegerColorSampleCounts = framebufferIntegerColorSampleCounts;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct ImageFormatListCreateInfo
{
    using MappedType = vk::ImageFormatListCreateInfo;

    Array<vk::Format> viewFormats = {};

    MappedType map() const
    {
        MappedType r;
        r.viewFormatCount = viewFormats.size();
        r.pViewFormats = viewFormats.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SubpassDescription2
{
    using MappedType = vk::SubpassDescription2;

    vk::SubpassDescriptionFlags flags = {};
    vk::PipelineBindPoint pipelineBindPoint = {};
    uint32_t viewMask = {};
    Array<vk::AttachmentReference2> inputAttachments = {};
    Array<vk::AttachmentReference2> colorAttachments = {};
    Array<vk::AttachmentReference2> resolveAttachments = {};
    const vk::AttachmentReference2* pDepthStencilAttachment = {};
    Array<uint32_t> preserveAttachments = {};

    MappedType map() const
    {
        VKEX_ASSERT(resolveAttachments.size() == colorAttachments.size());
        MappedType r;
        r.flags = flags;
        r.pipelineBindPoint = pipelineBindPoint;
        r.viewMask = viewMask;
        r.inputAttachmentCount = inputAttachments.size();
        r.pInputAttachments = inputAttachments.data();
        r.colorAttachmentCount = colorAttachments.size();
        r.pColorAttachments = colorAttachments.data();
        r.pResolveAttachments = resolveAttachments.data();
        r.pDepthStencilAttachment = pDepthStencilAttachment;
        r.preserveAttachmentCount = preserveAttachments.size();
        r.pPreserveAttachments = preserveAttachments.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct RenderPassCreateInfo2
{
    using MappedType = vk::RenderPassCreateInfo2;

    vk::RenderPassCreateFlags flags = {};
    Array<vk::AttachmentDescription2> attachments = {};
    Array<vk::SubpassDescription2> subpasses = {};
    Array<vk::SubpassDependency2> dependencies = {};
    Array<uint32_t> correlatedViewMasks = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.attachmentCount = attachments.size();
        r.pAttachments = attachments.data();
        r.subpassCount = subpasses.size();
        r.pSubpasses = subpasses.data();
        r.dependencyCount = dependencies.size();
        r.pDependencies = dependencies.data();
        r.correlatedViewMaskCount = correlatedViewMasks.size();
        r.pCorrelatedViewMasks = correlatedViewMasks.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PhysicalDeviceDriverProperties
{
    using MappedType = vk::PhysicalDeviceDriverProperties;

    vk::DriverId driverID = {};
    char driverName[ VK_MAX_DRIVER_NAME_SIZE] = {};
    char driverInfo[ VK_MAX_DRIVER_INFO_SIZE] = {};
    vk::ConformanceVersion conformanceVersion = {};

    MappedType map() const
    {
        MappedType r;
        r.driverID = driverID;
        r.driverName = driverName;
        r.driverInfo = driverInfo;
        r.conformanceVersion = conformanceVersion;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct DescriptorSetLayoutBindingFlagsCreateInfo
{
    using MappedType = vk::DescriptorSetLayoutBindingFlagsCreateInfo;

    Array<vk::DescriptorBindingFlags> bindingFlags = {};

    MappedType map() const
    {
        MappedType r;
        r.bindingCount = bindingFlags.size();
        r.pBindingFlags = bindingFlags.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct DescriptorSetVariableDescriptorCountAllocateInfo
{
    using MappedType = vk::DescriptorSetVariableDescriptorCountAllocateInfo;

    Array<uint32_t> descriptorCounts = {};

    MappedType map() const
    {
        MappedType r;
        r.descriptorSetCount = descriptorCounts.size();
        r.pDescriptorCounts = descriptorCounts.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct FramebufferAttachmentImageInfo
{
    using MappedType = vk::FramebufferAttachmentImageInfo;

    vk::ImageCreateFlags flags = {};
    vk::ImageUsageFlags usage = {};
    uint32_t width = {};
    uint32_t height = {};
    uint32_t layerCount = {};
    Array<vk::Format> viewFormats = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.usage = usage;
        r.width = width;
        r.height = height;
        r.layerCount = layerCount;
        r.viewFormatCount = viewFormats.size();
        r.pViewFormats = viewFormats.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct FramebufferAttachmentsCreateInfo
{
    using MappedType = vk::FramebufferAttachmentsCreateInfo;

    Array<vk::FramebufferAttachmentImageInfo> attachmentImageInfos = {};

    MappedType map() const
    {
        MappedType r;
        r.attachmentImageInfoCount = attachmentImageInfos.size();
        r.pAttachmentImageInfos = attachmentImageInfos.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct RenderPassAttachmentBeginInfo
{
    using MappedType = vk::RenderPassAttachmentBeginInfo;

    Array<vk::ImageView> attachments = {};

    MappedType map() const
    {
        MappedType r;
        r.attachmentCount = attachments.size();
        r.pAttachments = attachments.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct TimelineSemaphoreSubmitInfo
{
    using MappedType = vk::TimelineSemaphoreSubmitInfo;

    Array<uint64_t> waitSemaphoreValues = {};
    Array<uint64_t> signalSemaphoreValues = {};

    MappedType map() const
    {
        MappedType r;
        r.waitSemaphoreValueCount = waitSemaphoreValues.size();
        r.pWaitSemaphoreValues = waitSemaphoreValues.data();
        r.signalSemaphoreValueCount = signalSemaphoreValues.size();
        r.pSignalSemaphoreValues = signalSemaphoreValues.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SemaphoreWaitInfo
{
    using MappedType = vk::SemaphoreWaitInfo;

    vk::SemaphoreWaitFlags flags = {};
    Array<vk::Semaphore> semaphores = {};
    Array<uint64_t> values = {};

    MappedType map() const
    {
        VKEX_ASSERT(values.size() == semaphores.size());
        MappedType r;
        r.flags = flags;
        r.semaphoreCount = semaphores.size();
        r.pSemaphores = semaphores.data();
        r.pValues = values.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_VERSION_1_3
struct PipelineCreationFeedbackCreateInfo
{
    using MappedType = vk::PipelineCreationFeedbackCreateInfo;

    vk::PipelineCreationFeedback* pPipelineCreationFeedback = {};
    Array<vk::PipelineCreationFeedback>* pipelineStageCreationFeedbacks = {};

    MappedType map() const
    {
        MappedType r;
        r.pPipelineCreationFeedback = pPipelineCreationFeedback;
        r.pipelineStageCreationFeedbackCount = pipelineStageCreationFeedbacks ? pipelineStageCreationFeedbacks->size() : 0;
        r.pPipelineStageCreationFeedbacks = pipelineStageCreationFeedbacks ? pipelineStageCreationFeedbacks->data() : nullptr;
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PhysicalDeviceToolProperties
{
    using MappedType = vk::PhysicalDeviceToolProperties;

    char name[ VK_MAX_EXTENSION_NAME_SIZE] = {};
    char version[ VK_MAX_EXTENSION_NAME_SIZE] = {};
    vk::ToolPurposeFlags purposes = {};
    char description[ VK_MAX_DESCRIPTION_SIZE] = {};
    char layer[ VK_MAX_EXTENSION_NAME_SIZE] = {};

    MappedType map() const
    {
        MappedType r;
        r.name = name;
        r.version = version;
        r.purposes = purposes;
        r.description = description;
        r.layer = layer;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct DependencyInfo
{
    using MappedType = vk::DependencyInfo;

    vk::DependencyFlags dependencyFlags = {};
    Array<vk::MemoryBarrier2> memoryBarriers = {};
    Array<vk::BufferMemoryBarrier2> bufferMemoryBarriers = {};
    Array<vk::ImageMemoryBarrier2> imageMemoryBarriers = {};

    MappedType map() const
    {
        MappedType r;
        r.dependencyFlags = dependencyFlags;
        r.memoryBarrierCount = memoryBarriers.size();
        r.pMemoryBarriers = memoryBarriers.data();
        r.bufferMemoryBarrierCount = bufferMemoryBarriers.size();
        r.pBufferMemoryBarriers = bufferMemoryBarriers.data();
        r.imageMemoryBarrierCount = imageMemoryBarriers.size();
        r.pImageMemoryBarriers = imageMemoryBarriers.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SubmitInfo2
{
    using MappedType = vk::SubmitInfo2;

    vk::SubmitFlags flags = {};
    Array<vk::SemaphoreSubmitInfo> waitSemaphoreInfos = {};
    Array<vk::CommandBufferSubmitInfo> commandBufferInfos = {};
    Array<vk::SemaphoreSubmitInfo> signalSemaphoreInfos = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.waitSemaphoreInfoCount = waitSemaphoreInfos.size();
        r.pWaitSemaphoreInfos = waitSemaphoreInfos.data();
        r.commandBufferInfoCount = commandBufferInfos.size();
        r.pCommandBufferInfos = commandBufferInfos.data();
        r.signalSemaphoreInfoCount = signalSemaphoreInfos.size();
        r.pSignalSemaphoreInfos = signalSemaphoreInfos.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct CopyBufferInfo2
{
    using MappedType = vk::CopyBufferInfo2;

    vk::Buffer srcBuffer = {};
    vk::Buffer dstBuffer = {};
    Array<vk::BufferCopy2> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.srcBuffer = srcBuffer;
        r.dstBuffer = dstBuffer;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct CopyImageInfo2
{
    using MappedType = vk::CopyImageInfo2;

    vk::Image srcImage = {};
    vk::ImageLayout srcImageLayout = {};
    vk::Image dstImage = {};
    vk::ImageLayout dstImageLayout = {};
    Array<vk::ImageCopy2> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.srcImage = srcImage;
        r.srcImageLayout = srcImageLayout;
        r.dstImage = dstImage;
        r.dstImageLayout = dstImageLayout;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct CopyBufferToImageInfo2
{
    using MappedType = vk::CopyBufferToImageInfo2;

    vk::Buffer srcBuffer = {};
    vk::Image dstImage = {};
    vk::ImageLayout dstImageLayout = {};
    Array<vk::BufferImageCopy2> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.srcBuffer = srcBuffer;
        r.dstImage = dstImage;
        r.dstImageLayout = dstImageLayout;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct CopyImageToBufferInfo2
{
    using MappedType = vk::CopyImageToBufferInfo2;

    vk::Image srcImage = {};
    vk::ImageLayout srcImageLayout = {};
    vk::Buffer dstBuffer = {};
    Array<vk::BufferImageCopy2> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.srcImage = srcImage;
        r.srcImageLayout = srcImageLayout;
        r.dstBuffer = dstBuffer;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct ImageBlit2
{
    using MappedType = vk::ImageBlit2;

    vk::ImageSubresourceLayers srcSubresource = {};
    vk::Offset3D srcOffsets[2] = {};
    vk::ImageSubresourceLayers dstSubresource = {};
    vk::Offset3D dstOffsets[2] = {};

    MappedType map() const
    {
        MappedType r;
        r.srcSubresource = srcSubresource;
        r.srcOffsets = srcOffsets;
        r.dstSubresource = dstSubresource;
        r.dstOffsets = dstOffsets;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct BlitImageInfo2
{
    using MappedType = vk::BlitImageInfo2;

    vk::Image srcImage = {};
    vk::ImageLayout srcImageLayout = {};
    vk::Image dstImage = {};
    vk::ImageLayout dstImageLayout = {};
    Array<vk::ImageBlit2> regions = {};
    vk::Filter filter = {};

    MappedType map() const
    {
        MappedType r;
        r.srcImage = srcImage;
        r.srcImageLayout = srcImageLayout;
        r.dstImage = dstImage;
        r.dstImageLayout = dstImageLayout;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        r.filter = filter;
        return r;
    }

    operator MappedType() const { return map(); }
};

struct ResolveImageInfo2
{
    using MappedType = vk::ResolveImageInfo2;

    vk::Image srcImage = {};
    vk::ImageLayout srcImageLayout = {};
    vk::Image dstImage = {};
    vk::ImageLayout dstImageLayout = {};
    Array<vk::ImageResolve2> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.srcImage = srcImage;
        r.srcImageLayout = srcImageLayout;
        r.dstImage = dstImage;
        r.dstImageLayout = dstImageLayout;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct WriteDescriptorSetInlineUniformBlock
{
    using MappedType = vk::WriteDescriptorSetInlineUniformBlock;

    Array<std::byte> data = {};

    MappedType map() const
    {
        MappedType r;
        r.dataSize = data.size();
        r.pData = data.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct RenderingInfo
{
    using MappedType = vk::RenderingInfo;

    vk::RenderingFlags flags = {};
    vk::Rect2D renderArea = {};
    uint32_t layerCount = {};
    uint32_t viewMask = {};
    Array<vk::RenderingAttachmentInfo> colorAttachments = {};
    const vk::RenderingAttachmentInfo* pDepthAttachment = {};
    const vk::RenderingAttachmentInfo* pStencilAttachment = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.renderArea = renderArea;
        r.layerCount = layerCount;
        r.viewMask = viewMask;
        r.colorAttachmentCount = colorAttachments.size();
        r.pColorAttachments = colorAttachments.data();
        r.pDepthAttachment = pDepthAttachment;
        r.pStencilAttachment = pStencilAttachment;
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PipelineRenderingCreateInfo
{
    using MappedType = vk::PipelineRenderingCreateInfo;

    uint32_t viewMask = {};
    Array<vk::Format> colorAttachmentFormats = {};
    vk::Format depthAttachmentFormat = {};
    vk::Format stencilAttachmentFormat = {};

    MappedType map() const
    {
        MappedType r;
        r.viewMask = viewMask;
        r.colorAttachmentCount = colorAttachmentFormats.size();
        r.pColorAttachmentFormats = colorAttachmentFormats.data();
        r.depthAttachmentFormat = depthAttachmentFormat;
        r.stencilAttachmentFormat = stencilAttachmentFormat;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct CommandBufferInheritanceRenderingInfo
{
    using MappedType = vk::CommandBufferInheritanceRenderingInfo;

    vk::RenderingFlags flags = {};
    uint32_t viewMask = {};
    Array<vk::Format> colorAttachmentFormats = {};
    vk::Format depthAttachmentFormat = {};
    vk::Format stencilAttachmentFormat = {};
    vk::SampleCountFlagBits rasterizationSamples = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.viewMask = viewMask;
        r.colorAttachmentCount = colorAttachmentFormats.size();
        r.pColorAttachmentFormats = colorAttachmentFormats.data();
        r.depthAttachmentFormat = depthAttachmentFormat;
        r.stencilAttachmentFormat = stencilAttachmentFormat;
        r.rasterizationSamples = rasterizationSamples;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_swapchain
/*
struct SwapchainCreateInfoKHR
{
    using MappedType = vk::SwapchainCreateInfoKHR;

    vk::SwapchainCreateFlagsKHR flags = {};
    vk::SurfaceKHR surface = {};
    uint32_t minImageCount = {};
    vk::Format imageFormat = {};
    vk::ColorSpaceKHR imageColorSpace = {};
    vk::Extent2D imageExtent = {};
    uint32_t imageArrayLayers = {};
    vk::ImageUsageFlags imageUsage = {};
    vk::SharingMode imageSharingMode = {};
    Array<uint32_t> queueFamilyIndices = {};
    vk::SurfaceTransformFlagBitsKHR preTransform = {};
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = {};
    vk::PresentModeKHR presentMode = {};
    vk::Bool32 clipped = {};
    vk::SwapchainKHR oldSwapchain = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.surface = surface;
        r.minImageCount = minImageCount;
        r.imageFormat = imageFormat;
        r.imageColorSpace = imageColorSpace;
        r.imageExtent = imageExtent;
        r.imageArrayLayers = imageArrayLayers;
        r.imageUsage = imageUsage;
        r.imageSharingMode = imageSharingMode;
        r.queueFamilyIndexCount = queueFamilyIndices.size();
        r.pQueueFamilyIndices = queueFamilyIndices.data();
        r.preTransform = preTransform;
        r.compositeAlpha = compositeAlpha;
        r.presentMode = presentMode;
        r.clipped = clipped;
        r.oldSwapchain = oldSwapchain;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct PresentInfoKHR
{
    using MappedType = vk::PresentInfoKHR;

    Array<vk::Semaphore> waitSemaphores = {};
    Array<vk::SwapchainKHR> swapchains = {};
    Array<uint32_t> imageIndices = {};
    Array<vk::Result>* results = {};

    MappedType map() const
    {
        VKEX_ASSERT(imageIndices.size() == swapchains.size());
        if (results && results->size() != swapchains.size())
        {
            results->reset(swapchains.size());
        }
        MappedType r;
        r.waitSemaphoreCount = waitSemaphores.size();
        r.pWaitSemaphores = waitSemaphores.data();
        r.swapchainCount = swapchains.size();
        r.pSwapchains = swapchains.data();
        r.pImageIndices = imageIndices.data();
        r.pResults = results ? results->data() : nullptr;
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct DeviceGroupPresentCapabilitiesKHR
{
    using MappedType = vk::DeviceGroupPresentCapabilitiesKHR;

    uint32_t presentMask[ VK_MAX_DEVICE_GROUP_SIZE] = {};
    vk::DeviceGroupPresentModeFlagsKHR modes = {};

    MappedType map() const
    {
        MappedType r;
        r.presentMask = presentMask;
        r.modes = modes;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct DeviceGroupPresentInfoKHR
{
    using MappedType = vk::DeviceGroupPresentInfoKHR;

    Array<uint32_t> deviceMasks = {};
    vk::DeviceGroupPresentModeFlagBitsKHR mode = {};

    MappedType map() const
    {
        MappedType r;
        r.swapchainCount = deviceMasks.size();
        r.pDeviceMasks = deviceMasks.data();
        r.mode = mode;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_video_queue
struct VideoProfileListInfoKHR
{
    using MappedType = vk::VideoProfileListInfoKHR;

    Array<vk::VideoProfileInfoKHR> profiles = {};

    MappedType map() const
    {
        MappedType r;
        r.profileCount = profiles.size();
        r.pProfiles = profiles.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct VideoBeginCodingInfoKHR
{
    using MappedType = vk::VideoBeginCodingInfoKHR;

    vk::VideoBeginCodingFlagsKHR flags = {};
    vk::VideoSessionKHR videoSession = {};
    vk::VideoSessionParametersKHR videoSessionParameters = {};
    Array<vk::VideoReferenceSlotInfoKHR> referenceSlots = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.videoSession = videoSession;
        r.videoSessionParameters = videoSessionParameters;
        r.referenceSlotCount = referenceSlots.size();
        r.pReferenceSlots = referenceSlots.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_video_decode_queue
struct VideoDecodeInfoKHR
{
    using MappedType = vk::VideoDecodeInfoKHR;

    vk::VideoDecodeFlagsKHR flags = {};
    vk::Buffer srcBuffer = {};
    vk::DeviceSize srcBufferOffset = {};
    vk::DeviceSize srcBufferRange = {};
    vk::VideoPictureResourceInfoKHR dstPictureResource = {};
    const vk::VideoReferenceSlotInfoKHR* pSetupReferenceSlot = {};
    Array<vk::VideoReferenceSlotInfoKHR> referenceSlots = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.srcBuffer = srcBuffer;
        r.srcBufferOffset = srcBufferOffset;
        r.srcBufferRange = srcBufferRange;
        r.dstPictureResource = dstPictureResource;
        r.pSetupReferenceSlot = pSetupReferenceSlot;
        r.referenceSlotCount = referenceSlots.size();
        r.pReferenceSlots = referenceSlots.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_video_decode_h264
struct VideoDecodeH264SessionParametersAddInfoKHR
{
    using MappedType = vk::VideoDecodeH264SessionParametersAddInfoKHR;

    Array<StdVideoH264SequenceParameterSet> stdSPSs = {};
    Array<StdVideoH264PictureParameterSet> stdPPSs = {};

    MappedType map() const
    {
        MappedType r;
        r.stdSPSCount = stdSPSs.size();
        r.pStdSPSs = stdSPSs.data();
        r.stdPPSCount = stdPPSs.size();
        r.pStdPPSs = stdPPSs.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct VideoDecodeH264PictureInfoKHR
{
    using MappedType = vk::VideoDecodeH264PictureInfoKHR;

    const StdVideoDecodeH264PictureInfo* pStdPictureInfo = {};
    Array<uint32_t> sliceOffsets = {};

    MappedType map() const
    {
        MappedType r;
        r.pStdPictureInfo = pStdPictureInfo;
        r.sliceCount = sliceOffsets.size();
        r.pSliceOffsets = sliceOffsets.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_dynamic_rendering
/*
struct AttachmentSampleCountInfoAMD
{
    using MappedType = vk::AttachmentSampleCountInfoAMD;

    Array<vk::SampleCountFlagBits> colorAttachmentSamples = {};
    vk::SampleCountFlagBits depthStencilAttachmentSamples = {};

    MappedType map() const
    {
        MappedType r;
        r.colorAttachmentCount = colorAttachmentSamples.size();
        r.pColorAttachmentSamples = colorAttachmentSamples.data();
        r.depthStencilAttachmentSamples = depthStencilAttachmentSamples;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_KHR_win32_keyed_mutex
struct Win32KeyedMutexAcquireReleaseInfoKHR
{
    using MappedType = vk::Win32KeyedMutexAcquireReleaseInfoKHR;

    Array<vk::DeviceMemory> acquireSyncs = {};
    Array<uint64_t> acquireKeys = {};
    Array<uint32_t> acquireTimeouts = {};
    Array<vk::DeviceMemory> releaseSyncs = {};
    Array<uint64_t> releaseKeys = {};

    MappedType map() const
    {
        VKEX_ASSERT(acquireKeys.size() == acquireSyncs.size());
        VKEX_ASSERT(acquireTimeouts.size() == acquireSyncs.size());
        VKEX_ASSERT(releaseKeys.size() == releaseSyncs.size());
        MappedType r;
        r.acquireCount = acquireSyncs.size();
        r.pAcquireSyncs = acquireSyncs.data();
        r.pAcquireKeys = acquireKeys.data();
        r.pAcquireTimeouts = acquireTimeouts.data();
        r.releaseCount = releaseSyncs.size();
        r.pReleaseSyncs = releaseSyncs.data();
        r.pReleaseKeys = releaseKeys.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_external_semaphore_win32
struct D3D12FenceSubmitInfoKHR
{
    using MappedType = vk::D3D12FenceSubmitInfoKHR;

    Array<uint64_t> waitSemaphoreValues = {};
    Array<uint64_t> signalSemaphoreValues = {};

    MappedType map() const
    {
        MappedType r;
        r.waitSemaphoreValuesCount = waitSemaphoreValues.size();
        r.pWaitSemaphoreValues = waitSemaphoreValues.data();
        r.signalSemaphoreValuesCount = signalSemaphoreValues.size();
        r.pSignalSemaphoreValues = signalSemaphoreValues.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_incremental_present
struct PresentRegionKHR
{
    using MappedType = vk::PresentRegionKHR;

    Array<vk::RectLayerKHR> rectangles = {};

    MappedType map() const
    {
        MappedType r;
        r.rectangleCount = rectangles.size();
        r.pRectangles = rectangles.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct PresentRegionsKHR
{
    using MappedType = vk::PresentRegionsKHR;

    Array<vk::PresentRegionKHR> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.swapchainCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_performance_query
/*
struct PerformanceCounterKHR
{
    using MappedType = vk::PerformanceCounterKHR;

    vk::PerformanceCounterUnitKHR unit = {};
    vk::PerformanceCounterScopeKHR scope = {};
    vk::PerformanceCounterStorageKHR storage = {};
    uint8_t uuid[ VK_UUID_SIZE] = {};

    MappedType map() const
    {
        MappedType r;
        r.unit = unit;
        r.scope = scope;
        r.storage = storage;
        r.uuid = uuid;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct PerformanceCounterDescriptionKHR
{
    using MappedType = vk::PerformanceCounterDescriptionKHR;

    vk::PerformanceCounterDescriptionFlagsKHR flags = {};
    char name[ VK_MAX_DESCRIPTION_SIZE] = {};
    char category[ VK_MAX_DESCRIPTION_SIZE] = {};
    char description[ VK_MAX_DESCRIPTION_SIZE] = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.name = name;
        r.category = category;
        r.description = description;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct QueryPoolPerformanceCreateInfoKHR
{
    using MappedType = vk::QueryPoolPerformanceCreateInfoKHR;

    uint32_t queueFamilyIndex = {};
    Array<uint32_t> counterIndices = {};

    MappedType map() const
    {
        MappedType r;
        r.queueFamilyIndex = queueFamilyIndex;
        r.counterIndexCount = counterIndices.size();
        r.pCounterIndices = counterIndices.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_video_decode_h265
struct VideoDecodeH265SessionParametersAddInfoKHR
{
    using MappedType = vk::VideoDecodeH265SessionParametersAddInfoKHR;

    Array<StdVideoH265VideoParameterSet> stdVPSs = {};
    Array<StdVideoH265SequenceParameterSet> stdSPSs = {};
    Array<StdVideoH265PictureParameterSet> stdPPSs = {};

    MappedType map() const
    {
        MappedType r;
        r.stdVPSCount = stdVPSs.size();
        r.pStdVPSs = stdVPSs.data();
        r.stdSPSCount = stdSPSs.size();
        r.pStdSPSs = stdSPSs.data();
        r.stdPPSCount = stdPPSs.size();
        r.pStdPPSs = stdPPSs.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct VideoDecodeH265PictureInfoKHR
{
    using MappedType = vk::VideoDecodeH265PictureInfoKHR;

    const StdVideoDecodeH265PictureInfo* pStdPictureInfo = {};
    Array<uint32_t> sliceSegmentOffsets = {};

    MappedType map() const
    {
        MappedType r;
        r.pStdPictureInfo = pStdPictureInfo;
        r.sliceSegmentCount = sliceSegmentOffsets.size();
        r.pSliceSegmentOffsets = sliceSegmentOffsets.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_global_priority
/*
struct QueueFamilyGlobalPriorityPropertiesKHR
{
    using MappedType = vk::QueueFamilyGlobalPriorityPropertiesKHR;

    uint32_t priorityCount = {};
    vk::QueueGlobalPriorityKHR priorities[ VK_MAX_GLOBAL_PRIORITY_SIZE_KHR] = {};

    MappedType map() const
    {
        MappedType r;
        r.priorityCount = priorityCount;
        r.priorities = priorities;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_KHR_fragment_shading_rate
/*
struct PipelineFragmentShadingRateStateCreateInfoKHR
{
    using MappedType = vk::PipelineFragmentShadingRateStateCreateInfoKHR;

    vk::Extent2D fragmentSize = {};
    vk::FragmentShadingRateCombinerOpKHR combinerOps[2] = {};

    MappedType map() const
    {
        MappedType r;
        r.fragmentSize = fragmentSize;
        r.combinerOps = combinerOps;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_KHR_pipeline_executable_properties
/*
struct PipelineExecutablePropertiesKHR
{
    using MappedType = vk::PipelineExecutablePropertiesKHR;

    vk::ShaderStageFlags stages = {};
    char name[ VK_MAX_DESCRIPTION_SIZE] = {};
    char description[ VK_MAX_DESCRIPTION_SIZE] = {};
    uint32_t subgroupSize = {};

    MappedType map() const
    {
        MappedType r;
        r.stages = stages;
        r.name = name;
        r.description = description;
        r.subgroupSize = subgroupSize;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct PipelineExecutableStatisticKHR
{
    using MappedType = vk::PipelineExecutableStatisticKHR;

    char name[ VK_MAX_DESCRIPTION_SIZE] = {};
    char description[ VK_MAX_DESCRIPTION_SIZE] = {};
    vk::PipelineExecutableStatisticFormatKHR format = {};
    vk::PipelineExecutableStatisticValueKHR value = {};

    MappedType map() const
    {
        MappedType r;
        r.name = name;
        r.description = description;
        r.format = format;
        r.value = value;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct PipelineExecutableInternalRepresentationKHR
{
    using MappedType = vk::PipelineExecutableInternalRepresentationKHR;

    char name[ VK_MAX_DESCRIPTION_SIZE] = {};
    char description[ VK_MAX_DESCRIPTION_SIZE] = {};
    vk::Bool32 isText = {};
    Array<std::byte>* data = {};

    MappedType map() const
    {
        MappedType r;
        r.name = name;
        r.description = description;
        r.isText = isText;
        r.dataSize = data ? data->size() : 0;
        r.pData = data ? data->data() : nullptr;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_KHR_pipeline_library
struct PipelineLibraryCreateInfoKHR
{
    using MappedType = vk::PipelineLibraryCreateInfoKHR;

    Array<vk::Pipeline> libraries = {};

    MappedType map() const
    {
        MappedType r;
        r.libraryCount = libraries.size();
        r.pLibraries = libraries.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_present_id
struct PresentIdKHR
{
    using MappedType = vk::PresentIdKHR;

    Array<uint64_t> presentIds = {};

    MappedType map() const
    {
        MappedType r;
        r.swapchainCount = presentIds.size();
        r.pPresentIds = presentIds.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_video_encode_queue
struct VideoEncodeInfoKHR
{
    using MappedType = vk::VideoEncodeInfoKHR;

    vk::VideoEncodeFlagsKHR flags = {};
    vk::Buffer dstBuffer = {};
    vk::DeviceSize dstBufferOffset = {};
    vk::DeviceSize dstBufferRange = {};
    vk::VideoPictureResourceInfoKHR srcPictureResource = {};
    const vk::VideoReferenceSlotInfoKHR* pSetupReferenceSlot = {};
    Array<vk::VideoReferenceSlotInfoKHR> referenceSlots = {};
    uint32_t precedingExternallyEncodedBytes = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.dstBuffer = dstBuffer;
        r.dstBufferOffset = dstBufferOffset;
        r.dstBufferRange = dstBufferRange;
        r.srcPictureResource = srcPictureResource;
        r.pSetupReferenceSlot = pSetupReferenceSlot;
        r.referenceSlotCount = referenceSlots.size();
        r.pReferenceSlots = referenceSlots.data();
        r.precedingExternallyEncodedBytes = precedingExternallyEncodedBytes;
        return r;
    }

    operator MappedType() const { return map(); }
};

struct VideoEncodeRateControlInfoKHR
{
    using MappedType = vk::VideoEncodeRateControlInfoKHR;

    vk::VideoEncodeRateControlFlagsKHR flags = {};
    vk::VideoEncodeRateControlModeFlagBitsKHR rateControlMode = {};
    Array<vk::VideoEncodeRateControlLayerInfoKHR> layers = {};
    uint32_t virtualBufferSizeInMs = {};
    uint32_t initialVirtualBufferSizeInMs = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.rateControlMode = rateControlMode;
        r.layerCount = layers.size();
        r.pLayers = layers.data();
        r.virtualBufferSizeInMs = virtualBufferSizeInMs;
        r.initialVirtualBufferSizeInMs = initialVirtualBufferSizeInMs;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_maintenance5
/*
struct RenderingAreaInfoKHR
{
    using MappedType = vk::RenderingAreaInfoKHR;

    uint32_t viewMask = {};
    Array<vk::Format> colorAttachmentFormats = {};
    vk::Format depthAttachmentFormat = {};
    vk::Format stencilAttachmentFormat = {};

    MappedType map() const
    {
        MappedType r;
        r.viewMask = viewMask;
        r.colorAttachmentCount = colorAttachmentFormats.size();
        r.pColorAttachmentFormats = colorAttachmentFormats.data();
        r.depthAttachmentFormat = depthAttachmentFormat;
        r.stencilAttachmentFormat = stencilAttachmentFormat;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_debug_marker
struct DebugMarkerObjectTagInfoEXT
{
    using MappedType = vk::DebugMarkerObjectTagInfoEXT;

    vk::DebugReportObjectTypeEXT objectType = {};
    uint64_t object = {};
    uint64_t tagName = {};
    Array<std::byte> tag = {};

    MappedType map() const
    {
        MappedType r;
        r.objectType = objectType;
        r.object = object;
        r.tagName = tagName;
        r.tagSize = tag.size();
        r.pTag = tag.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct DebugMarkerMarkerInfoEXT
{
    using MappedType = vk::DebugMarkerMarkerInfoEXT;

    const char* pMarkerName = {};
    float color[4] = {};

    MappedType map() const
    {
        MappedType r;
        r.pMarkerName = pMarkerName;
        r.color = color;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_NVX_binary_import
struct CuModuleCreateInfoNVX
{
    using MappedType = vk::CuModuleCreateInfoNVX;

    Array<std::byte> data = {};

    MappedType map() const
    {
        MappedType r;
        r.dataSize = data.size();
        r.pData = data.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct CuLaunchInfoNVX
{
    using MappedType = vk::CuLaunchInfoNVX;

    vk::CuFunctionNVX function = {};
    uint32_t gridDimX = {};
    uint32_t gridDimY = {};
    uint32_t gridDimZ = {};
    uint32_t blockDimX = {};
    uint32_t blockDimY = {};
    uint32_t blockDimZ = {};
    uint32_t sharedMemBytes = {};
    Array<const void*> params = {};
    Array<const void*> extras = {};

    MappedType map() const
    {
        MappedType r;
        r.function = function;
        r.gridDimX = gridDimX;
        r.gridDimY = gridDimY;
        r.gridDimZ = gridDimZ;
        r.blockDimX = blockDimX;
        r.blockDimY = blockDimY;
        r.blockDimZ = blockDimZ;
        r.sharedMemBytes = sharedMemBytes;
        r.paramCount = params.size();
        r.pParams = params.data();
        r.extraCount = extras.size();
        r.pExtras = extras.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_video_encode_h264
struct VideoEncodeH264SessionParametersAddInfoEXT
{
    using MappedType = vk::VideoEncodeH264SessionParametersAddInfoEXT;

    Array<StdVideoH264SequenceParameterSet> stdSPSs = {};
    Array<StdVideoH264PictureParameterSet> stdPPSs = {};

    MappedType map() const
    {
        MappedType r;
        r.stdSPSCount = stdSPSs.size();
        r.pStdSPSs = stdSPSs.data();
        r.stdPPSCount = stdPPSs.size();
        r.pStdPPSs = stdPPSs.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct VideoEncodeH264PictureInfoEXT
{
    using MappedType = vk::VideoEncodeH264PictureInfoEXT;

    Array<vk::VideoEncodeH264NaluSliceInfoEXT> naluSliceEntries = {};
    const StdVideoEncodeH264PictureInfo* pStdPictureInfo = {};
    vk::Bool32 generatePrefixNalu = {};

    MappedType map() const
    {
        MappedType r;
        r.naluSliceEntryCount = naluSliceEntries.size();
        r.pNaluSliceEntries = naluSliceEntries.data();
        r.pStdPictureInfo = pStdPictureInfo;
        r.generatePrefixNalu = generatePrefixNalu;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_video_encode_h265
struct VideoEncodeH265SessionParametersAddInfoEXT
{
    using MappedType = vk::VideoEncodeH265SessionParametersAddInfoEXT;

    Array<StdVideoH265VideoParameterSet> stdVPSs = {};
    Array<StdVideoH265SequenceParameterSet> stdSPSs = {};
    Array<StdVideoH265PictureParameterSet> stdPPSs = {};

    MappedType map() const
    {
        MappedType r;
        r.stdVPSCount = stdVPSs.size();
        r.pStdVPSs = stdVPSs.data();
        r.stdSPSCount = stdSPSs.size();
        r.pStdSPSs = stdSPSs.data();
        r.stdPPSCount = stdPPSs.size();
        r.pStdPPSs = stdPPSs.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct VideoEncodeH265PictureInfoEXT
{
    using MappedType = vk::VideoEncodeH265PictureInfoEXT;

    Array<vk::VideoEncodeH265NaluSliceSegmentInfoEXT> naluSliceSegmentEntries = {};
    const StdVideoEncodeH265PictureInfo* pStdPictureInfo = {};

    MappedType map() const
    {
        MappedType r;
        r.naluSliceSegmentEntryCount = naluSliceSegmentEntries.size();
        r.pNaluSliceSegmentEntries = naluSliceSegmentEntries.data();
        r.pStdPictureInfo = pStdPictureInfo;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_AMD_shader_info
/*
struct ShaderStatisticsInfoAMD
{
    using MappedType = vk::ShaderStatisticsInfoAMD;

    vk::ShaderStageFlags shaderStageMask = {};
    vk::ShaderResourceUsageAMD resourceUsage = {};
    uint32_t numPhysicalVgprs = {};
    uint32_t numPhysicalSgprs = {};
    uint32_t numAvailableVgprs = {};
    uint32_t numAvailableSgprs = {};
    uint32_t computeWorkGroupSize[3] = {};

    MappedType map() const
    {
        MappedType r;
        r.shaderStageMask = shaderStageMask;
        r.resourceUsage = resourceUsage;
        r.numPhysicalVgprs = numPhysicalVgprs;
        r.numPhysicalSgprs = numPhysicalSgprs;
        r.numAvailableVgprs = numAvailableVgprs;
        r.numAvailableSgprs = numAvailableSgprs;
        r.computeWorkGroupSize = computeWorkGroupSize;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_NV_win32_keyed_mutex
struct Win32KeyedMutexAcquireReleaseInfoNV
{
    using MappedType = vk::Win32KeyedMutexAcquireReleaseInfoNV;

    Array<vk::DeviceMemory> acquireSyncs = {};
    Array<uint64_t> acquireKeys = {};
    Array<uint32_t> acquireTimeoutMilliseconds = {};
    Array<vk::DeviceMemory> releaseSyncs = {};
    Array<uint64_t> releaseKeys = {};

    MappedType map() const
    {
        VKEX_ASSERT(acquireKeys.size() == acquireSyncs.size());
        VKEX_ASSERT(acquireTimeoutMilliseconds.size() == acquireSyncs.size());
        VKEX_ASSERT(releaseKeys.size() == releaseSyncs.size());
        MappedType r;
        r.acquireCount = acquireSyncs.size();
        r.pAcquireSyncs = acquireSyncs.data();
        r.pAcquireKeys = acquireKeys.data();
        r.pAcquireTimeoutMilliseconds = acquireTimeoutMilliseconds.data();
        r.releaseCount = releaseSyncs.size();
        r.pReleaseSyncs = releaseSyncs.data();
        r.pReleaseKeys = releaseKeys.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_validation_flags
struct ValidationFlagsEXT
{
    using MappedType = vk::ValidationFlagsEXT;

    Array<vk::ValidationCheckEXT> disabledValidationChecks = {};

    MappedType map() const
    {
        MappedType r;
        r.disabledValidationCheckCount = disabledValidationChecks.size();
        r.pDisabledValidationChecks = disabledValidationChecks.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_NV_clip_space_w_scaling
/*
struct PipelineViewportWScalingStateCreateInfoNV
{
    using MappedType = vk::PipelineViewportWScalingStateCreateInfoNV;

    vk::Bool32 viewportWScalingEnable = {};
    Array<vk::ViewportWScalingNV> viewportWScalings = {};

    MappedType map() const
    {
        MappedType r;
        r.viewportWScalingEnable = viewportWScalingEnable;
        r.viewportCount = viewportWScalings.size();
        r.pViewportWScalings = viewportWScalings.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_GOOGLE_display_timing
struct PresentTimesInfoGOOGLE
{
    using MappedType = vk::PresentTimesInfoGOOGLE;

    Array<vk::PresentTimeGOOGLE> times = {};

    MappedType map() const
    {
        MappedType r;
        r.swapchainCount = times.size();
        r.pTimes = times.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_NV_viewport_swizzle
struct PipelineViewportSwizzleStateCreateInfoNV
{
    using MappedType = vk::PipelineViewportSwizzleStateCreateInfoNV;

    vk::PipelineViewportSwizzleStateCreateFlagsNV flags = {};
    Array<vk::ViewportSwizzleNV> viewportSwizzles = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.viewportCount = viewportSwizzles.size();
        r.pViewportSwizzles = viewportSwizzles.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_discard_rectangles
/*
struct PipelineDiscardRectangleStateCreateInfoEXT
{
    using MappedType = vk::PipelineDiscardRectangleStateCreateInfoEXT;

    vk::PipelineDiscardRectangleStateCreateFlagsEXT flags = {};
    vk::DiscardRectangleModeEXT discardRectangleMode = {};
    Array<vk::Rect2D> discardRectangles = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.discardRectangleMode = discardRectangleMode;
        r.discardRectangleCount = discardRectangles.size();
        r.pDiscardRectangles = discardRectangles.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_debug_utils
/*
struct DebugUtilsLabelEXT
{
    using MappedType = vk::DebugUtilsLabelEXT;

    const char* pLabelName = {};
    float color[4] = {};

    MappedType map() const
    {
        MappedType r;
        r.pLabelName = pLabelName;
        r.color = color;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct DebugUtilsMessengerCallbackDataEXT
{
    using MappedType = vk::DebugUtilsMessengerCallbackDataEXT;

    vk::DebugUtilsMessengerCallbackDataFlagsEXT flags = {};
    const char* pMessageIdName = {};
    int32_t messageIdNumber = {};
    const char* pMessage = {};
    Array<vk::DebugUtilsLabelEXT> queueLabels = {};
    Array<vk::DebugUtilsLabelEXT> cmdBufLabels = {};
    Array<vk::DebugUtilsObjectNameInfoEXT> objects = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.pMessageIdName = pMessageIdName;
        r.messageIdNumber = messageIdNumber;
        r.pMessage = pMessage;
        r.queueLabelCount = queueLabels.size();
        r.pQueueLabels = queueLabels.data();
        r.cmdBufLabelCount = cmdBufLabels.size();
        r.pCmdBufLabels = cmdBufLabels.data();
        r.objectCount = objects.size();
        r.pObjects = objects.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct DebugUtilsObjectTagInfoEXT
{
    using MappedType = vk::DebugUtilsObjectTagInfoEXT;

    vk::ObjectType objectType = {};
    uint64_t objectHandle = {};
    uint64_t tagName = {};
    Array<std::byte> tag = {};

    MappedType map() const
    {
        MappedType r;
        r.objectType = objectType;
        r.objectHandle = objectHandle;
        r.tagName = tagName;
        r.tagSize = tag.size();
        r.pTag = tag.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_AMDX_shader_enqueue
struct ExecutionGraphPipelineCreateInfoAMDX
{
    using MappedType = vk::ExecutionGraphPipelineCreateInfoAMDX;

    vk::PipelineCreateFlags flags = {};
    Array<vk::PipelineShaderStageCreateInfo> stages = {};
    const vk::PipelineLibraryCreateInfoKHR* pLibraryInfo = {};
    vk::PipelineLayout layout = {};
    vk::Pipeline basePipelineHandle = {};
    int32_t basePipelineIndex = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.stageCount = stages.size();
        r.pStages = stages.data();
        r.pLibraryInfo = pLibraryInfo;
        r.layout = layout;
        r.basePipelineHandle = basePipelineHandle;
        r.basePipelineIndex = basePipelineIndex;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_sample_locations
struct SampleLocationsInfoEXT
{
    using MappedType = vk::SampleLocationsInfoEXT;

    vk::SampleCountFlagBits sampleLocationsPerPixel = {};
    vk::Extent2D sampleLocationGridSize = {};
    Array<vk::SampleLocationEXT> sampleLocations = {};

    MappedType map() const
    {
        MappedType r;
        r.sampleLocationsPerPixel = sampleLocationsPerPixel;
        r.sampleLocationGridSize = sampleLocationGridSize;
        r.sampleLocationsCount = sampleLocations.size();
        r.pSampleLocations = sampleLocations.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct RenderPassSampleLocationsBeginInfoEXT
{
    using MappedType = vk::RenderPassSampleLocationsBeginInfoEXT;

    Array<vk::AttachmentSampleLocationsEXT> attachmentInitialSampleLocations = {};
    Array<vk::SubpassSampleLocationsEXT> postSubpassSampleLocations = {};

    MappedType map() const
    {
        MappedType r;
        r.attachmentInitialSampleLocationsCount = attachmentInitialSampleLocations.size();
        r.pAttachmentInitialSampleLocations = attachmentInitialSampleLocations.data();
        r.postSubpassSampleLocationsCount = postSubpassSampleLocations.size();
        r.pPostSubpassSampleLocations = postSubpassSampleLocations.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PhysicalDeviceSampleLocationsPropertiesEXT
{
    using MappedType = vk::PhysicalDeviceSampleLocationsPropertiesEXT;

    vk::SampleCountFlags sampleLocationSampleCounts = {};
    vk::Extent2D maxSampleLocationGridSize = {};
    float sampleLocationCoordinateRange[2] = {};
    uint32_t sampleLocationSubPixelBits = {};
    vk::Bool32 variableSampleLocations = {};

    MappedType map() const
    {
        MappedType r;
        r.sampleLocationSampleCounts = sampleLocationSampleCounts;
        r.maxSampleLocationGridSize = maxSampleLocationGridSize;
        r.sampleLocationCoordinateRange = sampleLocationCoordinateRange;
        r.sampleLocationSubPixelBits = sampleLocationSubPixelBits;
        r.variableSampleLocations = variableSampleLocations;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_NV_framebuffer_mixed_samples
/*
struct PipelineCoverageModulationStateCreateInfoNV
{
    using MappedType = vk::PipelineCoverageModulationStateCreateInfoNV;

    vk::PipelineCoverageModulationStateCreateFlagsNV flags = {};
    vk::CoverageModulationModeNV coverageModulationMode = {};
    vk::Bool32 coverageModulationTableEnable = {};
    Array<float> coverageModulationTable = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.coverageModulationMode = coverageModulationMode;
        r.coverageModulationTableEnable = coverageModulationTableEnable;
        r.coverageModulationTableCount = coverageModulationTable.size();
        r.pCoverageModulationTable = coverageModulationTable.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_image_drm_format_modifier
struct DrmFormatModifierPropertiesListEXT
{
    using MappedType = vk::DrmFormatModifierPropertiesListEXT;

    Array<vk::DrmFormatModifierPropertiesEXT>* drmFormatModifierProperties = {};

    MappedType map() const
    {
        MappedType r;
        r.drmFormatModifierCount = drmFormatModifierProperties ? drmFormatModifierProperties->size() : 0;
        r.pDrmFormatModifierProperties = drmFormatModifierProperties ? drmFormatModifierProperties->data() : nullptr;
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PhysicalDeviceImageDrmFormatModifierInfoEXT
{
    using MappedType = vk::PhysicalDeviceImageDrmFormatModifierInfoEXT;

    uint64_t drmFormatModifier = {};
    vk::SharingMode sharingMode = {};
    Array<uint32_t> queueFamilyIndices = {};

    MappedType map() const
    {
        MappedType r;
        r.drmFormatModifier = drmFormatModifier;
        r.sharingMode = sharingMode;
        r.queueFamilyIndexCount = queueFamilyIndices.size();
        r.pQueueFamilyIndices = queueFamilyIndices.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct ImageDrmFormatModifierListCreateInfoEXT
{
    using MappedType = vk::ImageDrmFormatModifierListCreateInfoEXT;

    Array<uint64_t> drmFormatModifiers = {};

    MappedType map() const
    {
        MappedType r;
        r.drmFormatModifierCount = drmFormatModifiers.size();
        r.pDrmFormatModifiers = drmFormatModifiers.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct ImageDrmFormatModifierExplicitCreateInfoEXT
{
    using MappedType = vk::ImageDrmFormatModifierExplicitCreateInfoEXT;

    uint64_t drmFormatModifier = {};
    Array<vk::SubresourceLayout> planeLayouts = {};

    MappedType map() const
    {
        MappedType r;
        r.drmFormatModifier = drmFormatModifier;
        r.drmFormatModifierPlaneCount = planeLayouts.size();
        r.pPlaneLayouts = planeLayouts.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct DrmFormatModifierPropertiesList2EXT
{
    using MappedType = vk::DrmFormatModifierPropertiesList2EXT;

    Array<vk::DrmFormatModifierProperties2EXT>* drmFormatModifierProperties = {};

    MappedType map() const
    {
        MappedType r;
        r.drmFormatModifierCount = drmFormatModifierProperties ? drmFormatModifierProperties->size() : 0;
        r.pDrmFormatModifierProperties = drmFormatModifierProperties ? drmFormatModifierProperties->data() : nullptr;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_validation_cache
struct ValidationCacheCreateInfoEXT
{
    using MappedType = vk::ValidationCacheCreateInfoEXT;

    vk::ValidationCacheCreateFlagsEXT flags = {};
    Array<std::byte> initialData = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.initialDataSize = initialData.size();
        r.pInitialData = initialData.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_NV_shading_rate_image
struct ShadingRatePaletteNV
{
    using MappedType = vk::ShadingRatePaletteNV;

    Array<vk::ShadingRatePaletteEntryNV> shadingRatePaletteEntries = {};

    MappedType map() const
    {
        MappedType r;
        r.shadingRatePaletteEntryCount = shadingRatePaletteEntries.size();
        r.pShadingRatePaletteEntries = shadingRatePaletteEntries.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct PipelineViewportShadingRateImageStateCreateInfoNV
{
    using MappedType = vk::PipelineViewportShadingRateImageStateCreateInfoNV;

    vk::Bool32 shadingRateImageEnable = {};
    Array<vk::ShadingRatePaletteNV> shadingRatePalettes = {};

    MappedType map() const
    {
        MappedType r;
        r.shadingRateImageEnable = shadingRateImageEnable;
        r.viewportCount = shadingRatePalettes.size();
        r.pShadingRatePalettes = shadingRatePalettes.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct CoarseSampleOrderCustomNV
{
    using MappedType = vk::CoarseSampleOrderCustomNV;

    vk::ShadingRatePaletteEntryNV shadingRate = {};
    uint32_t sampleCount = {};
    Array<vk::CoarseSampleLocationNV> sampleLocations = {};

    MappedType map() const
    {
        MappedType r;
        r.shadingRate = shadingRate;
        r.sampleCount = sampleCount;
        r.sampleLocationCount = sampleLocations.size();
        r.pSampleLocations = sampleLocations.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct PipelineViewportCoarseSampleOrderStateCreateInfoNV
{
    using MappedType = vk::PipelineViewportCoarseSampleOrderStateCreateInfoNV;

    vk::CoarseSampleOrderTypeNV sampleOrderType = {};
    Array<vk::CoarseSampleOrderCustomNV> customSampleOrders = {};

    MappedType map() const
    {
        MappedType r;
        r.sampleOrderType = sampleOrderType;
        r.customSampleOrderCount = customSampleOrders.size();
        r.pCustomSampleOrders = customSampleOrders.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_NV_ray_tracing
struct RayTracingPipelineCreateInfoNV
{
    using MappedType = vk::RayTracingPipelineCreateInfoNV;

    vk::PipelineCreateFlags flags = {};
    Array<vk::PipelineShaderStageCreateInfo> stages = {};
    Array<vk::RayTracingShaderGroupCreateInfoNV> groups = {};
    uint32_t maxRecursionDepth = {};
    vk::PipelineLayout layout = {};
    vk::Pipeline basePipelineHandle = {};
    int32_t basePipelineIndex = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.stageCount = stages.size();
        r.pStages = stages.data();
        r.groupCount = groups.size();
        r.pGroups = groups.data();
        r.maxRecursionDepth = maxRecursionDepth;
        r.layout = layout;
        r.basePipelineHandle = basePipelineHandle;
        r.basePipelineIndex = basePipelineIndex;
        return r;
    }

    operator MappedType() const { return map(); }
};

struct AccelerationStructureInfoNV
{
    using MappedType = vk::AccelerationStructureInfoNV;

    vk::AccelerationStructureTypeNV type = {};
    vk::BuildAccelerationStructureFlagsNV flags = {};
    uint32_t instanceCount = {};
    Array<vk::GeometryNV> geometries = {};

    MappedType map() const
    {
        MappedType r;
        r.type = type;
        r.flags = flags;
        r.instanceCount = instanceCount;
        r.geometryCount = geometries.size();
        r.pGeometries = geometries.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct BindAccelerationStructureMemoryInfoNV
{
    using MappedType = vk::BindAccelerationStructureMemoryInfoNV;

    vk::AccelerationStructureNV accelerationStructure = {};
    vk::DeviceMemory memory = {};
    vk::DeviceSize memoryOffset = {};
    Array<uint32_t> deviceIndices = {};

    MappedType map() const
    {
        MappedType r;
        r.accelerationStructure = accelerationStructure;
        r.memory = memory;
        r.memoryOffset = memoryOffset;
        r.deviceIndexCount = deviceIndices.size();
        r.pDeviceIndices = deviceIndices.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct WriteDescriptorSetAccelerationStructureNV
{
    using MappedType = vk::WriteDescriptorSetAccelerationStructureNV;

    Array<vk::AccelerationStructureNV> accelerationStructures = {};

    MappedType map() const
    {
        MappedType r;
        r.accelerationStructureCount = accelerationStructures.size();
        r.pAccelerationStructures = accelerationStructures.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct TransformMatrixKHR
{
    using MappedType = vk::TransformMatrixKHR;

    float matrix[3][4] = {};

    MappedType map() const
    {
        MappedType r;
        r.matrix = matrix;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_vertex_attribute_divisor
struct PipelineVertexInputDivisorStateCreateInfoEXT
{
    using MappedType = vk::PipelineVertexInputDivisorStateCreateInfoEXT;

    Array<vk::VertexInputBindingDivisorDescriptionEXT> vertexBindingDivisors = {};

    MappedType map() const
    {
        MappedType r;
        r.vertexBindingDivisorCount = vertexBindingDivisors.size();
        r.pVertexBindingDivisors = vertexBindingDivisors.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_NV_mesh_shader
/*
struct PhysicalDeviceMeshShaderPropertiesNV
{
    using MappedType = vk::PhysicalDeviceMeshShaderPropertiesNV;

    uint32_t maxDrawMeshTasksCount = {};
    uint32_t maxTaskWorkGroupInvocations = {};
    uint32_t maxTaskWorkGroupSize[3] = {};
    uint32_t maxTaskTotalMemorySize = {};
    uint32_t maxTaskOutputCount = {};
    uint32_t maxMeshWorkGroupInvocations = {};
    uint32_t maxMeshWorkGroupSize[3] = {};
    uint32_t maxMeshTotalMemorySize = {};
    uint32_t maxMeshOutputVertices = {};
    uint32_t maxMeshOutputPrimitives = {};
    uint32_t maxMeshMultiviewViewCount = {};
    uint32_t meshOutputPerVertexGranularity = {};
    uint32_t meshOutputPerPrimitiveGranularity = {};

    MappedType map() const
    {
        MappedType r;
        r.maxDrawMeshTasksCount = maxDrawMeshTasksCount;
        r.maxTaskWorkGroupInvocations = maxTaskWorkGroupInvocations;
        r.maxTaskWorkGroupSize = maxTaskWorkGroupSize;
        r.maxTaskTotalMemorySize = maxTaskTotalMemorySize;
        r.maxTaskOutputCount = maxTaskOutputCount;
        r.maxMeshWorkGroupInvocations = maxMeshWorkGroupInvocations;
        r.maxMeshWorkGroupSize = maxMeshWorkGroupSize;
        r.maxMeshTotalMemorySize = maxMeshTotalMemorySize;
        r.maxMeshOutputVertices = maxMeshOutputVertices;
        r.maxMeshOutputPrimitives = maxMeshOutputPrimitives;
        r.maxMeshMultiviewViewCount = maxMeshMultiviewViewCount;
        r.meshOutputPerVertexGranularity = meshOutputPerVertexGranularity;
        r.meshOutputPerPrimitiveGranularity = meshOutputPerPrimitiveGranularity;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_NV_scissor_exclusive
/*
struct PipelineViewportExclusiveScissorStateCreateInfoNV
{
    using MappedType = vk::PipelineViewportExclusiveScissorStateCreateInfoNV;

    Array<vk::Rect2D> exclusiveScissors = {};

    MappedType map() const
    {
        MappedType r;
        r.exclusiveScissorCount = exclusiveScissors.size();
        r.pExclusiveScissors = exclusiveScissors.data();
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_memory_budget
/*
struct PhysicalDeviceMemoryBudgetPropertiesEXT
{
    using MappedType = vk::PhysicalDeviceMemoryBudgetPropertiesEXT;

    vk::DeviceSize heapBudget[ VK_MAX_MEMORY_HEAPS] = {};
    vk::DeviceSize heapUsage[ VK_MAX_MEMORY_HEAPS] = {};

    MappedType map() const
    {
        MappedType r;
        r.heapBudget = heapBudget;
        r.heapUsage = heapUsage;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_validation_features
struct ValidationFeaturesEXT
{
    using MappedType = vk::ValidationFeaturesEXT;

    Array<vk::ValidationFeatureEnableEXT> enabledValidationFeatures = {};
    Array<vk::ValidationFeatureDisableEXT> disabledValidationFeatures = {};

    MappedType map() const
    {
        MappedType r;
        r.enabledValidationFeatureCount = enabledValidationFeatures.size();
        r.pEnabledValidationFeatures = enabledValidationFeatures.data();
        r.disabledValidationFeatureCount = disabledValidationFeatures.size();
        r.pDisabledValidationFeatures = disabledValidationFeatures.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_host_image_copy
/*
struct PhysicalDeviceHostImageCopyPropertiesEXT
{
    using MappedType = vk::PhysicalDeviceHostImageCopyPropertiesEXT;

    Array<vk::ImageLayout>* copySrcLayouts = {};
    Array<vk::ImageLayout>* copyDstLayouts = {};
    uint8_t optimalTilingLayoutUUID[ VK_UUID_SIZE] = {};
    vk::Bool32 identicalMemoryTypeRequirements = {};

    MappedType map() const
    {
        MappedType r;
        r.copySrcLayoutCount = copySrcLayouts ? copySrcLayouts->size() : 0;
        r.pCopySrcLayouts = copySrcLayouts ? copySrcLayouts->data() : nullptr;
        r.copyDstLayoutCount = copyDstLayouts ? copyDstLayouts->size() : 0;
        r.pCopyDstLayouts = copyDstLayouts ? copyDstLayouts->data() : nullptr;
        r.optimalTilingLayoutUUID = optimalTilingLayoutUUID;
        r.identicalMemoryTypeRequirements = identicalMemoryTypeRequirements;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct CopyMemoryToImageInfoEXT
{
    using MappedType = vk::CopyMemoryToImageInfoEXT;

    vk::HostImageCopyFlagsEXT flags = {};
    vk::Image dstImage = {};
    vk::ImageLayout dstImageLayout = {};
    Array<vk::MemoryToImageCopyEXT> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.dstImage = dstImage;
        r.dstImageLayout = dstImageLayout;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct CopyImageToMemoryInfoEXT
{
    using MappedType = vk::CopyImageToMemoryInfoEXT;

    vk::HostImageCopyFlagsEXT flags = {};
    vk::Image srcImage = {};
    vk::ImageLayout srcImageLayout = {};
    Array<vk::ImageToMemoryCopyEXT> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.srcImage = srcImage;
        r.srcImageLayout = srcImageLayout;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct CopyImageToImageInfoEXT
{
    using MappedType = vk::CopyImageToImageInfoEXT;

    vk::HostImageCopyFlagsEXT flags = {};
    vk::Image srcImage = {};
    vk::ImageLayout srcImageLayout = {};
    vk::Image dstImage = {};
    vk::ImageLayout dstImageLayout = {};
    Array<vk::ImageCopy2> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.srcImage = srcImage;
        r.srcImageLayout = srcImageLayout;
        r.dstImage = dstImage;
        r.dstImageLayout = dstImageLayout;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_surface_maintenance1
struct SurfacePresentModeCompatibilityEXT
{
    using MappedType = vk::SurfacePresentModeCompatibilityEXT;

    Array<vk::PresentModeKHR>* presentModes = {};

    MappedType map() const
    {
        MappedType r;
        r.presentModeCount = presentModes ? presentModes->size() : 0;
        r.pPresentModes = presentModes ? presentModes->data() : nullptr;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_swapchain_maintenance1
struct SwapchainPresentFenceInfoEXT
{
    using MappedType = vk::SwapchainPresentFenceInfoEXT;

    Array<vk::Fence> fences = {};

    MappedType map() const
    {
        MappedType r;
        r.swapchainCount = fences.size();
        r.pFences = fences.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SwapchainPresentModesCreateInfoEXT
{
    using MappedType = vk::SwapchainPresentModesCreateInfoEXT;

    Array<vk::PresentModeKHR> presentModes = {};

    MappedType map() const
    {
        MappedType r;
        r.presentModeCount = presentModes.size();
        r.pPresentModes = presentModes.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct SwapchainPresentModeInfoEXT
{
    using MappedType = vk::SwapchainPresentModeInfoEXT;

    Array<vk::PresentModeKHR> presentModes = {};

    MappedType map() const
    {
        MappedType r;
        r.swapchainCount = presentModes.size();
        r.pPresentModes = presentModes.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct ReleaseSwapchainImagesInfoEXT
{
    using MappedType = vk::ReleaseSwapchainImagesInfoEXT;

    vk::SwapchainKHR swapchain = {};
    Array<uint32_t> imageIndices = {};

    MappedType map() const
    {
        MappedType r;
        r.swapchain = swapchain;
        r.imageIndexCount = imageIndices.size();
        r.pImageIndices = imageIndices.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_NV_device_generated_commands
struct GraphicsShaderGroupCreateInfoNV
{
    using MappedType = vk::GraphicsShaderGroupCreateInfoNV;

    Array<vk::PipelineShaderStageCreateInfo> stages = {};
    const vk::PipelineVertexInputStateCreateInfo* pVertexInputState = {};
    const vk::PipelineTessellationStateCreateInfo* pTessellationState = {};

    MappedType map() const
    {
        MappedType r;
        r.stageCount = stages.size();
        r.pStages = stages.data();
        r.pVertexInputState = pVertexInputState;
        r.pTessellationState = pTessellationState;
        return r;
    }

    operator MappedType() const { return map(); }
};

struct GraphicsPipelineShaderGroupsCreateInfoNV
{
    using MappedType = vk::GraphicsPipelineShaderGroupsCreateInfoNV;

    Array<vk::GraphicsShaderGroupCreateInfoNV> groups = {};
    Array<vk::Pipeline> pipelines = {};

    MappedType map() const
    {
        MappedType r;
        r.groupCount = groups.size();
        r.pGroups = groups.data();
        r.pipelineCount = pipelines.size();
        r.pPipelines = pipelines.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct IndirectCommandsLayoutTokenNV
{
    using MappedType = vk::IndirectCommandsLayoutTokenNV;

    vk::IndirectCommandsTokenTypeNV tokenType = {};
    uint32_t stream = {};
    uint32_t offset = {};
    uint32_t vertexBindingUnit = {};
    vk::Bool32 vertexDynamicStride = {};
    vk::PipelineLayout pushconstantPipelineLayout = {};
    vk::ShaderStageFlags pushconstantShaderStageFlags = {};
    uint32_t pushconstantOffset = {};
    uint32_t pushconstantSize = {};
    vk::IndirectStateFlagsNV indirectStateFlags = {};
    Array<vk::IndexType> indexTypes = {};
    Array<uint32_t> indexTypeValues = {};

    MappedType map() const
    {
        VKEX_ASSERT(indexTypeValues.size() == indexTypes.size());
        MappedType r;
        r.tokenType = tokenType;
        r.stream = stream;
        r.offset = offset;
        r.vertexBindingUnit = vertexBindingUnit;
        r.vertexDynamicStride = vertexDynamicStride;
        r.pushconstantPipelineLayout = pushconstantPipelineLayout;
        r.pushconstantShaderStageFlags = pushconstantShaderStageFlags;
        r.pushconstantOffset = pushconstantOffset;
        r.pushconstantSize = pushconstantSize;
        r.indirectStateFlags = indirectStateFlags;
        r.indexTypeCount = indexTypes.size();
        r.pIndexTypes = indexTypes.data();
        r.pIndexTypeValues = indexTypeValues.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct IndirectCommandsLayoutCreateInfoNV
{
    using MappedType = vk::IndirectCommandsLayoutCreateInfoNV;

    vk::IndirectCommandsLayoutUsageFlagsNV flags = {};
    vk::PipelineBindPoint pipelineBindPoint = {};
    Array<vk::IndirectCommandsLayoutTokenNV> tokens = {};
    Array<uint32_t> streamStrides = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.pipelineBindPoint = pipelineBindPoint;
        r.tokenCount = tokens.size();
        r.pTokens = tokens.data();
        r.streamCount = streamStrides.size();
        r.pStreamStrides = streamStrides.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct GeneratedCommandsInfoNV
{
    using MappedType = vk::GeneratedCommandsInfoNV;

    vk::PipelineBindPoint pipelineBindPoint = {};
    vk::Pipeline pipeline = {};
    vk::IndirectCommandsLayoutNV indirectCommandsLayout = {};
    Array<vk::IndirectCommandsStreamNV> streams = {};
    uint32_t sequencesCount = {};
    vk::Buffer preprocessBuffer = {};
    vk::DeviceSize preprocessOffset = {};
    vk::DeviceSize preprocessSize = {};
    vk::Buffer sequencesCountBuffer = {};
    vk::DeviceSize sequencesCountOffset = {};
    vk::Buffer sequencesIndexBuffer = {};
    vk::DeviceSize sequencesIndexOffset = {};

    MappedType map() const
    {
        MappedType r;
        r.pipelineBindPoint = pipelineBindPoint;
        r.pipeline = pipeline;
        r.indirectCommandsLayout = indirectCommandsLayout;
        r.streamCount = streams.size();
        r.pStreams = streams.data();
        r.sequencesCount = sequencesCount;
        r.preprocessBuffer = preprocessBuffer;
        r.preprocessOffset = preprocessOffset;
        r.preprocessSize = preprocessSize;
        r.sequencesCountBuffer = sequencesCountBuffer;
        r.sequencesCountOffset = sequencesCountOffset;
        r.sequencesIndexBuffer = sequencesIndexBuffer;
        r.sequencesIndexOffset = sequencesIndexOffset;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_NV_fragment_shading_rate_enums
/*
struct PipelineFragmentShadingRateEnumStateCreateInfoNV
{
    using MappedType = vk::PipelineFragmentShadingRateEnumStateCreateInfoNV;

    vk::FragmentShadingRateTypeNV shadingRateType = {};
    vk::FragmentShadingRateNV shadingRate = {};
    vk::FragmentShadingRateCombinerOpKHR combinerOps[2] = {};

    MappedType map() const
    {
        MappedType r;
        r.shadingRateType = shadingRateType;
        r.shadingRate = shadingRate;
        r.combinerOps = combinerOps;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_image_compression_control
/*
struct ImageCompressionControlEXT
{
    using MappedType = vk::ImageCompressionControlEXT;

    vk::ImageCompressionFlagsEXT flags = {};
    Array<vk::ImageCompressionFixedRateFlagsEXT>* fixedRateFlags = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.compressionControlPlaneCount = fixedRateFlags ? fixedRateFlags->size() : 0;
        r.pFixedRateFlags = fixedRateFlags ? fixedRateFlags->data() : nullptr;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_device_fault
/*
struct DeviceFaultVendorInfoEXT
{
    using MappedType = vk::DeviceFaultVendorInfoEXT;

    char description[ VK_MAX_DESCRIPTION_SIZE] = {};
    uint64_t vendorFaultCode = {};
    uint64_t vendorFaultData = {};

    MappedType map() const
    {
        MappedType r;
        r.description = description;
        r.vendorFaultCode = vendorFaultCode;
        r.vendorFaultData = vendorFaultData;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct DeviceFaultInfoEXT
{
    using MappedType = vk::DeviceFaultInfoEXT;

    char description[ VK_MAX_DESCRIPTION_SIZE] = {};
    vk::DeviceFaultAddressInfoEXT* pAddressInfos = {};
    vk::DeviceFaultVendorInfoEXT* pVendorInfos = {};
    void* pVendorBinaryData = {};

    MappedType map() const
    {
        MappedType r;
        r.description = description;
        r.pAddressInfos = pAddressInfos;
        r.pVendorInfos = pVendorInfos;
        r.pVendorBinaryData = pVendorBinaryData;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct DeviceFaultVendorBinaryHeaderVersionOneEXT
{
    using MappedType = vk::DeviceFaultVendorBinaryHeaderVersionOneEXT;

    uint32_t headerSize = {};
    vk::DeviceFaultVendorBinaryHeaderVersionEXT headerVersion = {};
    uint32_t vendorID = {};
    uint32_t deviceID = {};
    uint32_t driverVersion = {};
    uint8_t pipelineCacheUUID[ VK_UUID_SIZE] = {};
    uint32_t applicationNameOffset = {};
    uint32_t applicationVersion = {};
    uint32_t engineNameOffset = {};
    uint32_t engineVersion = {};
    uint32_t apiVersion = {};

    MappedType map() const
    {
        MappedType r;
        r.headerSize = headerSize;
        r.headerVersion = headerVersion;
        r.vendorID = vendorID;
        r.deviceID = deviceID;
        r.driverVersion = driverVersion;
        r.pipelineCacheUUID = pipelineCacheUUID;
        r.applicationNameOffset = applicationNameOffset;
        r.applicationVersion = applicationVersion;
        r.engineNameOffset = engineNameOffset;
        r.engineVersion = engineVersion;
        r.apiVersion = apiVersion;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_VALVE_mutable_descriptor_type
struct MutableDescriptorTypeListEXT
{
    using MappedType = vk::MutableDescriptorTypeListEXT;

    Array<vk::DescriptorType> descriptorTypes = {};

    MappedType map() const
    {
        MappedType r;
        r.descriptorTypeCount = descriptorTypes.size();
        r.pDescriptorTypes = descriptorTypes.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct MutableDescriptorTypeCreateInfoEXT
{
    using MappedType = vk::MutableDescriptorTypeCreateInfoEXT;

    Array<vk::MutableDescriptorTypeListEXT> mutableDescriptorTypeLists = {};

    MappedType map() const
    {
        MappedType r;
        r.mutableDescriptorTypeListCount = mutableDescriptorTypeLists.size();
        r.pMutableDescriptorTypeLists = mutableDescriptorTypeLists.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_FUCHSIA_buffer_collection
struct ImageFormatConstraintsInfoFUCHSIA
{
    using MappedType = vk::ImageFormatConstraintsInfoFUCHSIA;

    vk::ImageCreateInfo imageCreateInfo = {};
    vk::FormatFeatureFlags requiredFormatFeatures = {};
    vk::ImageFormatConstraintsFlagsFUCHSIA flags = {};
    uint64_t sysmemPixelFormat = {};
    Array<vk::SysmemColorSpaceFUCHSIA> colorSpaces = {};

    MappedType map() const
    {
        MappedType r;
        r.imageCreateInfo = imageCreateInfo;
        r.requiredFormatFeatures = requiredFormatFeatures;
        r.flags = flags;
        r.sysmemPixelFormat = sysmemPixelFormat;
        r.colorSpaceCount = colorSpaces.size();
        r.pColorSpaces = colorSpaces.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

struct ImageConstraintsInfoFUCHSIA
{
    using MappedType = vk::ImageConstraintsInfoFUCHSIA;

    Array<vk::ImageFormatConstraintsInfoFUCHSIA> formatConstraints = {};
    vk::BufferCollectionConstraintsInfoFUCHSIA bufferCollectionConstraints = {};
    vk::ImageConstraintsInfoFlagsFUCHSIA flags = {};

    MappedType map() const
    {
        MappedType r;
        r.formatConstraintsCount = formatConstraints.size();
        r.pFormatConstraints = formatConstraints.data();
        r.bufferCollectionConstraints = bufferCollectionConstraints;
        r.flags = flags;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_pipeline_properties
/*
struct PipelinePropertiesIdentifierEXT
{
    using MappedType = vk::PipelinePropertiesIdentifierEXT;

    uint8_t pipelineIdentifier[ VK_UUID_SIZE] = {};

    MappedType map() const
    {
        MappedType r;
        r.pipelineIdentifier = pipelineIdentifier;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_EXT_frame_boundary
struct FrameBoundaryEXT
{
    using MappedType = vk::FrameBoundaryEXT;

    vk::FrameBoundaryFlagsEXT flags = {};
    uint64_t frameID = {};
    Array<vk::Image> images = {};
    Array<vk::Buffer> buffers = {};
    uint64_t tagName = {};
    size_t tagSize = {};
    const void* pTag = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.frameID = frameID;
        r.imageCount = images.size();
        r.pImages = images.data();
        r.bufferCount = buffers.size();
        r.pBuffers = buffers.data();
        r.tagName = tagName;
        r.tagSize = tagSize;
        r.pTag = pTag;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_color_write_enable
struct PipelineColorWriteCreateInfoEXT
{
    using MappedType = vk::PipelineColorWriteCreateInfoEXT;

    Array<vk::Bool32> colorWriteEnables = {};

    MappedType map() const
    {
        MappedType r;
        r.attachmentCount = colorWriteEnables.size();
        r.pColorWriteEnables = colorWriteEnables.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_opacity_micromap
/*
struct MicromapBuildInfoEXT
{
    using MappedType = vk::MicromapBuildInfoEXT;

    vk::MicromapTypeEXT type = {};
    vk::BuildMicromapFlagsEXT flags = {};
    vk::BuildMicromapModeEXT mode = {};
    vk::MicromapEXT dstMicromap = {};
    Array<vk::MicromapUsageEXT> usageCounts = {};
    const vk::MicromapUsageEXT* const* ppUsageCounts = {};
    vk::DeviceOrHostAddressConstKHR data = {};
    vk::DeviceOrHostAddressKHR scratchData = {};
    vk::DeviceOrHostAddressConstKHR triangleArray = {};
    vk::DeviceSize triangleArrayStride = {};

    MappedType map() const
    {
        MappedType r;
        r.type = type;
        r.flags = flags;
        r.mode = mode;
        r.dstMicromap = dstMicromap;
        r.usageCountsCount = usageCounts.size();
        r.pUsageCounts = usageCounts.data();
        r.ppUsageCounts = ppUsageCounts;
        r.data = data;
        r.scratchData = scratchData;
        r.triangleArray = triangleArray;
        r.triangleArrayStride = triangleArrayStride;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct MicromapVersionInfoEXT
{
    using MappedType = vk::MicromapVersionInfoEXT;

    const uint8_t* pVersionData = {};

    MappedType map() const
    {
        MappedType r;
        r.pVersionData = pVersionData;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

/*
struct AccelerationStructureTrianglesOpacityMicromapEXT
{
    using MappedType = vk::AccelerationStructureTrianglesOpacityMicromapEXT;

    vk::IndexType indexType = {};
    vk::DeviceOrHostAddressConstKHR indexBuffer = {};
    vk::DeviceSize indexStride = {};
    uint32_t baseTriangle = {};
    Array<vk::MicromapUsageEXT> usageCounts = {};
    const vk::MicromapUsageEXT* const* ppUsageCounts = {};
    vk::MicromapEXT micromap = {};

    MappedType map() const
    {
        MappedType r;
        r.indexType = indexType;
        r.indexBuffer = indexBuffer;
        r.indexStride = indexStride;
        r.baseTriangle = baseTriangle;
        r.usageCountsCount = usageCounts.size();
        r.pUsageCounts = usageCounts.data();
        r.ppUsageCounts = ppUsageCounts;
        r.micromap = micromap;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_NV_displacement_micromap
/*
struct AccelerationStructureTrianglesDisplacementMicromapNV
{
    using MappedType = vk::AccelerationStructureTrianglesDisplacementMicromapNV;

    vk::Format displacementBiasAndScaleFormat = {};
    vk::Format displacementVectorFormat = {};
    vk::DeviceOrHostAddressConstKHR displacementBiasAndScaleBuffer = {};
    vk::DeviceSize displacementBiasAndScaleStride = {};
    vk::DeviceOrHostAddressConstKHR displacementVectorBuffer = {};
    vk::DeviceSize displacementVectorStride = {};
    vk::DeviceOrHostAddressConstKHR displacedMicromapPrimitiveFlags = {};
    vk::DeviceSize displacedMicromapPrimitiveFlagsStride = {};
    vk::IndexType indexType = {};
    vk::DeviceOrHostAddressConstKHR indexBuffer = {};
    vk::DeviceSize indexStride = {};
    uint32_t baseTriangle = {};
    Array<vk::MicromapUsageEXT> usageCounts = {};
    const vk::MicromapUsageEXT* const* ppUsageCounts = {};
    vk::MicromapEXT micromap = {};

    MappedType map() const
    {
        MappedType r;
        r.displacementBiasAndScaleFormat = displacementBiasAndScaleFormat;
        r.displacementVectorFormat = displacementVectorFormat;
        r.displacementBiasAndScaleBuffer = displacementBiasAndScaleBuffer;
        r.displacementBiasAndScaleStride = displacementBiasAndScaleStride;
        r.displacementVectorBuffer = displacementVectorBuffer;
        r.displacementVectorStride = displacementVectorStride;
        r.displacedMicromapPrimitiveFlags = displacedMicromapPrimitiveFlags;
        r.displacedMicromapPrimitiveFlagsStride = displacedMicromapPrimitiveFlagsStride;
        r.indexType = indexType;
        r.indexBuffer = indexBuffer;
        r.indexStride = indexStride;
        r.baseTriangle = baseTriangle;
        r.usageCountsCount = usageCounts.size();
        r.pUsageCounts = usageCounts.data();
        r.ppUsageCounts = ppUsageCounts;
        r.micromap = micromap;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_HUAWEI_cluster_culling_shader
/*
struct PhysicalDeviceClusterCullingShaderPropertiesHUAWEI
{
    using MappedType = vk::PhysicalDeviceClusterCullingShaderPropertiesHUAWEI;

    uint32_t maxWorkGroupCount[3] = {};
    uint32_t maxWorkGroupSize[3] = {};
    uint32_t maxOutputClusterCount = {};
    vk::DeviceSize indirectBufferOffsetAlignment = {};

    MappedType map() const
    {
        MappedType r;
        r.maxWorkGroupCount = maxWorkGroupCount;
        r.maxWorkGroupSize = maxWorkGroupSize;
        r.maxOutputClusterCount = maxOutputClusterCount;
        r.indirectBufferOffsetAlignment = indirectBufferOffsetAlignment;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_QCOM_fragment_density_map_offset
struct SubpassFragmentDensityMapOffsetEndInfoQCOM
{
    using MappedType = vk::SubpassFragmentDensityMapOffsetEndInfoQCOM;

    Array<vk::Offset2D> fragmentDensityOffsets = {};

    MappedType map() const
    {
        MappedType r;
        r.fragmentDensityOffsetCount = fragmentDensityOffsets.size();
        r.pFragmentDensityOffsets = fragmentDensityOffsets.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_subpass_merge_feedback
/*
struct RenderPassSubpassFeedbackInfoEXT
{
    using MappedType = vk::RenderPassSubpassFeedbackInfoEXT;

    vk::SubpassMergeStatusEXT subpassMergeStatus = {};
    char description[ VK_MAX_DESCRIPTION_SIZE] = {};
    uint32_t postMergeIndex = {};

    MappedType map() const
    {
        MappedType r;
        r.subpassMergeStatus = subpassMergeStatus;
        r.description = description;
        r.postMergeIndex = postMergeIndex;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_LUNARG_direct_driver_loading
struct DirectDriverLoadingListLUNARG
{
    using MappedType = vk::DirectDriverLoadingListLUNARG;

    vk::DirectDriverLoadingModeLUNARG mode = {};
    Array<vk::DirectDriverLoadingInfoLUNARG> drivers = {};

    MappedType map() const
    {
        MappedType r;
        r.mode = mode;
        r.driverCount = drivers.size();
        r.pDrivers = drivers.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_shader_module_identifier
/*
struct PhysicalDeviceShaderModuleIdentifierPropertiesEXT
{
    using MappedType = vk::PhysicalDeviceShaderModuleIdentifierPropertiesEXT;

    uint8_t shaderModuleIdentifierAlgorithmUUID[ VK_UUID_SIZE] = {};

    MappedType map() const
    {
        MappedType r;
        r.shaderModuleIdentifierAlgorithmUUID = shaderModuleIdentifierAlgorithmUUID;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct PipelineShaderStageModuleIdentifierCreateInfoEXT
{
    using MappedType = vk::PipelineShaderStageModuleIdentifierCreateInfoEXT;

    Array<uint8_t> identifier = {};

    MappedType map() const
    {
        MappedType r;
        r.identifierSize = identifier.size();
        r.pIdentifier = identifier.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct ShaderModuleIdentifierEXT
{
    using MappedType = vk::ShaderModuleIdentifierEXT;

    uint32_t identifierSize = {};
    uint8_t identifier[ VK_MAX_SHADER_MODULE_IDENTIFIER_SIZE_EXT] = {};

    MappedType map() const
    {
        MappedType r;
        r.identifierSize = identifierSize;
        r.identifier = identifier;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_NV_optical_flow
struct OpticalFlowExecuteInfoNV
{
    using MappedType = vk::OpticalFlowExecuteInfoNV;

    vk::OpticalFlowExecuteFlagsNV flags = {};
    Array<vk::Rect2D> regions = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.regionCount = regions.size();
        r.pRegions = regions.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_shader_object
/*
struct PhysicalDeviceShaderObjectPropertiesEXT
{
    using MappedType = vk::PhysicalDeviceShaderObjectPropertiesEXT;

    uint8_t shaderBinaryUUID[ VK_UUID_SIZE] = {};
    uint32_t shaderBinaryVersion = {};

    MappedType map() const
    {
        MappedType r;
        r.shaderBinaryUUID = shaderBinaryUUID;
        r.shaderBinaryVersion = shaderBinaryVersion;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct ShaderCreateInfoEXT
{
    using MappedType = vk::ShaderCreateInfoEXT;

    vk::ShaderCreateFlagsEXT flags = {};
    vk::ShaderStageFlagBits stage = {};
    vk::ShaderStageFlags nextStage = {};
    vk::ShaderCodeTypeEXT codeType = {};
    Array<std::byte> code = {};
    const char* pName = {};
    Array<vk::DescriptorSetLayout> setLayouts = {};
    Array<vk::PushConstantRange> pushConstantRanges = {};
    const vk::SpecializationInfo* pSpecializationInfo = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.stage = stage;
        r.nextStage = nextStage;
        r.codeType = codeType;
        r.codeSize = code.size();
        r.pCode = code.data();
        r.pName = pName;
        r.setLayoutCount = setLayouts.size();
        r.pSetLayouts = setLayouts.data();
        r.pushConstantRangeCount = pushConstantRanges.size();
        r.pPushConstantRanges = pushConstantRanges.data();
        r.pSpecializationInfo = pSpecializationInfo;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_QCOM_multiview_per_view_render_areas
struct MultiviewPerViewRenderAreasRenderPassBeginInfoQCOM
{
    using MappedType = vk::MultiviewPerViewRenderAreasRenderPassBeginInfoQCOM;

    Array<vk::Rect2D> perViewRenderAreas = {};

    MappedType map() const
    {
        MappedType r;
        r.perViewRenderAreaCount = perViewRenderAreas.size();
        r.pPerViewRenderAreas = perViewRenderAreas.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_KHR_acceleration_structure
/*
struct AccelerationStructureBuildGeometryInfoKHR
{
    using MappedType = vk::AccelerationStructureBuildGeometryInfoKHR;

    vk::AccelerationStructureTypeKHR type = {};
    vk::BuildAccelerationStructureFlagsKHR flags = {};
    vk::BuildAccelerationStructureModeKHR mode = {};
    vk::AccelerationStructureKHR srcAccelerationStructure = {};
    vk::AccelerationStructureKHR dstAccelerationStructure = {};
    Array<vk::AccelerationStructureGeometryKHR> geometries = {};
    const vk::AccelerationStructureGeometryKHR* const* ppGeometries = {};
    vk::DeviceOrHostAddressKHR scratchData = {};

    MappedType map() const
    {
        MappedType r;
        r.type = type;
        r.flags = flags;
        r.mode = mode;
        r.srcAccelerationStructure = srcAccelerationStructure;
        r.dstAccelerationStructure = dstAccelerationStructure;
        r.geometryCount = geometries.size();
        r.pGeometries = geometries.data();
        r.ppGeometries = ppGeometries;
        r.scratchData = scratchData;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

struct WriteDescriptorSetAccelerationStructureKHR
{
    using MappedType = vk::WriteDescriptorSetAccelerationStructureKHR;

    Array<vk::AccelerationStructureKHR> accelerationStructures = {};

    MappedType map() const
    {
        MappedType r;
        r.accelerationStructureCount = accelerationStructures.size();
        r.pAccelerationStructures = accelerationStructures.data();
        return r;
    }

    operator MappedType() const { return map(); }
};

/*
struct AccelerationStructureVersionInfoKHR
{
    using MappedType = vk::AccelerationStructureVersionInfoKHR;

    const uint8_t* pVersionData = {};

    MappedType map() const
    {
        MappedType r;
        r.pVersionData = pVersionData;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

#ifdef VK_KHR_ray_tracing_pipeline
struct RayTracingPipelineCreateInfoKHR
{
    using MappedType = vk::RayTracingPipelineCreateInfoKHR;

    vk::PipelineCreateFlags flags = {};
    Array<vk::PipelineShaderStageCreateInfo> stages = {};
    Array<vk::RayTracingShaderGroupCreateInfoKHR> groups = {};
    uint32_t maxPipelineRayRecursionDepth = {};
    const vk::PipelineLibraryCreateInfoKHR* pLibraryInfo = {};
    const vk::RayTracingPipelineInterfaceCreateInfoKHR* pLibraryInterface = {};
    const vk::PipelineDynamicStateCreateInfo* pDynamicState = {};
    vk::PipelineLayout layout = {};
    vk::Pipeline basePipelineHandle = {};
    int32_t basePipelineIndex = {};

    MappedType map() const
    {
        MappedType r;
        r.flags = flags;
        r.stageCount = stages.size();
        r.pStages = stages.data();
        r.groupCount = groups.size();
        r.pGroups = groups.data();
        r.maxPipelineRayRecursionDepth = maxPipelineRayRecursionDepth;
        r.pLibraryInfo = pLibraryInfo;
        r.pLibraryInterface = pLibraryInterface;
        r.pDynamicState = pDynamicState;
        r.layout = layout;
        r.basePipelineHandle = basePipelineHandle;
        r.basePipelineIndex = basePipelineIndex;
        return r;
    }

    operator MappedType() const { return map(); }
};

#endif

#ifdef VK_EXT_mesh_shader
/*
struct PhysicalDeviceMeshShaderPropertiesEXT
{
    using MappedType = vk::PhysicalDeviceMeshShaderPropertiesEXT;

    uint32_t maxTaskWorkGroupTotalCount = {};
    uint32_t maxTaskWorkGroupCount[3] = {};
    uint32_t maxTaskWorkGroupInvocations = {};
    uint32_t maxTaskWorkGroupSize[3] = {};
    uint32_t maxTaskPayloadSize = {};
    uint32_t maxTaskSharedMemorySize = {};
    uint32_t maxTaskPayloadAndSharedMemorySize = {};
    uint32_t maxMeshWorkGroupTotalCount = {};
    uint32_t maxMeshWorkGroupCount[3] = {};
    uint32_t maxMeshWorkGroupInvocations = {};
    uint32_t maxMeshWorkGroupSize[3] = {};
    uint32_t maxMeshSharedMemorySize = {};
    uint32_t maxMeshPayloadAndSharedMemorySize = {};
    uint32_t maxMeshOutputMemorySize = {};
    uint32_t maxMeshPayloadAndOutputMemorySize = {};
    uint32_t maxMeshOutputComponents = {};
    uint32_t maxMeshOutputVertices = {};
    uint32_t maxMeshOutputPrimitives = {};
    uint32_t maxMeshOutputLayers = {};
    uint32_t maxMeshMultiviewViewCount = {};
    uint32_t meshOutputPerVertexGranularity = {};
    uint32_t meshOutputPerPrimitiveGranularity = {};
    uint32_t maxPreferredTaskWorkGroupInvocations = {};
    uint32_t maxPreferredMeshWorkGroupInvocations = {};
    vk::Bool32 prefersLocalInvocationVertexOutput = {};
    vk::Bool32 prefersLocalInvocationPrimitiveOutput = {};
    vk::Bool32 prefersCompactVertexOutput = {};
    vk::Bool32 prefersCompactPrimitiveOutput = {};

    MappedType map() const
    {
        MappedType r;
        r.maxTaskWorkGroupTotalCount = maxTaskWorkGroupTotalCount;
        r.maxTaskWorkGroupCount = maxTaskWorkGroupCount;
        r.maxTaskWorkGroupInvocations = maxTaskWorkGroupInvocations;
        r.maxTaskWorkGroupSize = maxTaskWorkGroupSize;
        r.maxTaskPayloadSize = maxTaskPayloadSize;
        r.maxTaskSharedMemorySize = maxTaskSharedMemorySize;
        r.maxTaskPayloadAndSharedMemorySize = maxTaskPayloadAndSharedMemorySize;
        r.maxMeshWorkGroupTotalCount = maxMeshWorkGroupTotalCount;
        r.maxMeshWorkGroupCount = maxMeshWorkGroupCount;
        r.maxMeshWorkGroupInvocations = maxMeshWorkGroupInvocations;
        r.maxMeshWorkGroupSize = maxMeshWorkGroupSize;
        r.maxMeshSharedMemorySize = maxMeshSharedMemorySize;
        r.maxMeshPayloadAndSharedMemorySize = maxMeshPayloadAndSharedMemorySize;
        r.maxMeshOutputMemorySize = maxMeshOutputMemorySize;
        r.maxMeshPayloadAndOutputMemorySize = maxMeshPayloadAndOutputMemorySize;
        r.maxMeshOutputComponents = maxMeshOutputComponents;
        r.maxMeshOutputVertices = maxMeshOutputVertices;
        r.maxMeshOutputPrimitives = maxMeshOutputPrimitives;
        r.maxMeshOutputLayers = maxMeshOutputLayers;
        r.maxMeshMultiviewViewCount = maxMeshMultiviewViewCount;
        r.meshOutputPerVertexGranularity = meshOutputPerVertexGranularity;
        r.meshOutputPerPrimitiveGranularity = meshOutputPerPrimitiveGranularity;
        r.maxPreferredTaskWorkGroupInvocations = maxPreferredTaskWorkGroupInvocations;
        r.maxPreferredMeshWorkGroupInvocations = maxPreferredMeshWorkGroupInvocations;
        r.prefersLocalInvocationVertexOutput = prefersLocalInvocationVertexOutput;
        r.prefersLocalInvocationPrimitiveOutput = prefersLocalInvocationPrimitiveOutput;
        r.prefersCompactVertexOutput = prefersCompactVertexOutput;
        r.prefersCompactPrimitiveOutput = prefersCompactPrimitiveOutput;
        return r;
    }

    operator MappedType() const { return map(); }
};
*/

#endif

} // namespace vkex

#endif
