
#pragma once

#include <vulkan/vulkan.hpp>

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
	vk::PipelineRasterizationStateCreateInfo rasterizationState = {.lineWidth = 1.0f};
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	vk::PipelineColorBlendAttachmentState colorBlendAttachment
	    = {.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
	           | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	std::vector<vk::DynamicState> dynamicStates;
};

struct Pipeline
{
	vk::UniquePipeline pipeline;
	vk::PipelineLayout layout;
};
