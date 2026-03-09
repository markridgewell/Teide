
#pragma once

#include "Teide/BasicTypes.h"

#include <vulkan/vulkan_extension_inspection.hpp>

namespace Teide
{

class VulkanLoader;

enum class Required : bool
{
    False = false,
    True = true
};

void EnableVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& availableLayers,
    const char* layerName, Required required);
void EnableVulkanExtension(
    std::vector<const char*>& enabledExtensions, const std::vector<vk::ExtensionProperties>& availableExtensions,
    const char* extensionName, Required required);

class InstanceExtensionName
{
public:
    template <usize N>
    consteval InstanceExtensionName(const char (&name)[N]) : // cppcheck-suppress noExplicitConstructor
        m_name{&name[0]}
    {
#if (201907 <= __cpp_constexpr) && (!defined(__GNUC__) || (110400 < GCC_VERSION))
        if (not vk::isInstanceExtension(std::string(&name[0])))
        {
            throw std::runtime_error("Unknown instance extension name");
        }
#endif
    }

    constexpr std::string_view Get() const { return m_name; }
    constexpr operator const char*() const { return m_name; }

private:
    const char* m_name;
};

class VulkanExtensionsBase
{
public:
    using container = std::vector<const char*>;
    using value_type = container::value_type;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    using pointer = container::pointer;
    using reference = container::reference;

    explicit VulkanExtensionsBase(const std::vector<vk::ExtensionProperties>& available) : m_available{available} {}

    auto data() const { return m_enabled.data(); }
    auto size() const { return m_enabled.size(); }
    auto begin() const { return m_enabled.begin(); }
    auto end() const { return m_enabled.end(); }
    auto empty() const { return m_enabled.empty(); }

protected:
    auto IsAvailableImpl(std::string_view extensionName) -> bool;

    void AddRequiredImpl(const char* extensionName);
    void AddOptionalImpl(const char* extensionName);

    std::vector<const char*> m_enabled; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    const std::vector<vk::ExtensionProperties> m_available;
};

class VulkanInstanceExtensions : public VulkanExtensionsBase
{
public:
    VulkanInstanceExtensions() : VulkanExtensionsBase(vk::enumerateInstanceExtensionProperties()) {}

    explicit VulkanInstanceExtensions(const std::vector<vk::ExtensionProperties>& available) :
        VulkanExtensionsBase(available)
    {}

    auto IsAvailable(InstanceExtensionName extensionName) -> bool { return IsAvailableImpl(extensionName.Get()); }

    void AddRequired(InstanceExtensionName extensionName) { AddRequiredImpl(extensionName); }
    void AddOptional(InstanceExtensionName extensionName) { AddOptionalImpl(extensionName); }
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

auto GetInstanceExtensions(const InstanceParams& params) -> InstanceExtensions;

vk::UniqueInstance CreateInstance(VulkanLoader& loader, const InstanceParams& params);

} // namespace Teide
