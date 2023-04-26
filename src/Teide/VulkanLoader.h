
#pragma once

#include "VulkanConfig.h"

#include <string>

namespace Teide
{

class VulkanLoader
{
public:
    VulkanLoader();

    static void LoadInstanceFunctions(vk::Instance instance);
    static void LoadDeviceFunctions(vk::Device device);

private:
    vk::DynamicLoader m_loader;
};

bool IsSoftwareRenderingEnabled();

} // namespace Teide
