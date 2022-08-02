
#pragma once

#include "Teide/Pipeline.h"
#include "Vulkan.h"

struct VulkanPipeline : public Pipeline
{
	VulkanPipeline(vk::UniquePipeline pipeline, vk::PipelineLayout layout) :
	    pipeline{std::move(pipeline)}, layout{layout}
	{}

	vk::UniquePipeline pipeline;
	vk::PipelineLayout layout;
};

template <>
struct VulkanImpl<Pipeline>
{
	using type = VulkanPipeline;
};
