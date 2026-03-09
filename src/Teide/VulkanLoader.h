
#pragma once

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

namespace Teide
{

class VulkanLoader
{
public:
    VulkanLoader();

    void LoadInstanceFunctions(vk::Instance instance);
    void LoadDeviceFunctions(vk::Device device);
    const vma::VulkanFunctions& GetAllocatorFunctions() const;

private:
    vk::detail::DynamicLoader m_loader;
    vk::detail::DispatchLoaderDynamic& m_dispatch;
    vma::VulkanFunctions m_allocatorFunctions;
};

bool IsSoftwareRenderingEnabled();

} // namespace Teide
