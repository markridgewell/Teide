
#include "VulkanLoader.h"

#include "Vulkan.h"

#include <SDL2/SDL_filesystem.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <filesystem>
#include <string>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#endif


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace Teide
{

namespace
{
#ifdef _WIN32
    constexpr auto putenv = _putenv;
#endif

    std::filesystem::path FindSwiftShaderConfig()
    {
        const auto basePath = SDL_GetBasePath();
        const auto applicationDir = std::filesystem::path(basePath);
        SDL_free(basePath);
        const auto swiftshaderConfigPath = applicationDir / "vk_swiftshader_icd.json";
        if (!std::filesystem::exists(swiftshaderConfigPath))
        {
            throw VulkanError("Software rendering was requested, but no Vulkan software implementation was found.");
        }
        return swiftshaderConfigPath;
    }

} // namespace

VulkanLoaderBase::VulkanLoaderBase(bool enableSoftwareRendering)
{
    // The VK_ICD_FILENAMES environment variable must be set *before* loading the Vulkan library.
    // Doing so in a base class ensures this happens before the DynamicLoader is created.
    static bool s_softwareRenderingEnabled = false;
    if (enableSoftwareRendering && !s_softwareRenderingEnabled)
    {
        spdlog::info("Enabling software rendering");
        static auto s_envVar = "VK_ICD_FILENAMES=" + FindSwiftShaderConfig().string();
        if (putenv(s_envVar.data()) == 0)
        {
            spdlog::info("Setting environment variable {}", s_envVar);
        }
        s_softwareRenderingEnabled = true;
    }
    else if (!enableSoftwareRendering && s_softwareRenderingEnabled)
    {
        spdlog::info("Disabling software rendering");
        static char s_envVar[] = "VK_ICD_FILENAMES=";
        if (putenv(s_envVar) == 0)
        {
            spdlog::info("Setting environment variable {}", s_envVar);
        }
        s_softwareRenderingEnabled = true;
    }
}

VulkanLoader::VulkanLoader(bool enableSoftwareRendering) : VulkanLoaderBase(enableSoftwareRendering)
{
#ifdef _WIN32
    const HINSTANCE library = LoadLibraryA("vulkan-1.dll");
    char filename[MAX_PATH];
    GetModuleFileNameA(library, filename, sizeof(filename));
    spdlog::info("Loaded {}", filename);
#endif

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
}

void VulkanLoader::LoadInstanceFunctions(vk::Instance instance)
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

void VulkanLoader::LoadDeviceFunctions(vk::Device device)
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
}

} // namespace Teide
