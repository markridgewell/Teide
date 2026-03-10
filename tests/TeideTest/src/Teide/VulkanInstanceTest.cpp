
#include "Teide/VulkanInstance.h"

#include <gmock/gmock.h>

#include <algorithm>
#include <vector>

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

TEST(VulkanInstanceTest, NoExtensionsAdded)
{
    const auto [enabled, missingReq, missingOpt] = GetInstanceExtensions({}, availableExtensions);
    EXPECT_THAT(enabled, IsEmpty());
    EXPECT_THAT(missingReq, IsEmpty());
    EXPECT_THAT(missingOpt, IsEmpty());
}

TEST(VulkanInstanceTest, OptionalExtensionFound)
{
    auto extensions = std::vector<InstanceExtensionName>();
    extensions.push_back("VK_KHR_surface");
    const auto [enabled, missingReq, missingOpt]
        = GetInstanceExtensions({.optionalExtensions = extensions}, availableExtensions);
    EXPECT_THAT(enabled, ElementsAre(StrEq("VK_KHR_surface")));
    EXPECT_THAT(missingReq, IsEmpty());
    EXPECT_THAT(missingOpt, IsEmpty());
}

TEST(VulkanInstanceTest, OptionalExtensionNotFound)
{
    auto extensions = std::vector<InstanceExtensionName>();
    extensions.push_back("VK_EXT_debug_report");
    const auto [enabled, missingReq, missingOpt]
        = GetInstanceExtensions({.optionalExtensions = extensions}, availableExtensions);
    EXPECT_THAT(enabled, IsEmpty());
    EXPECT_THAT(missingReq, IsEmpty());
    EXPECT_THAT(missingOpt, ElementsAre(StrEq("VK_EXT_debug_report")));
}

TEST(VulkanInstanceTest, RequiredExtensionFound)
{
    auto extensions = std::vector<InstanceExtensionName>();
    extensions.push_back("VK_KHR_surface");
    extensions.push_back("VK_KHR_display");
    const auto [enabled, missingReq, missingOpt]
        = GetInstanceExtensions({.requiredExtensions = extensions}, availableExtensions);
    EXPECT_THAT(enabled, ElementsAre(StrEq("VK_KHR_surface"), StrEq("VK_KHR_display")));
    EXPECT_THAT(missingReq, IsEmpty());
    EXPECT_THAT(missingOpt, IsEmpty());
}

TEST(VulkanInstanceTest, RequiredExtensionNotFound)
{
    auto extensions = std::vector<InstanceExtensionName>();
    extensions.push_back("VK_KHR_surface");
    extensions.push_back("VK_EXT_debug_report");
    const auto [enabled, missingReq, missingOpt]
        = GetInstanceExtensions({.requiredExtensions = extensions}, availableExtensions);
    EXPECT_THAT(enabled, ElementsAre(StrEq("VK_KHR_surface")));
    EXPECT_THAT(missingReq, ElementsAre(StrEq("VK_EXT_debug_report")));
    EXPECT_THAT(missingOpt, IsEmpty());
}

TEST(VulkanInstanceTest, OptionalLayerFound)
{
    auto enabledLayers = std::vector<const char*>{};
    EnableVulkanLayer(enabledLayers, availableLayers, "Debug");
    EXPECT_THAT(enabledLayers, ElementsAre(StrEq("Debug")));
}

TEST(VulkanInstanceTest, OptionalLayerNotFound)
{
    auto enabledLayers = std::vector<const char*>{};
    EnableVulkanLayer(enabledLayers, availableLayers, "SecretLayer");
    EXPECT_THAT(enabledLayers, IsEmpty());
}
