
#pragma once

#include "Teide/BasicTypes.h"

#include <vulkan/vulkan_extension_inspection.hpp>

namespace Teide
{

class VulkanLoader;

class InstanceExtensionName
{
public:
    template <usize N>
    consteval InstanceExtensionName(const char (&name)[N]) : // cppcheck-suppress noExplicitConstructor
        m_name{&name[0]}
    {
#if (201907 <= __cpp_constexpr) && (!defined(__GNUC__) || (110400 < GCC_VERSION))
        const std::string extension = name;
        if (!(vk::isInstanceExtension(extension)
              || (extension == "VK_KHR_xlib_surface")
              || (extension == "VK_KHR_xcb_surface")
              || (extension == "VK_KHR_wayland_surface")
              || (extension == "VK_KHR_win32_surface")))
        {
            throw std::runtime_error("Unknown instance extension name");
        }
#endif
    }

    explicit InstanceExtensionName(const char* dynamicName) : m_name{dynamicName} {}

    constexpr std::string_view Get() const { return m_name; }
    constexpr operator const char*() const { return m_name; }

private:
    const char* m_name;
};

struct InstanceParams
{
    std::span<const InstanceExtensionName> requiredExtensions;
    std::span<const InstanceExtensionName> optionalExtensions;
};

struct InstanceExtensions
{
    std::vector<const char*> supported;
    std::vector<const char*> missingRequired;
    std::vector<const char*> missingOptional;
};

void EnableVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& availableLayers, const char* layerName);

auto GetInstanceExtensions(const InstanceParams& params) -> InstanceExtensions;
auto GetInstanceExtensions(const InstanceParams& params, std::span<const vk::ExtensionProperties> available)
    -> InstanceExtensions;

vk::UniqueInstance CreateInstance(VulkanLoader& loader, const InstanceParams& params);

} // namespace Teide
