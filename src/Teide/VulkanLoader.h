
#pragma once

#include <vulkan/vulkan.hpp>

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
};

bool IsSoftwareRenderingEnabled();

} // namespace Teide
