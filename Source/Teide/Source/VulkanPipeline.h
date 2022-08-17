
#pragma once

#include "Teide/Pipeline.h"
#include "Vulkan.h"

struct VulkanPipeline : public Pipeline
{
	VulkanPipeline(vk::UniquePipeline pipeline, vk::PipelineLayout layout, vk::ShaderStageFlags pushConstantsShaderStages) :
	    pipeline{std::move(pipeline)}, layout{layout}, pushConstantsShaderStages{pushConstantsShaderStages}
	{}

	vk::UniquePipeline pipeline;
	vk::PipelineLayout layout;
	vk::ShaderStageFlags pushConstantsShaderStages;
};

template <>
struct VulkanImpl<Pipeline>
{
	using type = VulkanPipeline;
};
