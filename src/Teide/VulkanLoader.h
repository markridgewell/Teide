
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

private:
    vk::DynamicLoader m_loader;
    vk::DispatchLoaderDynamic& m_dispatch;
};

bool IsSoftwareRenderingEnabled();

} // namespace Teide
