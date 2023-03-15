
#pragma once

#include <vulkan/vulkan.hpp>

#include <string>

namespace Teide
{

class VulkanLoader
{
public:
    explicit VulkanLoader(bool enableSoftwareRendering);

    void LoadInstanceFunctions(vk::Instance instance);
    void LoadDeviceFunctions(vk::Device device);

private:
    vk::DynamicLoader m_loader;
};

} // namespace Teide
