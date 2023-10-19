
#pragma once

#include "VulkanConfig.h"

#include <string>

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
    vk::DynamicLoader m_loader;
    vk::DispatchLoaderDynamic& m_dispatch;
    vma::VulkanFunctions m_allocatorFunctions;
};

bool IsSoftwareRenderingEnabled();

} // namespace Teide
