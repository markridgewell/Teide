
#pragma once

#include "Vulkan.h"
#include "VulkanParameterBlock.h"
#include "VulkanShader.h"

#include "Teide/Pipeline.h"

#include <algorithm>

namespace Teide
{

struct VulkanPipeline : public Pipeline
{
    explicit VulkanPipeline(const VulkanShaderPtr& shader) : shader{shader}, layout{shader->pipelineLayout.get()} {}

    vk::Pipeline GetPipeline(const RenderPassDesc& renderPass) const
    {
        const auto it = std::ranges::find(pipelines, renderPass, &RenderPassPipeline::renderPass);
        assert(it != pipelines.end());
        return it->pipeline.get();
    }

    struct RenderPassPipeline
    {
        RenderPassDesc renderPass;
        vk::UniquePipeline pipeline;
    };

    VulkanShaderPtr shader;
    vk::PipelineLayout layout;
    std::vector<RenderPassPipeline> pipelines;
};

template <>
struct VulkanImpl<Pipeline>
{
    using type = VulkanPipeline;
};

} // namespace Teide
