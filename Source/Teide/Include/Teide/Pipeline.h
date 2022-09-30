
#pragma once

#include "Teide/ForwardDeclare.h"
#include "Types/TextureData.h"

#include <vulkan/vulkan.hpp>

#include <compare>
#include <optional>
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

struct FramebufferLayout
{
	std::optional<TextureFormat> colorFormat;
	std::optional<TextureFormat> depthStencilFormat;
	vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

	auto operator<=>(const FramebufferLayout&) const = default;
};

struct PipelineData
{
	ShaderPtr shader;
	VertexLayout vertexLayout;
	RenderStates renderStates;
	FramebufferLayout framebufferLayout;
};

class Pipeline
{
public:
	virtual ~Pipeline() = default;
};
