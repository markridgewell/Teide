
#pragma once

#include "Teide/ForwardDeclare.h"
#include "Types/PipelineData.h"

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
