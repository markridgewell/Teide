
#include "VulkanInstance.h"

#include "Teide/Definitions.h"
#include "Teide/Vulkan.h"
#include "Teide/VulkanLoader.h"

#include <SDL_vulkan.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <algorithm>

namespace Teide
{

namespace
{
    const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

    inline std::string_view GetLayerName(const vk::LayerProperties& obj)
    {
        return obj.layerName;
    }

} // namespace

void EnableVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& availableLayers, const char* layerName)
{
    if (std::ranges::contains(availableLayers, layerName, GetLayerName))
    {
        spdlog::info("Enabling Vulkan layer {}", layerName);
        enabledLayers.push_back(layerName);
    }
    else
    {
        spdlog::warn("Vulkan layer not supported: {}", layerName);
    }
}

auto GetInstanceExtensions(const InstanceParams& params, std::span<const vk::ExtensionProperties> available) -> InstanceExtensions
{
    auto ret = InstanceExtensions{};

    for (const char* extension : params.requiredExtensions)
    {
        if (std::ranges::contains(available, extension, GetExtensionName))
        {
            ret.supported.push_back(extension);
        }
        else
        {
            ret.missingRequired.push_back(extension);
        }
    }

    for (const char* extension : params.optionalExtensions)
    {
        if (std::ranges::contains(available, extension, GetExtensionName))
        {
            ret.supported.push_back(extension);
        }
        else
        {
            ret.missingOptional.push_back(extension);
        }
    }

    return ret;
}

auto GetInstanceExtensions(const InstanceParams& params) -> InstanceExtensions
{
    return GetInstanceExtensions(params, vk::enumerateInstanceExtensionProperties());
}

vk::UniqueInstance CreateInstance(VulkanLoader& loader, const InstanceParams& params)
{
    using std::data;

    const auto [extensions, missingReq, missingOpt] = GetInstanceExtensions(params);
    if (not missingReq.empty())
    {
        throw vk::ExtensionNotPresentError(fmt::format("{}", missingReq));
    }
    if (not missingOpt.empty())
    {
        spdlog::warn("Instance extension(s) not supported: {}", missingOpt);
    }

    const vk::ApplicationInfo applicationInfo{
        .apiVersion = VulkanApiVersion,
    };

    const auto availableLayers = vk::enumerateInstanceLayerProperties();

    std::vector<const char*> layers;

    vk::UniqueInstance instance;
    if constexpr (IsDebugBuild)
    {
        EnableVulkanLayer(layers, availableLayers, "VK_LAYER_KHRONOS_validation");

        const std::array enabledFeatures = {
            vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
            vk::ValidationFeatureEnableEXT::eBestPractices,
        };

        vk::StructureChain createInfo = {
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

        if (not std::ranges::contains(extensions, "VK_EXT_validation_features"))
        {
            createInfo.unlink<vk::ValidationFeaturesEXT>();
        }

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

} // namespace Teide
