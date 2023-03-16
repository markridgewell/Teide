
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

    static bool s_envVarSet = false;

    void SetEnvVar()
    {
        if (!s_envVarSet)
        {
            static auto s_envVar = "VK_ICD_FILENAMES=" + FindSwiftShaderConfig().string();
            if (putenv(s_envVar.data()) == 0)
            {
                spdlog::debug("Setting environment variable {}", s_envVar);
                s_envVarSet = true;
            }
        }
    }

    void UnsetEnvVar()
    {
        if (s_envVarSet)
        {
            static char s_envVar[] = "VK_ICD_FILENAMES=";
            if (putenv(s_envVar) == 0)
            {
                spdlog::debug("Setting environment variable {}", s_envVar);
                s_envVarSet = false;
            }
        }
    }

    std::string GetSoftwareVulkanLibraryName()
    {
        // Attempt to load the system-provided Vulkan loader with the env var set
        SetEnvVar();
        vk::DynamicLoader loader;
        const auto vkGetInstanceProcAddr = loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        const auto vkCreateInstance = PFN_vkCreateInstance(vkGetInstanceProcAddr(NULL, "vkCreateInstance"));
        VkInstance instance;
        VkInstanceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        if (vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS)
        {
            return "";
        }

        // That failed, so just load SwiftShader directly (debugging and validation extensions will be disabled)
        spdlog::warn("Error creating Vulkan instance from loader, falling back to SwiftShader library");

#ifdef _WIN32
        return "vk_swiftshader.dll";
#else
        return "libvk_swiftshader.so";
#endif
    }

    vk::DynamicLoader CreateLoader(bool enableSoftwareRendering)
    {
        // The VK_ICD_FILENAMES environment variable must be set *before* loading the Vulkan library.
        // Doing so in a base class ensures this happens before the DynamicLoader is created.
        if (enableSoftwareRendering)
        {
            spdlog::info("Enabling software rendering");
            static std::string s_libraryName = GetSoftwareVulkanLibraryName();

            if (s_libraryName.empty())
            {
                SetEnvVar();
            }

            // Load either the system Vulkan library (with the VK_ICD_FILENAMES env var set)
            // or the Swiftshader library
            return vk::DynamicLoader(s_libraryName);
        }
        else
        {
            spdlog::info("Disabling software rendering");
            UnsetEnvVar();

            // Load the system Vulkan library
            return vk::DynamicLoader{};
        }
    }
} // namespace


VulkanLoader::VulkanLoader(bool enableSoftwareRendering) : m_loader{CreateLoader(enableSoftwareRendering)}
{
#ifdef _WIN32
    {
        char filename[MAX_PATH];
        const auto vulkanLib = GetModuleHandle("vulkan-1.dll");
        const auto swiftshaderLib = GetModuleHandle("vk_swiftshader.dll");
        if (vulkanLib)
        {
            GetModuleFileName(vulkanLib, filename, sizeof(filename));
            spdlog::debug("Vulkan loaded at {}", filename);
        }
        if (enableSoftwareRendering && swiftshaderLib)
        {
            GetModuleFileName(swiftshaderLib, filename, sizeof(filename));
            spdlog::debug("SwiftShader loaded at {}", filename);
        }
    }
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
