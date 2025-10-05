
#pragma once

#include "Vulkan.h"
#include "VulkanParameterBlock.h"

#include "Teide/Kernel.h"

namespace Teide
{

struct VulkanKernel
{
    vk::UniqueShaderModule computeShader;
    std::vector<ShaderVariable> inputs;
    std::vector<ShaderVariable> outputs;
    VulkanParameterBlockLayoutPtr scenePblockLayout;
    VulkanParameterBlockLayoutPtr viewPblockLayout;
    VulkanParameterBlockLayoutPtr paramsPblockLayout;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
};

template <>
struct VulkanImpl<Kernel>
{
    using type = VulkanKernel;
};

} // namespace Teide
