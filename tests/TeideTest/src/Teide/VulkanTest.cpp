
#include "Teide/Vulkan.h"

#include <gmock/gmock.h>

#include <ranges>

using namespace Teide;
using namespace testing;

static const auto availableExtensions = [] {
    std::vector<vk::ExtensionProperties> ret;
    std::ranges::copy("TestExtension", ret.emplace_back().extensionName.begin());
    std::ranges::copy("Debug", ret.emplace_back().extensionName.begin());
    return ret;
}();

static const auto availableLayers = [] {
    std::vector<vk::LayerProperties> ret;
    std::ranges::copy("Test1", ret.emplace_back().layerName.begin());
    std::ranges::copy("Debug", ret.emplace_back().layerName.begin());
    return ret;
}();

TEST(VulkanTest, OptionalExtensionFound)
{
    auto enabledExtensions = std::vector<const char*>{};
    EnableVulkanExtension(enabledExtensions, availableExtensions, "TestExtension", Required::False);
    EXPECT_THAT(enabledExtensions, ElementsAre(StrEq("TestExtension")));
}

TEST(VulkanTest, OptionalExtensionNotFound)
{
    auto enabledExtensions = std::vector<const char*>{};
    EnableVulkanExtension(enabledExtensions, availableExtensions, "NotFound", Required::False);
    EXPECT_THAT(enabledExtensions, IsEmpty());
}

TEST(VulkanTest, RequiredExtensionFound)
{
    auto enabledExtensions = std::vector<const char*>{};
    EnableVulkanExtension(enabledExtensions, availableExtensions, "TestExtension", Required::True);
    EXPECT_THAT(enabledExtensions, ElementsAre(StrEq("TestExtension")));
}

TEST(VulkanTest, RequiredExtensionNotFound)
{
    auto enabledExtensions = std::vector<const char*>{};
    EXPECT_THROW(
        EnableVulkanExtension(enabledExtensions, availableExtensions, "NotFound", Required::True),
        vk::ExtensionNotPresentError);
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
