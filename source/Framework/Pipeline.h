
#pragma once

#include "Framework/Vulkan.h"

#include <vector>

struct VertexLayout
{
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	std::vector<vk::VertexInputBindingDescription> vertexInputBindings;
	std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes;
};

struct RenderStates
{
	vk::Viewport viewport;
	vk::Rect2D scissor;
	vk::PipelineRasterizationStateCreateInfo rasterizationState;
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	std::vector<vk::DynamicState> dynamicStates;
};

struct Pipeline
{
	vk::UniquePipeline pipeline;
	vk::PipelineLayout layout;
};
