
#pragma once

#include "Teide/Shader.h"
#include "Vulkan.h"
#include "VulkanParameterBlock.h"

struct VulkanShaderBase
{
	vk::UniqueShaderModule vertexShader;
	vk::UniqueShaderModule pixelShader;
	VulkanParameterBlockLayoutPtr scenePblockLayout;
	VulkanParameterBlockLayoutPtr viewPblockLayout;
	VulkanParameterBlockLayoutPtr materialPblockLayout;
	VulkanParameterBlockLayoutPtr objectPblockLayout;
	vk::UniquePipelineLayout pipelineLayout;
};

struct VulkanShader : VulkanShaderBase, public Shader
{
	explicit VulkanShader(VulkanShaderBase base) : VulkanShaderBase{std::move(base)} {}

	ParameterBlockLayoutPtr GetMaterialPblockLayout() const override
	{
		return static_pointer_cast<const ParameterBlockLayout>(materialPblockLayout);
	}

	ParameterBlockLayoutPtr GetObjectPblockLayout() const override
	{
		return static_pointer_cast<const ParameterBlockLayout>(objectPblockLayout);
	}
};

template <>
struct VulkanImpl<Shader>
{
	using type = VulkanShader;
};
