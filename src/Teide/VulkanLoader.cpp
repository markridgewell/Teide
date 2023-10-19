
#include "VulkanLoader.h"

#include "Vulkan.h"

#include <SDL2/SDL_filesystem.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <string>

#if defined(_WIN32) && defined(_DEBUG)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#endif


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace Teide
{

namespace
{
    std::filesystem::path FindSwiftShaderConfig()
    {
        auto* const basePath = SDL_GetBasePath();
        const auto applicationDir = std::filesystem::path(basePath);
        SDL_free(basePath);
        auto swiftshaderConfigPath = applicationDir / "vk_swiftshader_icd.json";
        if (!std::filesystem::exists(swiftshaderConfigPath))
        {
            throw VulkanError("Software rendering was requested, but no Vulkan software implementation was found.");
        }
        return swiftshaderConfigPath;
    }

    std::string GetSoftwareVulkanLibraryName()
    {
        // Attempt to load the system-provided Vulkan loader with the env var set
        const vk::DynamicLoader loader;
        const auto vkGetInstanceProcAddr = loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        const auto vkCreateInstance = PFN_vkCreateInstance(vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
        VkInstance instance = nullptr;
        const VkInstanceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
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

    std::atomic_bool s_softwareRenderingEnabled = false;
    std::string s_vulkanLibraryName;

} // namespace

bool IsSoftwareRenderingEnabled()
{
    return s_softwareRenderingEnabled;
}

void EnableSoftwareRendering()
{
    if (!s_softwareRenderingEnabled.exchange(true))
    {
        spdlog::info("Enabling software rendering");

        // The VK_ICD_FILENAMES environment variable must be set loading the Vulkan library.
        const auto icdConfig = FindSwiftShaderConfig().string();
        if (SDL_setenv("VK_ICD_FILENAMES", icdConfig.c_str(), true) == 0)
        {
            spdlog::debug("Setting environment variable {}={}", "VK_ICD_FILENAMES", icdConfig);
        }

        // The SDL_VULKAN_PATH environment variable must be set before creating any SDL_Windows
        // with the SDL_WINDOW_VULKAN flag set.
        s_vulkanLibraryName = GetSoftwareVulkanLibraryName();
        if (!s_vulkanLibraryName.empty() && SDL_setenv("SDL_VULKAN_LIBRARY", s_vulkanLibraryName.c_str(), true) == 0)
        {
            spdlog::debug("Setting environment variable {}={}", "SDL_VULKAN_LIBRARY", s_vulkanLibraryName);
        }
    }
}

VulkanLoader::VulkanLoader() : m_loader(s_vulkanLibraryName), m_dispatch{VULKAN_HPP_DEFAULT_DISPATCHER}
{
#if defined(_WIN32) && defined(_DEBUG)
    {
        char filename[MAX_PATH];
        const auto vulkanLib = GetModuleHandle("vulkan-1.dll");
        const auto swiftshaderLib = GetModuleHandle("vk_swiftshader.dll");
        if (vulkanLib)
        {
            GetModuleFileName(vulkanLib, filename, sizeof(filename));
            spdlog::debug("Vulkan loaded at {}", filename);
        }
        if (s_softwareRenderingEnabled && swiftshaderLib)
        {
            GetModuleFileName(swiftshaderLib, filename, sizeof(filename));
            spdlog::debug("SwiftShader loaded at {}", filename);
        }
    }
#endif

    m_dispatch.init(m_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
}

void VulkanLoader::LoadInstanceFunctions(vk::Instance instance)
{
    m_dispatch.init(instance);
}

void VulkanLoader::LoadDeviceFunctions(vk::Device device)
{
    m_dispatch.init(device);

    m_allocatorFunctions = vma::functionsFromDispatcher(m_dispatch);
    assert(m_allocatorFunctions.vkGetInstanceProcAddr);
    assert(m_allocatorFunctions.vkGetDeviceProcAddr);
}

const vma::VulkanFunctions& VulkanLoader::GetAllocatorFunctions() const
{
    return m_allocatorFunctions;
}

} // namespace Teide
