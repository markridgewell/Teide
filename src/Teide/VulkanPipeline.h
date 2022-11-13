
#pragma once

#include "Teide/Pipeline.h"
#include "Vulkan.h"
#include "VulkanParameterBlock.h"
#include "VulkanShader.h"

namespace Teide
{

struct VulkanPipeline : public Pipeline
{
    VulkanPipeline(const VulkanShader& shader, vk::UniquePipeline pipeline) :
        materialPblockLayout{shader.materialPblockLayout},
        objectPblockLayout{shader.objectPblockLayout},
        pipeline{std::move(pipeline)},
        layout{shader.pipelineLayout.get()}
    {}

    VulkanParameterBlockLayoutPtr materialPblockLayout;
    VulkanParameterBlockLayoutPtr objectPblockLayout;
    vk::UniquePipeline pipeline;
    vk::PipelineLayout layout;
};

template <>
struct VulkanImpl<Pipeline>
{
    using type = VulkanPipeline;
};

} // namespace Teide
