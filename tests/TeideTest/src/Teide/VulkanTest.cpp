
#include "Teide/Vulkan.h"

#include <gmock/gmock.h>

#include <ranges>

using namespace Teide;
using namespace testing;

static const auto availableExtensions = [] {
    std::vector<vk::ExtensionProperties> ret;
    std::ranges::copy("VK_KHR_surface", ret.emplace_back().extensionName.begin());
    std::ranges::copy("VK_KHR_display", ret.emplace_back().extensionName.begin());
    return ret;
}();

static const auto availableLayers = [] {
    std::vector<vk::LayerProperties> ret;
    std::ranges::copy("Test1", ret.emplace_back().layerName.begin());
    std::ranges::copy("Debug", ret.emplace_back().layerName.begin());
    return ret;
}();

TEST(VulkanTest, NoExtensionsAdded)
{
    auto enabledExtensions = VulkanInstanceExtensions(availableExtensions);
    EXPECT_THAT(enabledExtensions, IsEmpty());
}

TEST(VulkanTest, OptionalExtensionFound)
{
    auto enabledExtensions = VulkanInstanceExtensions(availableExtensions);
    enabledExtensions.AddOptional("VK_KHR_surface");
    EXPECT_THAT(enabledExtensions, ElementsAre(StrEq("VK_KHR_surface")));
}

TEST(VulkanTest, OptionalExtensionNotFound)
{
    auto enabledExtensions = VulkanInstanceExtensions(availableExtensions);
    enabledExtensions.AddOptional("VK_EXT_debug_report");
    EXPECT_THAT(enabledExtensions, IsEmpty());
}

TEST(VulkanTest, RequiredExtensionFound)
{
    auto enabledExtensions = VulkanInstanceExtensions(availableExtensions);
    enabledExtensions.AddRequired("VK_KHR_surface");
    enabledExtensions.AddRequired("VK_KHR_display");
    EXPECT_THAT(enabledExtensions, ElementsAre(StrEq("VK_KHR_surface"), StrEq("VK_KHR_display")));
}

TEST(VulkanTest, RequiredExtensionNotFound)
{
    auto enabledExtensions = VulkanInstanceExtensions(availableExtensions);
    EXPECT_THROW(enabledExtensions.AddRequired("VK_EXT_debug_report"), vk::ExtensionNotPresentError);
    EXPECT_THAT(enabledExtensions, IsEmpty());
}

TEST(VulkanTest, OptionalLayerFound)
{
    auto enabledLayers = std::vector<const char*>{};
    EnableVulkanLayer(enabledLayers, availableLayers, "Debug", Required::False);
    EXPECT_THAT(enabledLayers, ElementsAre(StrEq("Debug")));
}

TEST(VulkanTest, OptionalLayerNotFound)
{
    auto enabledLayers = std::vector<const char*>{};
    EnableVulkanLayer(enabledLayers, availableLayers, "SecretLayer", Required::False);
    EXPECT_THAT(enabledLayers, IsEmpty());
}

TEST(VulkanTest, RequiredLayerFound)
{
    auto enabledLayers = std::vector<const char*>{};
    EnableVulkanLayer(enabledLayers, availableLayers, "Debug", Required::True);
    EXPECT_THAT(enabledLayers, ElementsAre(StrEq("Debug")));
}

TEST(VulkanTest, RequiredLayerNotFound)
{
    auto enabledLayers = std::vector<const char*>{};
    EXPECT_THROW(EnableVulkanLayer(enabledLayers, availableLayers, "SecretLayer", Required::True), vk::LayerNotPresentError);
    EXPECT_THAT(enabledLayers, IsEmpty());
}
