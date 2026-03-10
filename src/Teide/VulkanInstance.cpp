
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
    inline std::string_view GetExtensionName(const vk::ExtensionProperties& obj)
    {
        return obj.extensionName;
    }

} // namespace

void EnableVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& availableLayers,
    const char* layerName, Required required)
{
    if (std::ranges::contains(availableLayers, std::string_view(layerName), GetLayerName))
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
    if (std::ranges::contains(availableExtensions, std::string_view(extensionName), GetExtensionName))
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

auto VulkanExtensionsBase::IsAvailableImpl(std::string_view extensionName) -> bool
{
    return std::ranges::contains(m_available, extensionName, GetExtensionName);
}

void VulkanExtensionsBase::AddRequiredImpl(const char* extensionName)
{
    if (IsAvailableImpl(extensionName))
    {
        spdlog::info("Enabling Vulkan extension {}", extensionName);
        m_enabled.push_back(extensionName);
    }
    else
    {
        throw vk::ExtensionNotPresentError(extensionName);
    }
}

void VulkanExtensionsBase::AddOptionalImpl(const char* extensionName)
{
    if (IsAvailableImpl(extensionName))
    {
        spdlog::info("Enabling Vulkan extension {}", extensionName);
        m_enabled.push_back(extensionName);
    }
    else
    {
        spdlog::warn("Vulkan extension {} not enabled!", extensionName);
    }
}

auto GetInstanceExtensions(const InstanceParams& params) -> InstanceExtensions
{
    auto ret = InstanceExtensions{};

    const auto available = vk::enumerateInstanceExtensionProperties();

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
        EnableVulkanLayer(layers, availableLayers, "VK_LAYER_KHRONOS_validation", Required::False);

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

        if (std::ranges::contains(extensions, "VK_EXT_validation_features"))
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
